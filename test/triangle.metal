#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VertexIn {
    float4 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertexShader(VertexIn in [[stage_in]],
                              constant Uniforms &uniforms [[buffer(1)]])
{
    float4 worldPos = uniforms.modelMatrix * in.position;
    float4 viewPos = uniforms.viewMatrix * worldPos;
    float4 clipPos = uniforms.projectionMatrix * viewPos;

    VertexOut out;
    out.position = clipPos;
    out.color = float4(1.0, 0.0, 0.0, 1.0);
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]]) {
    return float4(1.0); // white
}
