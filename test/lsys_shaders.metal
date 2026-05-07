// lsys_shaders.metal

#include <metal_stdlib>
using namespace metal;

// ── uniforms (must match C++ Uniforms struct exactly) ────────────────────────

struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

// ── vertex input (matches LineVertex / TriVertex: float3 pos, float4 color) ──

struct VertIn {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct VertOut {
    float4 clip_position [[position]];
    float4 color;
    float  depth;
};

// ─── line shaders ─────────────────────────────────────────────────────────────

vertex VertOut lineVertexShader(
    VertIn             in [[stage_in]],
    constant Uniforms& u  [[buffer(1)]])
{
    float4 worldPos = u.modelMatrix * float4(in.position, 1.0);
    float4 viewPos  = u.viewMatrix  * worldPos;

    VertOut out;
    out.clip_position = u.projectionMatrix * viewPos;
    out.color         = in.color;
    out.depth         = -viewPos.z;
    return out;
}

fragment float4 lineFragmentShader(VertOut in [[stage_in]])
{
    return in.color;
}

// ─── triangle shaders ─────────────────────────────────────────────────────────

vertex VertOut triVertexShader(
    VertIn             in [[stage_in]],
    constant Uniforms& u  [[buffer(1)]])
{
    float4 worldPos = u.modelMatrix * float4(in.position, 1.0);
    float4 viewPos  = u.viewMatrix  * worldPos;

    VertOut out;
    out.clip_position = u.projectionMatrix * viewPos;
    out.color         = in.color;
    out.depth         = -viewPos.z;
    return out;
}

fragment float4 triFragmentShader(VertOut in [[stage_in]])
{
    return in.color;
}

