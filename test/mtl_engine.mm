//
//  mtl_engine.mm
//  test
//

#include <iostream>
#include <sstream>
#include <stack>
#include "mtl_engine.hpp"
#include "Camera.hpp"
#include "desktop_window.hpp"


// ─── parse tree builder (shared with type checker) ────────────────────────────

static std::shared_ptr<Node> build_tree(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) return nullptr;
    std::istringstream ss(line);
    std::vector<std::string> t; std::string w;
    while (ss >> w) t.push_back(w);
    if (t.empty()) return nullptr;
    bool term = !t[0].empty() && isupper(t[0][0]);
    if (term && t.size() == 2) return std::make_shared<Node>(t[0], t[1]);
    auto node = std::make_shared<Node>(t[0]);
    for (size_t i = 1; i < t.size(); ++i) {
        if (t[i] == ".EMPTY") continue;
        auto c = build_tree(in);
        if (c) node->children.push_back(c);
    }
    return node;
}

// ─── init ─────────────────────────────────────────────────────────────────────

void MTLEngine::init() {
    initDevice();
    initWindow();
    makeDesktopWindow(glfwWindow);

    createStatusBarItem([this](const char* code, const char* axiom, int steps) {
        std::string src  = code;
        std::string axSrc= axiom;

        has_error = false;
        error_msg.clear();

        try {
            auto toks  = tokenize(src);
            auto input = tokens_to_parser_input(toks);

            Parser parser;
            if (parser.parse_input(input) != 0)
                throw std::runtime_error("Parse error");

            TypeChecker tc(parser.result);
            tc.type_check();

            l_system_       = std::make_unique<Lsystem>(tc.tree());
            current_string_ = parse_axiom(axSrc.empty() ? "F(1.0)" : axSrc);
            steps_          = steps;

            // run requested number of steps
            for (int i = 0; i < steps_; ++i)
                current_string_ = l_system_->step(current_string_);

            geometry_dirty_ = true;

        } catch (const std::exception& e) {
            has_error = true;
            error_msg = e.what();
            l_system_.reset();
        }
    });

//    createCube();
    createDefaultLibrary();
    createCommandQueue();
    createLinePipeline();
    createTriPipeline();
//    createRenderPipeline();    // cube pipeline — keep for reference

    // depth texture (created once; resized in resizeFrameBuffer)
    rebuildDepthTexture();

    frameSemaphore = dispatch_semaphore_create(kMaxFramesInFlight);
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        uniformBuffers[i] = metalDevice->newBuffer(
            sizeof(Uniforms), MTL::ResourceStorageModeShared);
    }

    camera = new Camera(M_PI / 4.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    camera->setPosition({0, 5, 20});
    camera->lookAt({0, 5, 0}, {0, 1, 0});
}

// ─── depth texture ────────────────────────────────────────────────────────────

void MTLEngine::rebuildDepthTexture() {
    if (depthTexture) { depthTexture->release(); depthTexture = nullptr; }

    CGSize ds = metalLayer.drawableSize;
    if (ds.width <= 0 || ds.height <= 0) return;

    auto* td = MTL::TextureDescriptor::alloc()->init();
    td->setTextureType(MTL::TextureType2D);
    td->setPixelFormat(MTL::PixelFormatDepth32Float);
    td->setWidth( (NS::UInteger)ds.width);
    td->setHeight((NS::UInteger)ds.height);
    td->setUsage(MTL::TextureUsageRenderTarget);
    td->setStorageMode(MTL::StorageModePrivate);

    depthTexture = metalDevice->newTexture(td);
    td->release();
}

// ─── run loop ─────────────────────────────────────────────────────────────────

void MTLEngine::run() {
    static float t = 0.0f;
    while (!glfwWindowShouldClose(glfwWindow)) {
        @autoreleasepool {
            metalDrawable = (__bridge CA::MetalDrawable*)[metalLayer nextDrawable];
            draw();
            t += 0.01f; // speed

            float radius = 15.0f;

            float camX = radius * cos(t);
            float camY = radius * sin(t);

            camera->setPosition({camX, 5, camY});
            camera->lookAt({0, 5, 0}, {0, 1, 0});
        }
        glfwPollEvents();
    }
}

