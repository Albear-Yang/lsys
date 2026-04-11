//
//  mtl_engine.mm
//  test
//
//  Created by Albert Yang on 2026-02-09.
//

// Objective-C frameworks used by this file:
#include <iostream>
#include "mtl_engine.hpp"
#include "Camera.hpp"
#include "desktop_window.hpp"

void MTLEngine::init() {
    initDevice();
    initWindow();
    
    makeDesktopWindow(glfwWindow);
    createStatusBarItem([this](int idx) {
//        currentProgram = idx;
    });
    
    createCube();
    createDefaultLibrary();
    createCommandQueue();
    createRenderPipeline();
    frameSemaphore = dispatch_semaphore_create(kMaxFramesInFlight);
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        uniformBuffers[i] = metalDevice->newBuffer(
            sizeof(Uniforms),
            MTL::ResourceStorageModeShared
        );
    }
    camera = new Camera(M_PI / 4.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    camera->setPosition({0,0,3});
    camera->lookAt({0, 0, 0}, {0, 1, 0});
    
}

void MTLEngine::run() {
    while (!glfwWindowShouldClose(glfwWindow)) {
        @autoreleasepool {
            metalDrawable = (__bridge CA::MetalDrawable*)[metalLayer nextDrawable];
            draw();
        }
        glfwPollEvents();
    }
}

void MTLEngine::cleanup() {
    glfwTerminate();
    metalDevice->release();
}

void MTLEngine::initDevice() {
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void MTLEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(800, 600, "Metal Engine", NULL, NULL);

    if (!glfwWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, frameBufferSizeCallback);

    metalWindow = glfwGetCocoaWindow(glfwWindow);
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = (__bridge id<MTLDevice>)metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.drawableSize = CGSizeMake(width, height);
    metalWindow.contentView.layer = metalLayer;
    metalWindow.contentView.wantsLayer = YES;
}


void MTLEngine::createTriangle() {
    simd::float3 triangleVertices[] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f}
    };

    triangleVertexBuffer = metalDevice->newBuffer(&triangleVertices,
                                                  sizeof(triangleVertices),
                                                  MTL::ResourceStorageModeShared);
}

void MTLEngine::createDefaultLibrary() {
    metalDefaultLibrary = metalDevice->newDefaultLibrary();
    if(!metalDefaultLibrary){
        std::cerr << "Failed to load default library.";
        std::exit(-1);
    }
}

void MTLEngine::createCommandQueue() {
    metalCommandQueue = metalDevice->newCommandQueue();
}

void MTLEngine::createRenderPipeline() {
    MTL::Function* vertexShader = metalDefaultLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Triangle Rendering Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

    NS::Error* error;
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::vertexDescriptor();
    // attribute 0: float4 position at offset 0 in buffer(0)
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // layout for buffer(0)
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(simd_float4));
    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);

    renderPipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    vertexDescriptor->release();

    metalRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);

    if (!metalRenderPSO) {
        if (error) {
            std::cerr << "Failed to create pipeline state: "
                      << error->localizedDescription()->utf8String() << "\n";
        } else {
            std::cerr << "Failed to create pipeline state: unknown error\n";
        }
        std::exit(-1);
    }
    renderPipelineDescriptor->release();
}

void MTLEngine::draw() {
    sendRenderCommand();
}

