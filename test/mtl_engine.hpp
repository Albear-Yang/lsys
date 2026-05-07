//
//  mtl_engine.hpp
//  test
//

#pragma once

#define GLFW_INCLUDE_NONE
#import <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>

#include <Metal/Metal.hpp>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/QuartzCore.hpp>
#include "VertexData.hpp"
#include "Camera.hpp"
#include <filesystem>
#include <stack>
#include <vector>
#include <memory>
#include <string>

#include "lsysgen.hpp"
#include "lsyslex.hpp"
#include "lsystype.hpp"
#include "lsysparse.hpp"

#include <simd/simd.h>

struct Uniforms {
    simd_float4x4 modelMatrix;
    simd_float4x4 viewMatrix;
    simd_float4x4 projectionMatrix;
};

// ── vertex formats sent to the GPU ───────────────────────────────────────────

struct LineVertex {
    simd_float3 position;
    simd_float4 color;
};

struct TriVertex {
    simd_float3 position;
    simd_float4 color;
};

// ─── 3D turtle ────────────────────────────────────────────────────────────────

class Turtle3D : public Turtle {
public:
    // position
    float x = 0, y = 0, z = 0;

    // orientation as euler angles (degrees)
    float yaw_   = -90.f;   // left/right in XZ plane
    float pitch_ =   0.f;   // up/down
    float roll_  =   0.f;   // tilt around forward axis

    // active fill colour — negative means "no fill, draw lines"
    float fr = -1, fg = -1, fb = -1, fa = -1;

    struct State {
        float x,y,z, yaw,pitch,roll;
        float fr,fg,fb,fa;
        // save the current fill-poly start index so branches don't corrupt it
        int fill_start;
    };
    std::stack<State> stk;

    // output buffers filled by draw()
    std::vector<LineVertex> line_verts;   // pairs: [A, B, A, B, ...]
    std::vector<TriVertex>  tri_verts;    // triangles: [A, B, C, A, B, C, ...]

    // current open polygon for fill: we collect the path and triangulate as a fan
    struct FillPoly {
        simd_float4 color;
        std::vector<simd_float3> pts;
    };
    std::vector<FillPoly> fill_polys; // completed polys, flushed at end

    // currently-open polygon (between fill(...) and fill(-1,-1,-1,-1))
    FillPoly* active_fill = nullptr;

    void reset() {
        x=y=z=0; yaw_=0; pitch_= 90; roll_=0;
        fr=fg=fb=fa=-1;
        while(!stk.empty()) stk.pop();
        line_verts.clear();
        tri_verts.clear();
        fill_polys.clear();
        active_fill = nullptr;
    }

    // ── direction vector from current orientation ─────────────────────────
    simd_float3 forward_dir() const {
        float yR = yaw_   * M_PI / 180.f;
        float pR = pitch_ * M_PI / 180.f;
        return simd_make_float3(
            cosf(pR) * cosf(yR),
            sinf(pR),
            cosf(pR) * sinf(yR)
        );
    }

    // ── Turtle interface ──────────────────────────────────────────────────

    void forward(float dist) override {
        auto d  = forward_dir();
        float nx = x + d.x * dist;
        float ny = y + d.y * dist;
        float nz = z + d.z * dist;

        if (fr >= 0) {
            // fill mode — record path point for polygon
            if (!active_fill) {
                fill_polys.push_back({ simd_make_float4(fr, fg, fb, fa), {} });
                active_fill = &fill_polys.back();
                active_fill->pts.push_back(simd_make_float3(x, y, z));
            }
            active_fill->pts.push_back(simd_make_float3(nx, ny, nz));
        } else {
            // line mode
            simd_float4 col = simd_make_float4(1, 1, 1, 1); // default white
            line_verts.push_back({ simd_make_float3(x,  y,  z),  col });
            line_verts.push_back({ simd_make_float3(nx, ny, nz), col });
        }

        x = nx; y = ny; z = nz;
    }

    void yaw(float deg)   override { yaw_   += deg; }
    void pitch(float deg) override { pitch_ += deg; }
    void roll(float deg)  override { roll_  += deg; }