void MTLEngine::cleanup() {
    if (lineVertexBuffer) lineVertexBuffer->release();
    if (triVertexBuffer)  triVertexBuffer->release();
    if (depthTexture)     depthTexture->release();
    if (linePSO)          linePSO->release();
    if (triPSO)           triPSO->release();
    delete camera;
    glfwTerminate();
    metalDevice->release();
}

// ─── device + window ──────────────────────────────────────────────────────────

void MTLEngine::initDevice() {
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void MTLEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(800, 600, "plantground", NULL, NULL);
    if (!glfwWindow) { glfwTerminate(); exit(EXIT_FAILURE); }

    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, frameBufferSizeCallback);

    metalWindow = glfwGetCocoaWindow(glfwWindow);
    metalLayer  = [CAMetalLayer layer];
    metalLayer.device      = (__bridge id<MTLDevice>)metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.drawableSize= CGSizeMake(width, height);
    metalWindow.contentView.layer      = metalLayer;
    metalWindow.contentView.wantsLayer = YES;
}

void MTLEngine::frameBufferSizeCallback(GLFWwindow* window, int w, int h) {
    ((MTLEngine*)glfwGetWindowUserPointer(window))->resizeFrameBuffer(w, h);
}

void MTLEngine::resizeFrameBuffer(int w, int h) {
    metalLayer.drawableSize = CGSizeMake(w, h);
    rebuildDepthTexture();
    geometry_dirty_ = true;   // viewport changed — re-upload just in case
}

// ─── geometry upload ──────────────────────────────────────────────────────────

void MTLEngine::uploadLSystemGeometry() {
    if (!geometry_dirty_ || !l_system_) return;
    geometry_dirty_ = false;

    turtle_.reset();
    l_system_->draw(current_string_, turtle_);
    turtle_.flush();

    // line buffer
    if (lineVertexBuffer) { lineVertexBuffer->release(); lineVertexBuffer = nullptr; }
    lineVertCount = turtle_.line_verts.size();
    if (lineVertCount > 0) {
        lineVertexBuffer = metalDevice->newBuffer(
            turtle_.line_verts.data(),
            lineVertCount * sizeof(LineVertex),
            MTL::ResourceStorageModeShared);
    }

    // triangle buffer
    if (triVertexBuffer) { triVertexBuffer->release(); triVertexBuffer = nullptr; }
    triVertCount = turtle_.tri_verts.size();
    if (triVertCount > 0) {
        triVertexBuffer = metalDevice->newBuffer(
            turtle_.tri_verts.data(),
            triVertCount * sizeof(TriVertex),
            MTL::ResourceStorageModeShared);
    }
}

// ─── default library + command queue ──────────────────────────────────────────

void MTLEngine::createDefaultLibrary() {
    metalDefaultLibrary = metalDevice->newDefaultLibrary();
    if (!metalDefaultLibrary) { std::cerr << "Failed to load default library\n"; exit(-1); }
}

void MTLEngine::createCommandQueue() {
    metalCommandQueue = metalDevice->newCommandQueue();
}

// ─── pipeline helpers ─────────────────────────────────────────────────────────

static MTL::VertexDescriptor* posColorDescriptor(size_t colorOffset, size_t stride) {
    auto* vd = MTL::VertexDescriptor::vertexDescriptor();
    // attribute 0: float3 position
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(0);
    // attribute 1: float4 color
    vd->attributes()->object(1)->setFormat(MTL::VertexFormatFloat4);
    vd->attributes()->object(1)->setOffset(colorOffset);
    vd->attributes()->object(1)->setBufferIndex(0);
    vd->layouts()->object(0)->setStride(stride);
    return vd;
}

static void enableAlphaBlend(MTL::RenderPipelineColorAttachmentDescriptor* att) {
    att->setBlendingEnabled(true);
    att->setRgbBlendOperation(MTL::BlendOperationAdd);
    att->setAlphaBlendOperation(MTL::BlendOperationAdd);
    att->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    att->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    att->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    att->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
}

