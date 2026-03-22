//
//  VertexData.hpp
//  test
//
//  Created by Albert Yang on 2026-02-11.
//

#pragma once
#include <simd/simd.h>

using namespace simd;

struct VertexData {
    float4 position;
};

struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};
