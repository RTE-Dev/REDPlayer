/*
 * video_macro_definition.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_MACRO_DEFINITION_H
#define VIDEO_MACRO_DEFINITION_H

#include "../redrender_macro_definition.h"
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__) || defined(ANDROID)
#include <android/native_window.h>

#elif defined(PLATFORM_IOS) || defined(__APPLE__)
#import <UIKit/UIKit.h>
#endif

#define STRINGIZE(x) #x
#define SHADER_STRING(text) STRINGIZE(text)

#define ENABLE_OPENGL_CHECK true
///* ErrorCode */
//#define GL_NO_ERROR                                      0
//#define GL_INVALID_ENUM                                  0x0500
//#define GL_INVALID_VALUE                                 0x0501
//#define GL_INVALID_OPERATION                             0x0502
//#define GL_OUT_OF_MEMORY                                 0x0505
#if ENABLE_OPENGL_CHECK
#define CHECK_OPENGL(glFunc)                                                   \
  glFunc;                                                                      \
  {                                                                            \
    int e = glGetError();                                                      \
    if (e != 0) {                                                              \
      AV_LOGE(LOG_TAG, "[%s:%d] OPENGL ERROR code:0x%04X . \n", __FUNCTION__,  \
              __LINE__, e);                                                    \
    }                                                                          \
  }
//    #define CHECK_OPENGL_ERROR_RETURN(glFunc) \
//    glFunc; \
//    { \
//        int e = glGetError(); \
//        if (e != 0) \
//        { \
//            AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] OPENGL ERROR code:0x%04X . \n", __FUNCTION__, __LINE__, e); \
//            return VRErrorContext; \
//        } \
//    } \
//    return VRErrorNone
#else
#define CHECK_OPENGL(glFunc) glFunc;
#endif

NS_REDRENDER_BEGIN
// enum
typedef enum VRError {
  VRErrorNone = 0,
  VRErrorInit,
  VRErrorInitFilter,
  VRErrorWindow,
  VRErrorContext,
  VRErrorUpdateParam,
  VRErrorInputFrame,
  VRErrorFilterCreate,
  VRErrorOnRender,
  VRErrorOnRenderFlter,

  VRErrorEnd,
} VRError;

typedef enum VRPixelFormat {
  VRPixelFormatUnknown = 0,
  VRPixelFormatRGB565,
  VRPixelFormatRGB888,
  VRPixelFormatRGBX8888,
  VRPixelFormatYUV420p,
  VRPixelFormatYUV420sp,
  VRPixelFormatYUV420sp_vtb,
  VRPixelFormatYUV420p10le,
  VRPixelFormatMediaCodec,
  VRPixelFormatHisiMediaCodec,

  VRPixelFormatEnd,
} VRPixelFormat;

/**
 * Chromaticity coordinates of the source primaries.
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.1 and
 * ITU-T H.273.
 */
typedef enum VRAVColorPrimaries {
  VR_AVCOL_PRI_RESERVED0 = 0,
  VR_AVCOL_PRI_BT709 =
      1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP 177 Annex B
  VR_AVCOL_PRI_UNSPECIFIED = 2,
  VR_AVCOL_PRI_RESERVED = 3,
  VR_AVCOL_PRI_BT470M =
      4, ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)

  VR_AVCOL_PRI_BT470BG = 5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 /
                            ///< ITU-R BT1700 625 PAL & SECAM
  VR_AVCOL_PRI_SMPTE170M =
      6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
  VR_AVCOL_PRI_SMPTE240M =
      7, ///< identical to above, also called "SMPTE C" even though it uses D65
  VR_AVCOL_PRI_FILM = 8,      ///< colour filters using Illuminant C
  VR_AVCOL_PRI_BT2020 = 9,    ///< ITU-R BT2020
  VR_AVCOL_PRI_SMPTE428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
  VR_AVCOL_PRI_SMPTEST428_1 = VR_AVCOL_PRI_SMPTE428,
  VR_AVCOL_PRI_SMPTE431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
  VR_AVCOL_PRI_SMPTE432 = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
  VR_AVCOL_PRI_EBU3213 = 22,  ///< EBU Tech. 3213-E (nothing there) / one of
                              ///< JEDEC P22 group phosphors
  VR_AVCOL_PRI_JEDEC_P22 = VR_AVCOL_PRI_EBU3213,
  VR_AVCOL_PRI_NB ///< Not part of ABI
} VRAVColorPrimaries;

/**
 * Color Transfer Characteristic.
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.2.
 */