void MTLEngine::sendRenderCommand() {
    dispatch_semaphore_wait(frameSemaphore, DISPATCH_TIME_FOREVER);
    metalCommandBuffer = metalCommandQueue->commandBuffer();

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
    
    // Add viewport
    MTL::Viewport viewport = {0.0, 0.0, 800.0, 600.0, 0.0, 1.0};
    renderCommandEncoder->setViewport(viewport);
    
    encodeRenderCommand(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->addCompletedHandler(^void(MTL::CommandBuffer* buffer) {
        dispatch_semaphore_signal(frameSemaphore);
    });
    metalCommandBuffer->commit();
    currentFrame = (currentFrame + 1) % kMaxFramesInFlight;

    renderPassDescriptor->release();
}

void MTLEngine::encodeRenderCommand(MTL::RenderCommandEncoder* renderCommandEncoder) {
    // Safety: make sure we have a drawable (callers set metalDrawable earlier)
    if (!metalDrawable) {
        std::cerr << "No metalDrawable! Skipping frame.\n";
        return;
    }

    CGSize ds = metalLayer.drawableSize;
    MTL::Viewport viewport = {0.0, 0.0, ds.width, ds.height, 0.0, 1.0};
    renderCommandEncoder->setViewport(viewport);

    // Bind cube vertex buffer at index 0 (matches vertex descriptor bufferIndex 0)
    float angleInDegrees = glfwGetTime() / 2.0f * 45.0f;
    float angleInRadians = angleInDegrees * M_PI / 180.0f;

    float c = cosf(angleInRadians);
    float s = sinf(angleInRadians);

    // Column-major (IMPORTANT)
    simd_float4x4 rotationMatrix = {
        simd_make_float4( c,  0, -s,  0),
        simd_make_float4( 0,  1,  0,  0),
        simd_make_float4( s,  0,  c,  0),
        simd_make_float4( 0,  0,  0,  1)
    };
    
    simd_float4x4 rotationMatrix2 = {
        simd_make_float4(1,  0,  0,  0),   // column 0
        simd_make_float4(0,  c,  s,  0),   // column 1
        simd_make_float4(0, -s,  c,  0),   // column 2
        simd_make_float4(0,  0,  0,  1)    // column 3
    };


    float aspectRatio = (metalLayer.frame.size.width / metalLayer.frame.size.height);
    float fov = 120 * (M_PI / 180.0f);
    float nearZ = 0.1f;
    float farZ = 100.0f;
    Uniforms uniforms;
    uniforms.modelMatrix = simd_mul(rotationMatrix, rotationMatrix2);
    uniforms.viewMatrix  = camera->getViewMatrix();
    uniforms.projectionMatrix = camera->get_matrix_perspective_right_hand(fov,aspectRatio, nearZ, farZ);
    
    // Upload uniforms to buffer(1) (matches shader [[buffer(1)]])
    MTL::Buffer* currentUniformBuffer = uniformBuffers[currentFrame];

    memcpy(currentUniformBuffer->contents(), &uniforms, sizeof(uniforms));

    renderCommandEncoder->setVertexBuffer(currentUniformBuffer, 0, 1);
    
    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);

    renderCommandEncoder->setVertexBuffer(cubeVertexBuffer, 0, 0);
    renderCommandEncoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeLine,
        24,
        MTL::IndexTypeUInt16,
        cubeIndexBuffer,
        0
    );
}


void MTLEngine::frameBufferSizeCallback(GLFWwindow *window, int width, int height) {
    MTLEngine* engine = (MTLEngine*)glfwGetWindowUserPointer(window);
    engine->resizeFrameBuffer(width, height);
}

void MTLEngine::resizeFrameBuffer(int width, int height) {
    metalLayer.drawableSize = CGSizeMake(width, height);
}

void MTLEngine::createCube() {
    // 8 vertices of a cube as float4
    simd_float4 cubeVertices[] = {
        // Front face
        {-0.5f, -0.5f,  0.5f, 1.0f},  // 0
        { 0.5f, -0.5f,  0.5f, 1.0f},  // 1
        { 0.5f,  0.5f,  0.5f, 1.0f},  // 2
        {-0.5f,  0.5f,  0.5f, 1.0f},  // 3
        // Back face
        {-0.5f, -0.5f, -0.5f, 1.0f},  // 4
        { 0.5f, -0.5f, -0.5f, 1.0f},  // 5
        { 0.5f,  0.5f, -0.5f, 1.0f},  // 6
        {-0.5f,  0.5f, -0.5f, 1.0f}   // 7
    };
    
    cubeVertexBuffer = metalDevice->newBuffer(&cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared);
    
    // 12 lines (edges) of the cube
    uint16_t cubeIndices[] = {
        // Front face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connect front to back
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    cubeIndexBuffer = metalDevice->newBuffer(&cubeIndices, sizeof(cubeIndices), MTL::ResourceStorageModeShared);
}
