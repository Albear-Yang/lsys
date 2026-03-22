//
//  Plant.hpp
//  test
//
//  Created by Albert Yang on 2026-02-23.
//

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

struct Rules{
    
};

struct Conditions{
    
    bool evaluate();
}

struct Functions{
    
}

class Plant {
public:
    Plant(&std::map<std::string, Rules> rules_);
    void draw();
    void step(float t); // step into the next thing...

    
private:
    std::map<std::string, Rules> rules_; // this is a map of rules with the key
    std::vector<std::unique_ptr<Symbol>> state_;
    std::map<std::string, simd::float4> colours;
    
    static MTL::Buffer* triangle_list_buffer_;
    static MTL::Buffer* line_list_buffer_;
    
    simd_float3 turtle_position_{(0,0,0)};
    simd_float4 turtle_orientation_;
};

class Symbol {
public:
    std::string id;
    float terminating_time{1};

private:
    std::vector<Symbol> 
};

