#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS

#import "metal_common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

static const matrix_float4x4 ModelViewProjectionMatrix = (matrix_float4x4){
    (vector_float4){1.0f, 0.0f, 0.0f, 0.0f},
    (vector_float4){0.0f, 1.0f, 0.0f, 0.0f},
    (vector_float4){0.0f, 0.0f, 1.0f, 0.0f},
    (vector_float4){0.0f, 0.0f, 0.0f, 1.0f},
};

////////////////////////////////////////////////////////////////////////////////////////////////////

static const matrix_float3x3 Bt601VideoRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.164384, 1.164384, 1.164384},
    (vector_float3){0.0, -0.391762, 2.017232},
    (vector_float3){1.596027, -0.812968, 0.0},
};

static const matrix_float3x3 Bt601FullRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.0, 1.0, 1.0},
    (vector_float3){0.0, -0.344136, 1.772},
    (vector_float3){1.402, -0.714136, 0.0},
};

static const matrix_float3x3 Bt709VideoRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.164384, 1.164384, 1.164384},
    (vector_float3){0.0, -0.213249, 2.112402},
    (vector_float3){1.792741, -0.532909, 0.0},
};

static const matrix_float3x3 Bt709FullRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.0, 1.0, 1.0},
    (vector_float3){0.0, -0.187324, 1.8556},
    (vector_float3){1.5748, -0.468124, 0.0},
};

static const matrix_float3x3 Bt2020VideoRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.164384, 1.164384, 1.164384},
    (vector_float3){0.0, -0.187326, 2.141772},
    (vector_float3){1.678674, -0.650424, 0.0},
};

static const matrix_float3x3 Bt2020FullRangeYUV2RGBMatrix = (matrix_float3x3){
    (vector_float3){1.0, 1.0, 1.0},
    (vector_float3){0.0, -0.164553, 1.8814},
    (vector_float3){1.4746, -0.571353, 0.0},
};

////////////////////////////////////////////////////////////////////////////////////////////////////
const matrix_float4x4 getModelViewProjectionMatrix() {
  return ModelViewProjectionMatrix;
}
const matrix_float3x3 getBt601VideoRangeYUV2RGBMatrix() {
  return Bt601VideoRangeYUV2RGBMatrix;
}
const matrix_float3x3 getBt601FullRangeYUV2RGBMatrix() {
  return Bt601FullRangeYUV2RGBMatrix;
}
const matrix_float3x3 getBt709VideoRangeYUV2RGBMatrix() {
  return Bt709VideoRangeYUV2RGBMatrix;
}
const matrix_float3x3 getBt709FullRangeYUV2RGBMatrix() {
  return Bt709FullRangeYUV2RGBMatrix;
}
const matrix_float3x3 getBt2020VideoRangeYUV2RGBMatrix() {
  return Bt2020VideoRangeYUV2RGBMatrix;
}
const matrix_float3x3 getBt2020FullRangeYUV2RGBMatrix() {
  return Bt2020FullRangeYUV2RGBMatrix;
}

#endif