void MTLEngine::createLinePipeline() {
    auto* vfn = metalDefaultLibrary->newFunction(
        NS::String::string("lineVertexShader", NS::ASCIIStringEncoding));
    auto* ffn = metalDefaultLibrary->newFunction(
        NS::String::string("lineFragmentShader", NS::ASCIIStringEncoding));

    auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vfn);
    desc->setFragmentFunction(ffn);
    desc->colorAttachments()->object(0)->setPixelFormat(
        (MTL::PixelFormat)metalLayer.pixelFormat);
    desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    enableAlphaBlend(desc->colorAttachments()->object(0));
    desc->setVertexDescriptor(
        posColorDescriptor(offsetof(LineVertex, color), sizeof(LineVertex)));

    NS::Error* err = nullptr;
    linePSO = metalDevice->newRenderPipelineState(desc, &err);
    if (!linePSO) std::cerr << "linePSO error: "
                            << err->localizedDescription()->utf8String() << "\n";
    desc->release(); vfn->release(); ffn->release();
}

void MTLEngine::createTriPipeline() {
    auto* vfn = metalDefaultLibrary->newFunction(
        NS::String::string("triVertexShader", NS::ASCIIStringEncoding));
    auto* ffn = metalDefaultLibrary->newFunction(
        NS::String::string("triFragmentShader", NS::ASCIIStringEncoding));

    auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vfn);
    desc->setFragmentFunction(ffn);
    desc->colorAttachments()->object(0)->setPixelFormat(
        (MTL::PixelFormat)metalLayer.pixelFormat);
    desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    enableAlphaBlend(desc->colorAttachments()->object(0));
    desc->setVertexDescriptor(
        posColorDescriptor(offsetof(TriVertex, color), sizeof(TriVertex)));

    NS::Error* err = nullptr;
    triPSO = metalDevice->newRenderPipelineState(desc, &err);
    if (!triPSO) std::cerr << "triPSO error: "
                           << err->localizedDescription()->utf8String() << "\n";
    desc->release(); vfn->release(); ffn->release();
}

