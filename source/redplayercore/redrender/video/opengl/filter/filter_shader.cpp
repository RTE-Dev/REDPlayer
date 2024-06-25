/*
 * filter_shader.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./filter_shader.h"

NS_REDRENDER_BEGIN
static const std::string OpenGLDeviceFilterFragmentShaderString =
    SHADER_STRING(precision highp float; varying highp vec2 vTexCoord;
                  uniform lowp sampler2D colorMap;

                  void main() {
                    gl_FragColor =
                        vec4(texture2D(colorMap, vTexCoord).rgb, 1.0);
                  });

static const std::string OpenGLDeviceFilterVertexShaderString =
    SHADER_STRING(precision highp float; attribute highp vec4 position;
                  attribute highp vec4 texCoord; varying highp vec2 vTexCoord;
                  uniform mat4 um4_mvp;

                  void main() {
                    gl_Position = um4_mvp * position;
                    vTexCoord = texCoord.xy;
                  });

static const std::string YUV420pVideoRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; varying highp vec2 vTexCoord2;
        uniform mat3 um3_ColorConversion; uniform lowp sampler2D colorMap;
        uniform lowp sampler2D colorMap1; uniform lowp sampler2D colorMap2;

        void main() {
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv.x = (texture2D(colorMap, vTexCoord).r - (16.0 / 255.0));
          yuv.y = (texture2D(colorMap1, vTexCoord1).r - 0.5);
          yuv.z = (texture2D(colorMap2, vTexCoord2).r - 0.5);
          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

static const std::string YUV420pFullRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; varying highp vec2 vTexCoord2;
        uniform mat3 um3_ColorConversion; uniform lowp sampler2D colorMap;
        uniform lowp sampler2D colorMap1; uniform lowp sampler2D colorMap2;

        void main() {
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv.x = (texture2D(colorMap, vTexCoord).r);
          yuv.y = (texture2D(colorMap1, vTexCoord1).r - 0.5);
          yuv.z = (texture2D(colorMap2, vTexCoord2).r - 0.5);
          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

static const std::string YUV420spVideoRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; uniform mat3 um3_ColorConversion;
        uniform lowp sampler2D colorMap; uniform lowp sampler2D colorMap1;

        void main() {
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv.x = (texture2D(colorMap, vTexCoord).r - (16.0 / 255.0));
          yuv.yz = (texture2D(colorMap1, vTexCoord1).rg - vec2(0.5, 0.5));
          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

static const std::string YUV420spFullRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; uniform mat3 um3_ColorConversion;
        uniform lowp sampler2D colorMap; uniform lowp sampler2D colorMap1;

        void main() {
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv.x = (texture2D(colorMap, vTexCoord).r);
          yuv.yz = (texture2D(colorMap1, vTexCoord1).rg - vec2(0.5, 0.5));
          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

static const std::string YUV444p10leVideoRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; varying highp vec2 vTexCoord2;
        uniform mat3 um3_ColorConversion; uniform lowp sampler2D colorMap;
        uniform lowp sampler2D colorMap1; uniform lowp sampler2D colorMap2;

        void main() {
          mediump vec3 yuv_l;
          mediump vec3 yuv_h;
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv_l.x = texture2D(colorMap, vTexCoord).r;
          yuv_h.x = texture2D(colorMap, vTexCoord).a;
          yuv_l.y = texture2D(colorMap1, vTexCoord1).r;
          yuv_h.y = texture2D(colorMap1, vTexCoord1).a;
          yuv_l.z = texture2D(colorMap2, vTexCoord2).r;
          yuv_h.z = texture2D(colorMap2, vTexCoord2).a;

          yuv = (yuv_l * 255.0 + yuv_h * 255.0 * 256.0) / (1023.0) -
                vec3(16.0 / 255.0, 0.5, 0.5);

          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

static const std::string YUV444p10leFullRange2RGBFilterFragmentShaderString =
    SHADER_STRING(
        precision highp float; varying highp vec2 vTexCoord;
        varying highp vec2 vTexCoord1; varying highp vec2 vTexCoord2;
        uniform mat3 um3_ColorConversion; uniform lowp sampler2D colorMap;
        uniform lowp sampler2D colorMap1; uniform lowp sampler2D colorMap2;

        void main() {
          mediump vec3 yuv_l;
          mediump vec3 yuv_h;
          mediump vec3 yuv;
          lowp vec3 rgb;

          yuv_l.x = texture2D(colorMap, vTexCoord).r;
          yuv_h.x = texture2D(colorMap, vTexCoord).a;
          yuv_l.y = texture2D(colorMap1, vTexCoord1).r;
          yuv_h.y = texture2D(colorMap1, vTexCoord1).a;
          yuv_l.z = texture2D(colorMap2, vTexCoord2).r;
          yuv_h.z = texture2D(colorMap2, vTexCoord2).a;

          yuv = (yuv_l * 255.0 + yuv_h * 255.0 * 256.0) / (1023.0) -
                vec3(0.0, 0.5, 0.5);

          rgb = um3_ColorConversion * yuv;
          gl_FragColor = vec4(rgb, 1);
        });

////////////////////////////////////////////////////////////////////////////////////////////////////

static const GLfloat Bt601VideoRangeYUV2RGBMatrix[] = {
    1.164384, 1.164384, 1.164384,  0.0, -0.391762,
    2.017232, 1.596027, -0.812968, 0.0,
};

static const GLfloat Bt601FullRangeYUV2RGBMatrix[] = {
    1.0, 1.0, 1.0, 0.0, -0.344136, 1.772, 1.402, -0.714136, 0.0,
};

static const GLfloat Bt709VideoRangeYUV2RGBMatrix[] = {
    1.164384, 1.164384, 1.164384,  0.0, -0.213249,
    2.112402, 1.792741, -0.532909, 0.0,
};

static const GLfloat Bt709FullRangeYUV2RGBMatrix[] = {
    1.0, 1.0, 1.0, 0.0, -0.187324, 1.8556, 1.5748, -0.468124, 0.0,
};

static const GLfloat Bt2020VideoRangeYUV2RGBMatrix[] = {
    1.164384, 1.164384, 1.164384,  0.0, -0.187326,
    2.141772, 1.678674, -0.650424, 0.0,
};

static const GLfloat Bt2020FullRangeYUV2RGBMatrix[] = {
    1.0, 1.0, 1.0, 0.0, -0.164553, 1.8814, 1.4746, -0.571353, 0.0,
};

static const GLfloat OpenGLModelViewProjectionMatrix[] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string getOpenGLDeviceFilterFragmentShaderString() {
  return OpenGLDeviceFilterFragmentShaderString;
}

const std::string getOpenGLDeviceFilterVertexShaderString() {
  return OpenGLDeviceFilterVertexShaderString;
}

const std::string getYUV420pVideoRange2RGBFilterFragmentShaderString() {
  return YUV420pVideoRange2RGBFilterFragmentShaderString;
}
const std::string getYUV420pFullRange2RGBFilterFragmentShaderString() {
  return YUV420pFullRange2RGBFilterFragmentShaderString;
}
const std::string getYUV420spVideoRange2RGBFilterFragmentShaderString() {
  return YUV420spVideoRange2RGBFilterFragmentShaderString;
}
const std::string getYUV420spFullRange2RGBFilterFragmentShaderString() {
  return YUV420spFullRange2RGBFilterFragmentShaderString;
}
const std::string getYUV444p10leVideoRange2RGBFilterFragmentShaderString() {
  return YUV444p10leVideoRange2RGBFilterFragmentShaderString;
}
const std::string getYUV444p10leFullRange2RGBFilterFragmentShaderString() {
  return YUV444p10leFullRange2RGBFilterFragmentShaderString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const GLfloat *getBt601VideoRangeYUV2RGBMatrix() {
  return Bt601VideoRangeYUV2RGBMatrix;
}
const GLfloat *getBt601FullRangeYUV2RGBMatrix() {
  return Bt601FullRangeYUV2RGBMatrix;
}
const GLfloat *getBt709VideoRangeYUV2RGBMatrix() {
  return Bt709VideoRangeYUV2RGBMatrix;
}
const GLfloat *getBt709FullRangeYUV2RGBMatrix() {
  return Bt709FullRangeYUV2RGBMatrix;
}
const GLfloat *getBt2020VideoRangeYUV2RGBMatrix() {
  return Bt2020VideoRangeYUV2RGBMatrix;
}
const GLfloat *getBt2020FullRangeYUV2RGBMatrix() {
  return Bt2020FullRangeYUV2RGBMatrix;
}

const GLfloat *getOpenGLModelViewProjectionMatrix() {
  return OpenGLModelViewProjectionMatrix;
}

NS_REDRENDER_END
