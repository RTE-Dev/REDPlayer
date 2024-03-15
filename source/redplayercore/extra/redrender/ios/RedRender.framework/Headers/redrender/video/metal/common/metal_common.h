/*
 * metal_common.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "video_inc_internal.h"
#import <Foundation/Foundation.h>
#import <simd/simd.h>

const matrix_float4x4 getModelViewProjectionMatrix();
const matrix_float3x3 getBt601VideoRangeYUV2RGBMatrix();
const matrix_float3x3 getBt601FullRangeYUV2RGBMatrix();
const matrix_float3x3 getBt709VideoRangeYUV2RGBMatrix();
const matrix_float3x3 getBt709FullRangeYUV2RGBMatrix();
const matrix_float3x3 getBt2020VideoRangeYUV2RGBMatrix();
const matrix_float3x3 getBt2020FullRangeYUV2RGBMatrix();

#endif
