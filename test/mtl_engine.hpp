//
//  mtl_engine.hpp
//  test
//
//  Created by Albert Yang on 2026-02-09.
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

#include <simd/simd.h>

struct Uniforms {
    simd_float4x4 modelMatrix;
    simd_float4x4 viewMatrix;
    simd_float4x4 projectionMatrix;
};

class MTLEngine {
public:
    void init();
    void run();
    void cleanup();

private:
    void initDevice();
    void initWindow();

    void createTriangle();
    void createCube();
    void createDefaultLibrary();
    void createCommandQueue();
    void createRenderPipeline();

    void encodeRenderCommand(MTL::RenderCommandEncoder* renderEncoder);
    void sendRenderCommand();
    void draw();
    static void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
    void resizeFrameBuffer(int width, int height);

    MTL::Device* metalDevice;
    GLFWwindow* glfwWindow;
    NSWindow* metalWindow;
    CAMetalLayer* metalLayer;
    CA::MetalDrawable* metalDrawable;

    MTL::Library* metalDefaultLibrary;
    MTL::CommandQueue* metalCommandQueue;
    MTL::CommandBuffer* metalCommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO;
    MTL::Buffer* triangleVertexBuffer;
    MTL::Buffer* squareVertexBuffer;
    MTL::Buffer* cubeVertexBuffer;
    MTL::Buffer* cubeIndexBuffer;

    //frames in flight code
    static const int kMaxFramesInFlight = 3;

    dispatch_semaphore_t frameSemaphore;
    int currentFrame = 0;

    MTL::Buffer* uniformBuffers[kMaxFramesInFlight];
    
    Camera* camera;
};
