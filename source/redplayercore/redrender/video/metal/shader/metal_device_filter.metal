/*
 * metal_device_filter.metal
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct {
    float4 position [[position]];
    float2 texcoord [[user(texturecoord)]];
} ShaderInOut;

/////////////////////////////////////////////////////////////////////////////////////////////compute////////////////////////////////////////////////////////////////////////////////////////////
kernel void computeShader(texture2d<half, access::read>  inTexture   [[ texture(0) ]],
                          texture2d<half, access::write> outTexture  [[ texture(1) ]],
                          uint2                          gid         [[ thread_position_in_grid ]])
{
    half4 inColor  = inTexture.read(gid);
    outTexture.write(inColor, gid);
}
/////////////////////////////////////////////////////////////////////////////////////////////vertex////////////////////////////////////////////////////////////////////////////////////////////



vertex ShaderInOut
redVertexShader(constant float4* pPosition [[ buffer(0) ]],
                constant float2* pTexCoords [[ buffer(1) ]],
                constant float4x4 & mvpMatrix [[ buffer(2) ]],
                uint vertexIndex [[vertex_id]]) {
    ShaderInOut out;

    out.position  = mvpMatrix * pPosition[vertexIndex];
    out.texcoord = pTexCoords[vertexIndex];

    return out;
}
/////////////////////////////////////////////////////////////////////////////////////////////input rgb////////////////////////////////////////////////////////////////////////////////////////////
fragment float4
redRgbFragmentShader(ShaderInOut in [[stage_in]],
                         texture2d<float> us2_Sampler [[ texture(0) ]]) {
    float2 tex_coord = in.texcoord;
    float3 rgb;
    
    constexpr sampler linearSampler (mip_filter::linear,
                                     mag_filter::linear,
                                     min_filter::linear);
    
    const float4 us2_SamplerRGBA = us2_Sampler.sample (linearSampler, tex_coord);
    rgb = us2_SamplerRGBA.rgb;
    return float4(rgb, 1);
}
/////////////////////////////////////////////////////////////////////////////////////////////input yuv////////////////////////////////////////////////////////////////////////////////////////////
fragment float4
redYuv420spFragmentShader(ShaderInOut in [[stage_in]],
                         constant float3x3 & um3_ColorConversion [[ buffer(0) ]],
                         texture2d<float> us2_SamplerX [[ texture(0) ]],
                         texture2d<float> us2_SamplerY [[ texture(1) ]]) {
    float2 tex_coord = in.texcoord;
    
    float3 yuv;
    float3 rgb;
    
    constexpr sampler linearSampler (mip_filter::linear,
                                     mag_filter::linear,
                                     min_filter::linear);
    
    const float4 us2_SamplerXSample = us2_SamplerX.sample (linearSampler, tex_coord);
    yuv.x = us2_SamplerXSample.r - (16.0 / 255.0);
    const float4 us2_SamplerYSample = us2_SamplerY.sample (linearSampler, tex_coord);
    yuv.yz = us2_SamplerYSample.rg - float2(0.5, 0.5);
    
    rgb = um3_ColorConversion * yuv;
    return float4(rgb, 1);
}

fragment float4
redYuv420spFullRangeFragmentShader(ShaderInOut in [[stage_in]],
                         constant float3x3 & um3_ColorConversion [[ buffer(0) ]],
                         texture2d<float> us2_SamplerX [[ texture(0) ]],
                         texture2d<float> us2_SamplerY [[ texture(1) ]]) {
    float2 tex_coord = in.texcoord;
    
    float3 yuv;
    float3 rgb;
    
    constexpr sampler linearSampler (mip_filter::linear,
                                     mag_filter::linear,
                                     min_filter::linear);
    
    const float4 us2_SamplerXSample = us2_SamplerX.sample (linearSampler, tex_coord);
    yuv.x = us2_SamplerXSample.r;
    const float4 us2_SamplerYSample = us2_SamplerY.sample (linearSampler, tex_coord);
    yuv.yz = us2_SamplerYSample.rg - float2(0.5, 0.5);
    
    rgb = um3_ColorConversion * yuv;
    return float4(rgb, 1);
}

fragment float4
redYuv420pFragmentShader(ShaderInOut in [[stage_in]],
                         constant float3x3 & um3_ColorConversion [[ buffer(0) ]],
                         texture2d<float> us2_SamplerX [[ texture(0) ]],
                         texture2d<float> us2_SamplerY [[ texture(1) ]],
                         texture2d<float> us2_SamplerZ [[ texture(2) ]]) {
    float2 tex_coord = in.texcoord;
    
    float3 yuv;
    float3 rgb;
    
    constexpr sampler linearSampler (mip_filter::linear,
                                     mag_filter::linear,
                                     min_filter::linear);
    
    const float4 us2_SamplerXSample = us2_SamplerX.sample (linearSampler, tex_coord);
    yuv.x = us2_SamplerXSample.r - (16.0 / 255.0);
    const float4 us2_SamplerYSample = us2_SamplerY.sample (linearSampler, tex_coord);
    yuv.y = us2_SamplerYSample.r - 0.5;
    const float4 us2_SamplerZSample = us2_SamplerZ.sample (linearSampler, tex_coord);
    yuv.z = us2_SamplerZSample.r - 0.5;
    
    rgb = um3_ColorConversion * yuv;
    return float4(rgb, 1);
}

fragment float4
redYuv420pFullRangeFragmentShader(ShaderInOut in [[stage_in]],
                         constant float3x3 & um3_ColorConversion [[ buffer(0) ]],
                         texture2d<float> us2_SamplerX [[ texture(0) ]],
                         texture2d<float> us2_SamplerY [[ texture(1) ]],
                         texture2d<float> us2_SamplerZ [[ texture(2) ]]) {
    float2 tex_coord = in.texcoord;
    
    float3 yuv;
    float3 rgb;
    
    constexpr sampler linearSampler (mip_filter::linear,
                                     mag_filter::linear,
                                     min_filter::linear);
    
    const float4 us2_SamplerXSample = us2_SamplerX.sample (linearSampler, tex_coord);
    yuv.x = us2_SamplerXSample.r;
    const float4 us2_SamplerYSample = us2_SamplerY.sample (linearSampler, tex_coord);
    yuv.y = us2_SamplerYSample.r - 0.5;
    const float4 us2_SamplerZSample = us2_SamplerZ.sample (linearSampler, tex_coord);
    yuv.z = us2_SamplerZSample.r - 0.5;
    
    rgb = um3_ColorConversion * yuv;
    return float4(rgb, 1);
}