typedef enum VRAVColorTrC {
  VR_AVCOL_TRC_RESERVED0 = 0,
  VR_AVCOL_TRC_BT709 = 1, ///< also ITU-R BT1361
  VR_AVCOL_TRC_UNSPECIFIED = 2,
  VR_AVCOL_TRC_RESERVED = 3,
  VR_AVCOL_TRC_GAMMA22 =
      4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
  VR_AVCOL_TRC_GAMMA28 = 5,   ///< also ITU-R BT470BG
  VR_AVCOL_TRC_SMPTE170M = 6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358
                              ///< 525 or 625 / ITU-R BT1700 NTSC
  VR_AVCOL_TRC_SMPTE240M = 7,
  VR_AVCOL_TRC_LINEAR = 8, ///< "Linear transfer characteristics"
  VR_AVCOL_TRC_LOG = 9, ///< "Logarithmic transfer characteristic (100:1 range)"
  VR_AVCOL_TRC_LOG_SQRT =
      10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
  VR_AVCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
  VR_AVCOL_TRC_BT1361_ECG = 12,   ///< ITU-R BT1361 Extended Colour Gamut
  VR_AVCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
  VR_AVCOL_TRC_BT2020_10 = 14,    ///< ITU-R BT2020 for 10-bit system
  VR_AVCOL_TRC_BT2020_12 = 15,    ///< ITU-R BT2020 for 12-bit system
  VR_AVCOL_TRC_SMPTE2084 =
      16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
  VR_AVCOL_TRC_SMPTEST2084 = VR_AVCOL_TRC_SMPTE2084,
  VR_AVCOL_TRC_SMPTE428 = 17, ///< SMPTE ST 428-1
  VR_AVCOL_TRC_SMPTEST428_1 = VR_AVCOL_TRC_SMPTE428,
  VR_AVCOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
  VR_AVCOL_TRC_NB                 ///< Not part of ABI
} VRAVColorTrC;

/**
 * YUV colorspace type.
 * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.3.
 */
typedef enum VRAVColorSpace {
  VR_AVCOL_SPC_RGB =
      0, ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
  VR_AVCOL_SPC_BT709 =
      1, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
  VR_AVCOL_SPC_UNSPECIFIED = 2,
  VR_AVCOL_SPC_RESERVED = 3,
  VR_AVCOL_SPC_FCC =
      4, ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
  VR_AVCOL_SPC_BT470BG =
      5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL &
         ///< SECAM / IEC 61966-2-4 xvYCC601
  VR_AVCOL_SPC_SMPTE170M =
      6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
  VR_AVCOL_SPC_SMPTE240M =
      7, ///< derived from 170M primaries and D65 white point, 170M is derived
         ///< from BT470 System M's primaries
  VR_AVCOL_SPC_YCGCO =
      8, ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
  VR_AVCOL_SPC_YCOCG = VR_AVCOL_SPC_YCGCO,
  VR_AVCOL_SPC_BT2020_NCL = 9, ///< ITU-R BT2020 non-constant luminance system
  VR_AVCOL_SPC_BT2020_CL = 10, ///< ITU-R BT2020 constant luminance system
  VR_AVCOL_SPC_SMPTE2085 = 11, ///< SMPTE 2085, Y'D'zD'x
  VR_AVCOL_SPC_CHROMA_DERIVED_NCL =
      12, ///< Chromaticity-derived non-constant luminance system
  VR_AVCOL_SPC_CHROMA_DERIVED_CL =
      13,                  ///< Chromaticity-derived constant luminance system
  VR_AVCOL_SPC_ICTCP = 14, ///< ITU-R BT.2100-0, ICtCp
  VR_AVCOL_SPC_NB          ///< Not part of ABI
} VRAVColorSpace;

/**
 * MPEG vs JPEG YUV range.
 */
typedef enum VRAVColorRange {
  VR_AVCOL_RANGE_UNSPECIFIED = 0,
  VR_AVCOL_RANGE_MPEG = 1, ///< the normal 219*2^(n-8) "MPEG" YUV ranges
  VR_AVCOL_RANGE_JPEG = 2, ///< the normal     2^n-1   "JPEG" YUV ranges
  VR_AVCOL_RANGE_NB        ///< Not part of ABI
} VRAVColorRange;

// All filters supported by opengl, select from here as needed when initializing
// the renderer
typedef enum VideoFilterType {
  VideoFilterTypeUnknown = -1,
  // opengl post-processingFilter,
  VideoOpenGLYUV2RGBFilterType,
  VideoOpenGLBlackAndWhiteFilterType,
  VideoOpenGLMirrorFilterType,
  VideoOpenGLFilterGroupType,
  // opengl final filter,
  VideoOpenGLDeviceFilterType,
  VideoOpenGLFilterTypeEnd,

  // todo metal
  VideoMetalYUV2RGBFilterType,
  VideoMetalBlackAndWhiteFilterType,
  VideoMetalDeviceFilterType,
  VideoMetalSRFilterType,
  VideoMetalFilterTypeEnd,

} VideoFilterType;

typedef enum RotationMode {
  NoRotation = 0,
  RotateLeft,
  RotateRight,
  FlipVertical,
  FlipHorizontal,
  RotateRightFlipVertical,
  RotateRightFlipHorizontal,
  Rotate180,
} RotationMode;

typedef struct SARRational {
  int num; ///< Numerator
  int den; ///< Denominator
} SARRational;

typedef enum GravityResizeAspectRatio {
  AspectRatioEqualWidth, // default mode
  AspectRatioEqualHeight,
  AspectRatioFill,

  ScaleAspectFit,
  ScaleAspectFill,
} GravityResizeAspectRatio;

#define rotationSwapsSize(rotation)                                            \
  ((rotation) == RedRender::RotateLeft ||                                      \
   (rotation) == RedRender::RotateRight ||                                     \
   (rotation) == RedRender::RotateRightFlipVertical ||                         \
   (rotation) == RedRender::RotateRightFlipHorizontal)

// cache post-processed frame queue size
#define VIDEO_TEXTURE_QUEUE_SIZE (3)
// input texture num
#define INPUT_RGBA_TEXTURE_NUM (1)
#define INPUT_YUV420SP_TEXTURE_NUM (2)
#define INPUT_YUV420P_TEXTURE_NUM (3)

NS_REDRENDER_END

#endif /* VIDEO_MACRO_DEFINITION_H */
