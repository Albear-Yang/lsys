//
//  Math3D.hpp
//  test
//
//  Created by Albert Yang on 2026-02-18.
//

#pragma once
#include <simd/simd.h>

struct Transform {
    simd_float4x4 modelMatrix;
    simd_float4x4 viewMatrix;
    simd_float4x4 projectionMatrix;
};

// Helper functions for creating matrices
simd_float4x4 matrixPerspective(float fov, float aspect, float near, float far);
simd_float4x4 matrixTranslate(float x, float y, float z);
simd_float4x4 matrixRotateZ(float angle);