// cube pipeline — kept exactly as you had it
void MTLEngine::createRenderPipeline() {
    auto* vfn = metalDefaultLibrary->newFunction(
        NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    auto* ffn = metalDefaultLibrary->newFunction(
        NS::String::string("fragmentShader", NS::ASCIIStringEncoding));

    auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setLabel(NS::String::string("Cube Pipeline", NS::ASCIIStringEncoding));
    desc->setVertexFunction(vfn);
    desc->setFragmentFunction(ffn);
    desc->colorAttachments()->object(0)->setPixelFormat(
        (MTL::PixelFormat)metalLayer.pixelFormat);
    desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    auto* vd = MTL::VertexDescriptor::vertexDescriptor();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(0);
    vd->layouts()->object(0)->setStride(sizeof(simd_float4));
    vd->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    desc->setVertexDescriptor(vd);

    NS::Error* err = nullptr;
    metalRenderPSO = metalDevice->newRenderPipelineState(desc, &err);
    if (!metalRenderPSO) std::cerr << "cubePSO failed\n";
    desc->release(); vfn->release(); ffn->release();
}

// ─── depth stencil state ──────────────────────────────────────────────────────

MTL::DepthStencilState* MTLEngine::makeDepthState(bool writeDepth) {
    auto* dd = MTL::DepthStencilDescriptor::alloc()->init();
    dd->setDepthWriteEnabled(writeDepth);
    dd->setDepthCompareFunction(MTL::CompareFunctionLess);
    auto* ds = metalDevice->newDepthStencilState(dd);
    dd->release();
    return ds;
}

// ─── draw ─────────────────────────────────────────────────────────────────────

void MTLEngine::draw() {
    uploadLSystemGeometry();
    sendRenderCommand();
}

void MTLEngine::sendRenderCommand() {
    dispatch_semaphore_wait(frameSemaphore, DISPATCH_TIME_FOREVER);
    metalCommandBuffer = metalCommandQueue->commandBuffer();

    // ── render pass ────────────────────────────────────────────────────────
    auto* rpd = MTL::RenderPassDescriptor::alloc()->init();

    // colour attachment
    auto* cd = rpd->colorAttachments()->object(0);
    cd->setTexture(metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(0.06, 0.06, 0.08, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    // depth attachment
    if (depthTexture) {
        auto* dd = rpd->depthAttachment();
        dd->setTexture(depthTexture);
        dd->setLoadAction(MTL::LoadActionClear);
        dd->setClearDepth(1.0);
        dd->setStoreAction(MTL::StoreActionDontCare);
    }

    auto* enc = metalCommandBuffer->renderCommandEncoder(rpd);
    encodeRenderCommand(enc);
    enc->endEncoding();

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->addCompletedHandler(^(MTL::CommandBuffer*) {
        dispatch_semaphore_signal(frameSemaphore);
    });
    metalCommandBuffer->commit();
    currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    rpd->release();
}

void MTLEngine::encodeRenderCommand(MTL::RenderCommandEncoder* enc) {
    if (!metalDrawable) return;

    CGSize ds = metalLayer.drawableSize;
    enc->setViewport({0, 0, ds.width, ds.height, 0, 1});

    // ── uniforms ───────────────────────────────────────────────────────────
    float aspect = (float)ds.width / (float)ds.height;
    Uniforms u;
    u.modelMatrix      = matrix_identity_float4x4;   // L-system sits at world origin
    u.viewMatrix       = camera->getViewMatrix();
    u.projectionMatrix = camera->get_matrix_perspective_right_hand(
        60.f * (M_PI / 180.f), aspect, 0.1f, 500.f);

    MTL::Buffer* uBuf = uniformBuffers[currentFrame];
    memcpy(uBuf->contents(), &u, sizeof(u));
    enc->setVertexBuffer(uBuf, 0, 1);   // slot 1 = uniforms in shaders

    // depth state: write depth for tris, test-only for lines (so lines always show)
    static auto* depthWrite = makeDepthState(true);
    static auto* depthTest  = makeDepthState(false);

    // ── triangles first (with depth write) ────────────────────────────────
    if (triPSO && triVertexBuffer && triVertCount > 0) {
        enc->setRenderPipelineState(triPSO);
        enc->setDepthStencilState(depthWrite);
        enc->setVertexBuffer(triVertexBuffer, 0, 0);
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, triVertCount);
    }

    // ── lines on top (depth test only, no write so they always render) ────
    if (linePSO && lineVertexBuffer && lineVertCount > 0) {
        enc->setRenderPipelineState(linePSO);
        enc->setDepthStencilState(depthTest);
        enc->setVertexBuffer(lineVertexBuffer, 0, 0);
        enc->drawPrimitives(MTL::PrimitiveTypeLine, (NS::UInteger)0, lineVertCount);
    }
}

// ─── cube ─────────────────────────────────────────────────────────────────────

void MTLEngine::createCube() {
    simd_float4 verts[] = {
        {-0.5f,-0.5f, 0.5f,1}, { 0.5f,-0.5f, 0.5f,1},
        { 0.5f, 0.5f, 0.5f,1}, {-0.5f, 0.5f, 0.5f,1},
        {-0.5f,-0.5f,-0.5f,1}, { 0.5f,-0.5f,-0.5f,1},
        { 0.5f, 0.5f,-0.5f,1}, {-0.5f, 0.5f,-0.5f,1},
    };
    cubeVertexBuffer = metalDevice->newBuffer(verts, sizeof(verts),
                                              MTL::ResourceStorageModeShared);
    uint16_t idx[] = {
        0,1,1,2,2,3,3,0,  4,5,5,6,6,7,7,4,  0,4,1,5,2,6,3,7
    };
    cubeIndexBuffer = metalDevice->newBuffer(idx, sizeof(idx),
                                             MTL::ResourceStorageModeShared);
}
