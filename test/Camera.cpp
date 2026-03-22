//
//  Camera.cpp
//  test
//
//  Created by Albert Yang on 2026-02-18.
//

#include "Camera.hpp"
#include <simd/simd.h>
#include <cmath>
#include <iostream>

Camera::Camera(float fov, float aspect, float nearPlane, float farPlane)
    : fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane) {
    position = {0, 0, 3};
    forward = {0, 0, -1};
    right = {1, 0, 0};
    up = {0, 1, 0};
}

void Camera::setPosition(simd_float3 pos) {
    position = pos;
}

void Camera::lookAt(simd_float3 target, simd_float3 upVec) {
    forward = simd_normalize(target - position);
    right = simd_normalize(simd_cross(forward, upVec));
    up = simd_normalize(simd_cross(right, forward));
}


// ...existing code...
simd_float4x4 Camera::createPerspective() {
    float yScale = 1.0f / tanf(fov * 0.5f);
    float xScale = yScale / aspect;
    float nf = 1.0f / (farPlane - nearPlane); // note far - near

    simd_float4x4 m;
    m.columns[0] = simd_make_float4(xScale, 0.0f,   0.0f,                               0.0f);
    m.columns[1] = simd_make_float4(0.0f,   yScale, 0.0f,                               0.0f);
    m.columns[2] = simd_make_float4(0.0f,   0.0f,   farPlane * nf,                       1.0f); // z
    m.columns[3] = simd_make_float4(0.0f,   0.0f,  -nearPlane * farPlane * nf,           0.0f);

    return m;
}
// ...existing code...



simd_float4x4 Camera::createLookAt() {
    simd_float4 row0 = {right.x, right.y, right.z, - simd_dot(right, position)};
    simd_float4 row1 = {up.x, up.y, up.z, - simd_dot(up, position)};
    simd_float4 row2 = {-forward.x, -forward.y, -forward.z, simd_dot(forward, position)};
    simd_float4 row3 = {0,0,0,1};
    simd_float4x4 m = simd_matrix_from_rows(row0, row1, row2, row3);

    return m;
}

simd_float4x4 Camera::get_matrix_perspective_right_hand(float fovyRadians,
                                              float aspect,
                                              float nearZ,
                                              float farZ) const
{
    float ys = 1 / tanf(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);
    simd_float4 row0 = {xs, 0, 0, 0};
    simd_float4 row1 = {0, ys, 0, 0};
    simd_float4 row2 = {0, 0, zs, nearZ * zs};
    simd_float4 row3 = {0,0,-1,0};
    return simd_matrix_from_rows(row0,row1,row2, row3);
}


simd_float4x4 Camera::getViewMatrix() const {
    return const_cast<Camera*>(this)->createLookAt();
}

simd_float4x4 Camera::getProjectionMatrix() const {
    return const_cast<Camera*>(this)->createPerspective();
//    return matrix_identity_float4x4;
}