    void fill(float r, float g, float b, float a) override {
        if (r < 0) {
            // terminating fill — close and triangulate the current polygon
            if (active_fill && active_fill->pts.size() >= 3) {
                // fan triangulation from first point
                auto& pts = active_fill->pts;
                auto  col = active_fill->color;
                for (size_t i = 1; i + 1 < pts.size(); ++i) {
                    tri_verts.push_back({ pts[0],   col });
                    tri_verts.push_back({ pts[i],   col });
                    tri_verts.push_back({ pts[i+1], col });
                }
            }
            active_fill = nullptr;
            fr = fg = fb = fa = -1;
        } else {
            fr = r; fg = g; fb = b; fa = a;
            // start a fresh polygon
            fill_polys.push_back({ simd_make_float4(r, g, b, a), {} });
            active_fill = &fill_polys.back();
            active_fill->pts.push_back(simd_make_float3(x, y, z));
        }
    }

    void push() override {
        stk.push({x,y,z, yaw_,pitch_,roll_, fr,fg,fb,fa,
                  active_fill ? (int)(active_fill - fill_polys.data()) : -1});
    }

    void pop() override {
        if (stk.empty()) return;
        auto s = stk.top(); stk.pop();
        x=s.x; y=s.y; z=s.z;
        yaw_=s.yaw; pitch_=s.pitch; roll_=s.roll;
        fr=s.fr; fg=s.fg; fb=s.fb; fa=s.fa;
        // restore active_fill pointer
        active_fill = (s.fill_start >= 0 && s.fill_start < (int)fill_polys.size())
                    ? &fill_polys[s.fill_start] : nullptr;
    }

    // call after draw() to flush any unclosed polygon
    void flush() {
        if (active_fill) {
            fill(-1,-1,-1,-1); // close it
        }
    }
};

// ─── axiom parser (free function, defined in mtl_engine.mm) ──────────────────
std::vector<SymbolInstance> parse_axiom(const std::string& src);

// ─── MTLEngine ───────────────────────────────────────────────────────────────

class MTLEngine {
public:
    void init();
    void run();
    void cleanup();

private:
    void initDevice();
    void initWindow();
    void createCube();
    void createDefaultLibrary();
    void createCommandQueue();
    void createRenderPipeline();
    
    void draw();
    void sendRenderCommand();
    void encodeRenderCommand(MTL::RenderCommandEncoder* enc);

    // separate pipelines for lines and triangles
    void createLinePipeline();
    void createTriPipeline();
    // upload geometry from the turtle into Metal buffers each frame
    void uploadLSystemGeometry();

    static void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
    void resizeFrameBuffer(int width, int height);

    // ── Metal objects ─────────────────────────────────────────────────────
    MTL::Device*              metalDevice;
    GLFWwindow*               glfwWindow;
    NSWindow*                 metalWindow;
    CAMetalLayer*             metalLayer;
    CA::MetalDrawable*        metalDrawable;

    MTL::Library*             metalDefaultLibrary;
    MTL::CommandQueue*        metalCommandQueue;
    MTL::CommandBuffer*       metalCommandBuffer;

    // one pipeline per primitive type
    MTL::RenderPipelineState* linePSO   = nullptr;
    MTL::RenderPipelineState* triPSO    = nullptr;
    MTL::RenderPipelineState* metalRenderPSO;   // cube (kept for now)

    // dynamic geometry buffers (recreated when geometry changes)
    MTL::Buffer*  lineVertexBuffer = nullptr;
    MTL::Buffer*  triVertexBuffer  = nullptr;
    NSUInteger    lineVertCount    = 0;
    NSUInteger    triVertCount     = 0;

    // depth buffer — rebuilt on resize
    MTL::Texture* depthTexture     = nullptr;
    void rebuildDepthTexture();
    MTL::DepthStencilState* makeDepthState(bool writeDepth);

    // cube (debug)
    MTL::Buffer* cubeVertexBuffer;
    MTL::Buffer* cubeIndexBuffer;

    // frames in flight
    static const int kMaxFramesInFlight = 3;
    dispatch_semaphore_t frameSemaphore;
    int currentFrame = 0;
    MTL::Buffer* uniformBuffers[kMaxFramesInFlight];

    // ── L-system state ────────────────────────────────────────────────────
    std::unique_ptr<Lsystem>        l_system_;
    std::vector<SymbolInstance>     current_string_;
    Turtle3D                        turtle_;
    int                             steps_ = 0;
    bool                            geometry_dirty_ = false;

    std::string error_msg;
    bool        has_error = false;

    Camera* camera;
};
