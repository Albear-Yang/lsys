//
//  Camera.hpp
//  test
//
//  Created by Albert Yang on 2026-02-18.
//

#pragma once
#include <simd/simd.h>

class Camera {
public:
    Camera(float fov, float aspect, float nearPlane, float farPlane);
    
    // Set camera position and orientation
    void setPosition(simd_float3 position);
    void lookAt(simd_float3 target, simd_float3 up);
    
    // Get matrices for shader
    simd_float4x4 getViewMatrix() const;
    simd_float4x4 getProjectionMatrix() const;
    simd_float4x4 get_matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ) const;
    
private:
    simd_float3 position;
    simd_float3 forward;
    simd_float3 right;
    simd_float3 up;
    
    float fov;
    float aspect;
    float nearPlane;
    float farPlane;
    
    simd_float4x4 createPerspective();
    simd_float4x4 createLookAt();
};
