//
//  Garden.hpp
//  test
//
//  Created by Albert Yang on 2026-02-24.
//
#include <Plant.h>
#include <vector>
#pragma once
#include <simd/simd.h>
#include <vector>

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
#include <memory>

class Garden{
    
public:
    void draw(); // draws the plants sending everything the commands to MTL
    void step(float t);
    void parse();
    
private:
    std::vector<std::unique_ptr<Plants>> plants;
    Camera* camera;
    
}
