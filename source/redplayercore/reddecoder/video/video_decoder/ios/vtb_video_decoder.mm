#include "reddecoder/video/video_decoder/ios/vtb_video_decoder.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "reddecoder/common/logger.h"
#include "reddecoder/video/video_common/h264_parser.h"
#include "reddecoder/video/video_decoder/ios/cf_obj_helper.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avc.h"
#include "libavutil/avutil.h"
}

#define AV_RB24(x)                                                             \
  ((((const uint8_t *)(x))[0] << 16) | (((const uint8_t *)(x))[1] << 8) |      \
   ((const uint8_t *)(x))[2])

namespace reddecoder {

static void vt_decode_callback(void *decoder_ref, void *, OSStatus status,
                               VTDecodeInfoFlags info_flags,
                               CVImageBufferRef image_buffer, CMTime pts,
                               CMTime duration) {
  @autoreleasepool {
    VtbVideoDecoder *decoder = reinterpret_cast<VtbVideoDecoder *>(decoder_ref);
    if (!decoder) {
      return;
    }
    if (status != noErr) {
      if (status == kVTVideoDecoderBadDataErr) {
        decoder->get_decode_complete_callback()->on_decode_error(
            VideoCodecError::kBadData);
      } else if (status == kVTInvalidSessionErr) {
        decoder->get_decode_complete_callback()->on_decode_error(
            VideoCodecError::kNeedIFrame);
      } else if (status == kVTVideoDecoderReferenceMissingErr) {
        decoder->get_decode_complete_callback()->on_decode_error(
            VideoCodecError::kDecoderErrorStatus);
      } else {
        decoder->get_decode_complete_callback()->on_decode_error(
            VideoCodecError::kInternalError, (int)status);
      }
      return;
    }

    if (image_buffer == NULL) {
      return;
    }

    if (kVTDecodeInfo_FrameDropped & info_flags) {
      return;
    }

    OSType format_type = CVPixelBufferGetPixelFormatType(image_buffer);
    if (format_type != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange &&
        format_type != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange &&
        format_type != kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange) {
      AV_LOGE(DEC_TAG, "[reddecoder] videotoolbox format error %u\n",
              format_type);
      decoder->get_decode_complete_callback()->on_decode_error(
          VideoCodecError::kNotSupported);
      return;
    }

    int64_t pts_ms = decoder->get_sample_info().pts_ms;

    std::unique_lock<std::mutex> lck(decoder->buffer_queue_mutex_);
    BufferQueue &buffer_queue = decoder->get_buffer_queue();
    if (buffer_queue.size()) {
      int64_t min_pts = buffer_queue.top()->get_video_frame_meta()->pts_ms;
      if (pts_ms < min_pts) {
        decoder->continue_pts_reorder_error_++;
        if (decoder->continue_pts_reorder_error_ == 20) { // report once
          decoder->get_decode_complete_callback()->on_decode_error(
              VideoCodecError::kPTSReorderError);
        }
        return;
      }
    }
    decoder->continue_pts_reorder_error_ = 0;

    Buffer *buffer = new Buffer(BufferType::kVideoFrame, 0);
    VideoFrameMeta *meta = buffer->get_video_frame_meta();
    if (CVPixelBufferIsPlanar(image_buffer)) {
      meta->height = CVPixelBufferGetHeightOfPlane(image_buffer, 0);
      meta->width = CVPixelBufferGetWidthOfPlane(image_buffer, 0);
    } else {
      meta->height = CVPixelBufferGetHeight(image_buffer);
      meta->width = CVPixelBufferGetWidth(image_buffer);
    }
    meta->pixel_format = PixelFormat::kVTBBuffer;
    meta->pts_ms = pts_ms;
    meta->dts_ms = decoder->get_sample_info().dts_ms;
    // need to be released by observer
    meta->buffer_context = (void *)new
    VideoToolBufferContext{buffer : CVBufferRetain(image_buffer)};

    buffer_queue.push(buffer);
    int size = buffer_queue.size();
    if (buffer_queue.size() > decoder->get_max_ref_frames()) {
      Buffer *temp_buffer = buffer_queue.top();
      std::unique_ptr<Buffer> out_buffer;
      out_buffer.reset(temp_buffer);
      decoder->get_decode_complete_callback()->on_decoded_frame(
          std::move(out_buffer));
      buffer_queue.pop();
    }
  }
}

VtbVideoDecoder::VtbVideoDecoder(VideoCodecInfo codec) : VideoDecoder(codec) {}

VtbVideoDecoder::~VtbVideoDecoder() { release(); }

VideoCodecError VtbVideoDecoder::init(const Buffer *buffer) {
  if ((codec_info_.codec_name != VideoCodecName::kH264 &&
       codec_info_.codec_name != VideoCodecName::kH265) ||
      codec_info_.implementation_type !=
          VideoCodecImplementationType::kHardwareiOS) {
    return VideoCodecError::kInitError;
  }

  if (codec_info_.codec_name == VideoCodecName::kH265) {
    bool is_hevc_support = false;
    if (@available(iOS 11.0, *)) {
      is_hevc_support = VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC);
    }
    if (!is_hevc_support) {
      return VideoCodecError::kNotSupported;
    }
  }

  release();
  if (buffer) {
    init_cm_format_desc(buffer);
    init_vt_decompress_session();
  }
  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::release() {
  release_vt_decompress_seesion();
  release_cm_format_desc();
  std::unique_lock<std::mutex> lock(buffer_queue_mutex_);
  while (!buffer_queue_.empty()) {
    Buffer *temp_buffer = buffer_queue_.top();
    buffer_queue_.pop();
    VideoFrameMeta *meta = temp_buffer->get_video_frame_meta();
    if (meta && meta->buffer_context) {
      VideoToolBufferContext *temp_buffer_ctx =
          (VideoToolBufferContext *)meta->buffer_context;
      if (temp_buffer_ctx) {
        CVBufferRelease(
            (CVBufferRef)temp_buffer_ctx->buffer); // todo, check android
        delete temp_buffer_ctx;
      }
    }

    delete temp_buffer;
  }
  return VideoCodecError::kNoError;
}

CMSampleBufferRef VtbVideoDecoder::create_cm_sample_buffer(void *data,
                                                           size_t size) {
  OSStatus status;
  CMBlockBufferRef block_buffer = nullptr;
  CMSampleBufferRef sample_buffer = nullptr;

  status = CMBlockBufferCreateWithMemoryBlock(nullptr, data, size,
                                              kCFAllocatorNull, nullptr, 0,
                                              size, FALSE, &block_buffer);

  if (!status) {
    status =
        CMSampleBufferCreate(nullptr, block_buffer, TRUE, 0, 0, cm_format_desc_,
                             1, 0, nullptr, 0, nullptr, &sample_buffer);
  }

  if (block_buffer) {
    CFRelease(block_buffer);
  }

  if (status == noErr) {
    return sample_buffer;
  }

  return nullptr;
}

VideoCodecError VtbVideoDecoder::decode(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoPacket) {
    return VideoCodecError::kInvalidParameter;
  }

  if (!cm_format_desc_) {
    return VideoCodecError::kInitError;
  }

  int convert_size = 0;
  uint8_t *convert_buff = nullptr;
  AVIOContext *pb = nullptr;
  if (buffer->get_video_packet_meta()->format == VideoPacketFormat::kAnnexb ||
      (buffer->get_video_packet_meta()->format ==
           VideoPacketFormat::kFollowExtradataFormat &&
       codec_ctx_.is_annexb_extradata)) {
    if (avio_open_dyn_buf(&pb) < 0) {
      return VideoCodecError::kAllocateBufferError;
    }
    ff_avc_parse_nal_units(pb, buffer->get_data(), buffer->get_size());
    convert_size =
        avio_close_dyn_buf(pb, &convert_buff); // has to av_free this data
    if (convert_size == 0) {
      return VideoCodecError::kAllocateBufferError;
    }
  } else if (codec_ctx_.nal_size == 3) {
    if (avio_open_dyn_buf(&pb) < 0) {
      return VideoCodecError::kAllocateBufferError;
    }

    uint32_t nal_size;
    uint8_t *end = buffer->get_data() + buffer->get_size();
    uint8_t *nal_start = buffer->get_data();
    while (nal_start < end) {
      nal_size = AV_RB24(nal_start);
      avio_wb32(pb, nal_size);
      nal_start += 3;
      avio_write(pb, nal_start, nal_size);
      nal_start += nal_size;
    }
    convert_size =
        avio_close_dyn_buf(pb, &convert_buff); // has to av_free this data
    if (convert_size == 0) {
      return VideoCodecError::kAllocateBufferError;
    }
  }

  CMSampleBufferRef sample_buffer = nullptr;
  if (convert_size > 0) {
    sample_buffer = create_cm_sample_buffer(convert_buff, convert_size);
  } else {
    sample_buffer =
        create_cm_sample_buffer(buffer->get_data(), buffer->get_size());
  }

  if (!sample_buffer) {
    return VideoCodecError::kAllocateBufferError;
  }

  uint32_t decoder_flags = 0;

  VideoPacketMeta *meta = buffer->get_video_packet_meta();
  if (meta->decode_flags & uint32_t(DecodeFlag::kDoNotOutputFrame)) {
    decoder_flags |= kVTDecodeFrame_DoNotOutputFrame;
  }
  sample_info_.dts_ms = meta->dts_ms;
  sample_info_.pts_ms = meta->pts_ms;
  if (sample_info_.pts_ms == AV_NOPTS_VALUE) {
    sample_info_.pts_ms = sample_info_.dts_ms;
  }
  OSStatus status = VTDecompressionSessionDecodeFrame(
      vt_decompress_session_, sample_buffer, decoder_flags, nullptr, nullptr);

  if (convert_size > 0) {
    av_free(convert_buff);
  }

  if (sample_buffer) {
    CFRelease(sample_buffer);
  }

  if (status != noErr) {
    if (status == kVTInvalidSessionErr ||
        status == kVTVideoDecoderMalfunctionErr) {
      return VideoCodecError::kNeedIFrame;
    } else if (status == kVTVideoDecoderBadDataErr) {
      return VideoCodecError::kBadData;
    } else if (status == kVTVideoDecoderReferenceMissingErr) {
      return VideoCodecError::kDecoderErrorStatus;
    } else {
      return VideoCodecError::kInternalError;
    }
  }

  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::init_vt_decompress_session() {
  release_vt_decompress_seesion();

  // need sps/pps info to create cm_format_desc_ first
  if (!cm_format_desc_) {
    return VideoCodecError::kNoError;
  }

  static size_t const num_pixel_atrributes = 4;

  CFTypeRef pixel_attibutes_keys[num_pixel_atrributes]{
      kCVPixelBufferPixelFormatTypeKey, kCVPixelBufferOpenGLESCompatibilityKey,
      kCVPixelBufferWidthKey, kCVPixelBufferHeightKey};

  int width = codec_ctx_.width;
  int height = codec_ctx_.height;
  int max_frame_width = 4096;
  if (width > max_frame_width) {
    double w_scaler = (float)max_frame_width / width;
    width = max_frame_width;
    height = height * w_scaler;
  }

  OSType pixel_format_value = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  if (codec_ctx_.color_range == VideoColorRange::kJPEG) {
    pixel_format_value = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
  }

  if (codec_ctx_.is_hdr) {
    pixel_format_value = kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
  }
  CFNumberRef pixel_format =
      CFNumberCreate(nullptr, kCFNumberSInt32Type, &pixel_format_value);

  CFNumberRef width_key = CFNumberCreate(nullptr, kCFNumberSInt32Type, &width);
  CFNumberRef height_key =
      CFNumberCreate(nullptr, kCFNumberSInt32Type, &height);

  CFTypeRef pixel_attibutes_values[num_pixel_atrributes] = {
      pixel_format, kCFBooleanTrue, width_key, height_key};

  CFDictionaryRef pixel_buffer_attributes = CFDictionaryCreate(
      kCFAllocatorDefault, pixel_attibutes_keys, pixel_attibutes_values,
      num_pixel_atrributes, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

  VTDecompressionOutputCallbackRecord output_callback{
      vt_decode_callback,
      reinterpret_cast<void *>(this),
  };

  OSStatus status = VTDecompressionSessionCreate(
      kCFAllocatorDefault, cm_format_desc_, nullptr, pixel_buffer_attributes,
      &output_callback, &vt_decompress_session_);

  CFRelease(width_key);
  CFRelease(height_key);
  CFRelease(pixel_format);
  CFRelease(pixel_buffer_attributes);

  if (status != noErr || vt_decompress_session_ == nullptr) {
    release_vt_decompress_seesion();
    return VideoCodecError::kInitError;
  }

  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::release_vt_decompress_seesion() {
  if (vt_decompress_session_) {
    VTDecompressionSessionInvalidate(vt_decompress_session_);
    CFRelease(vt_decompress_session_);
    vt_decompress_session_ = nullptr;
  }
  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::init_cm_format_desc_color_space_info(
    CFMutableDictionaryRef extensions, VideoFormatDescMeta *meta) {
  if (codec_ctx_.is_hdr) {
    if (meta->color_primaries == VideoColorPrimaries::kBT2020) {
      CFDictionarySetValue(extensions,
                           kCMFormatDescriptionExtension_ColorPrimaries,
                           kCMFormatDescriptionColorPrimaries_ITU_R_2020);
    }
    if (meta->colorspace == VideoColorSpace::kBT2020_CL ||
        meta->colorspace == VideoColorSpace::kBT2020_NCL) {
      CFDictionarySetValue(extensions,
                           kCMFormatDescriptionExtension_YCbCrMatrix,
                           kCMFormatDescriptionYCbCrMatrix_ITU_R_2020);
    }
    if (@available(iOS 11.0, *)) {
      if (meta->color_trc == VideoColorTransferCharacteristic::kSMPTEST2084) {
        CFDictionarySetValue(
            extensions, kCMFormatDescriptionExtension_TransferFunction,
            kCMFormatDescriptionTransferFunction_SMPTE_ST_2084_PQ);
      }
      if (meta->color_trc == VideoColorTransferCharacteristic::kARIB_STD_B67) {
        CFDictionarySetValue(
            extensions, kCMFormatDescriptionExtension_TransferFunction,
            kCMFormatDescriptionTransferFunction_ITU_R_2100_HLG);
      }
    }
  }
  if (meta->color_primaries == VideoColorPrimaries::kBT709) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_ColorPrimaries,
                         kCMFormatDescriptionColorPrimaries_ITU_R_709_2);
  } else if (meta->color_primaries == VideoColorPrimaries::kSMPTE170M ||
             meta->color_primaries == VideoColorPrimaries::kSMPTE240M) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_ColorPrimaries,
                         kCMFormatDescriptionColorPrimaries_SMPTE_C);
  } else if (meta->color_primaries == VideoColorPrimaries::kBT470BG) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_ColorPrimaries,
                         kCMFormatDescriptionColorPrimaries_EBU_3213);
  } else if (meta->color_primaries == VideoColorPrimaries::kSMPTE432) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_ColorPrimaries,
                         kCMFormatDescriptionColorPrimaries_P3_D65);
  }

  if (meta->color_trc == VideoColorTransferCharacteristic::kBT709 ||
      meta->color_trc == VideoColorTransferCharacteristic::kBT2020_10 ||
      meta->color_trc == VideoColorTransferCharacteristic::kBT2020_12) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_TransferFunction,
                         kCMFormatDescriptionTransferFunction_ITU_R_709_2);
  } else if (meta->color_trc == VideoColorTransferCharacteristic::kSMPTE170M ||
             meta->color_trc == VideoColorTransferCharacteristic::kSMPTE240M) {
    CFDictionarySetValue(extensions,
                         kCMFormatDescriptionExtension_TransferFunction,
                         kCMFormatDescriptionTransferFunction_SMPTE_240M_1995);
  }

  if (meta->colorspace == VideoColorSpace::kBT709) {
    CFDictionarySetValue(extensions, kCMFormatDescriptionExtension_YCbCrMatrix,
                         kCMFormatDescriptionYCbCrMatrix_ITU_R_709_2);
  } else if (meta->colorspace == VideoColorSpace::kSMPTE170M ||
             meta->colorspace == VideoColorSpace::kBT470BG) {
    CFDictionarySetValue(extensions, kCMFormatDescriptionExtension_YCbCrMatrix,
                         kCMFormatDescriptionYCbCrMatrix_ITU_R_601_4);
  } else if (meta->colorspace == VideoColorSpace::kSMPTE240M) {
    CFDictionarySetValue(extensions, kCMFormatDescriptionExtension_YCbCrMatrix,
                         kCMFormatDescriptionYCbCrMatrix_SMPTE_240M_1995);
  }

  if (meta->color_range == VideoColorRange::kJPEG) {
    CFObjHelper::dict_set_boolean(
        extensions, kCMFormatDescriptionExtension_FullRangeVideo, true);
  }
  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::init_cm_format_desc(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoFormatDesc) {
    return VideoCodecError::kInvalidParameter;
  }

  VideoFormatDescMeta *meta = buffer->get_video_format_desc_meta();
  if (!meta || meta->height <= 0 || meta->width <= 0) {
    return VideoCodecError::kInvalidParameter;
  }

  codec_ctx_.is_hdr = meta->is_hdr;
  codec_ctx_.color_range = meta->color_range;
  codec_ctx_.height = meta->height;
  codec_ctx_.width = meta->width;
  codec_ctx_.sample_aspect_ratio.den = meta->sample_aspect_ratio.den;
  codec_ctx_.sample_aspect_ratio.num = meta->sample_aspect_ratio.num;

  CFMutableDictionaryRef par =
      CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                &kCFTypeDictionaryValueCallBacks);

  CFMutableDictionaryRef atoms =
      CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                &kCFTypeDictionaryValueCallBacks);
  CFMutableDictionaryRef extensions =
      CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                &kCFTypeDictionaryValueCallBacks);
  uint8_t *extradata = buffer->get_data();
  size_t extradata_size = buffer->get_size();

  if (extradata_size < 7 || !extradata) {
    return VideoCodecError::kInvalidParameter;
  }

  codec_ctx_.is_annexb_extradata = false;
  codec_ctx_.nal_size = 4;
  if (extradata[0] == 1) {
    if (codec_info_.codec_name == VideoCodecName::kH264 &&
        extradata[4] == 0xFE) {
      extradata[4] = 0xFF;
      codec_ctx_.nal_size = 3;
    } else if (codec_info_.codec_name == VideoCodecName::kH265) {
      if (extradata_size < 22) {
        return VideoCodecError::kInvalidParameter;
      }
      if (extradata[21] == 0xFE) {
        extradata[21] = 0xFF;
        codec_ctx_.nal_size = 3;
      }
    }
  } else {
    if ((extradata[0] == 0 && extradata[1] == 0 && extradata[2] == 0 &&
         extradata[3] == 1) ||
        (extradata[0] == 0 && extradata[1] == 0 && extradata[2] == 1)) {
      AVIOContext *pb;
      if (avio_open_dyn_buf(&pb) < 0) {
        return VideoCodecError::kAllocateBufferError;
      }

      codec_ctx_.is_annexb_extradata = true;
      if (codec_info_.codec_name == VideoCodecName::kH264) {
        ff_isom_write_avcc(pb, extradata, extradata_size);
      }
      extradata = NULL;
      extradata_size = avio_close_dyn_buf(pb, &extradata);
    }
  }

  if (codec_ctx_.sample_aspect_ratio.den > 0 &&
      codec_ctx_.sample_aspect_ratio.num > 0) {
    CFObjHelper::dict_set_i32(par, CFSTR("HorizontalSpacing"),
                              codec_ctx_.sample_aspect_ratio.num);
    CFObjHelper::dict_set_i32(par, CFSTR("VerticalSpacing"),
                              codec_ctx_.sample_aspect_ratio.den);
  }

  CMVideoCodecType cm_codec_type;
  switch (codec_info_.codec_name) {
  case VideoCodecName::kH264:
    cm_codec_type = kCMVideoCodecType_H264;
    CFObjHelper::dict_set_data(atoms, CFSTR("avcC"), extradata, extradata_size);
    break;
  case VideoCodecName::kH265:
    cm_codec_type = kCMVideoCodecType_HEVC;
    CFObjHelper::dict_set_data(atoms, CFSTR("hvcC"), extradata, extradata_size);
    break;
  default:
    break;
  }

  /* Extensions dict */
  CFObjHelper::dict_set_string(
      extensions, CFSTR("CVImageBufferChromaLocationBottomField"), "left");
  CFObjHelper::dict_set_string(
      extensions, CFSTR("CVImageBufferChromaLocationTopField"), "left");
  if (codec_ctx_.color_range == VideoColorRange::kJPEG) {
    CFObjHelper::dict_set_boolean(extensions, CFSTR("FullRangeVideo"), TRUE);
  } else {
    CFObjHelper::dict_set_boolean(extensions, CFSTR("FullRangeVideo"), FALSE);
  }
  CFObjHelper::dict_set_object(extensions, CFSTR("CVPixelAspectRatio"),
                               (CFTypeRef *)par);
  CFObjHelper::dict_set_object(
      extensions, CFSTR("SampleDescriptionExtensionAtoms"), (CFTypeRef *)atoms);

  init_cm_format_desc_color_space_info(extensions, meta);

  release_cm_format_desc();
  OSStatus status = CMVideoFormatDescriptionCreate(
      NULL, cm_codec_type, meta->width, meta->height, extensions,
      &cm_format_desc_);

  VideoCodecError ret = VideoCodecError::kNoError;
  if (status != noErr || cm_format_desc_ == nullptr) {
    ret = VideoCodecError::kInitError;
  }

  max_ref_frames_ = 5;
  if (codec_info_.codec_name == VideoCodecName::kH264) {
    auto h264_info = H264FormatParser::get_h264_info_from_extradata(
        extradata, extradata_size);
    if (h264_info->is_interlaced) {
      ret = VideoCodecError::kNotSupported;
    }
    if (h264_info->profile == FF_PROFILE_H264_MAIN && h264_info->level == 32 &&
        h264_info->max_ref_frames > 4) {
      AV_LOGE(DEC_TAG,
              "[reddecoder]  Main@L3.2 detected, VTB cannot decode with %d "
              "ref frames\n",
              h264_info->max_ref_frames);
      ret = VideoCodecError::kNotSupported;
    }
    max_ref_frames_ = h264_info->max_ref_frames;
    AV_LOGI(DEC_TAG, "[reddecoder] videotoolbox max_ref_frame is %d\n",
            max_ref_frames_);
  }

  max_ref_frames_ = std::max(max_ref_frames_, 5);
  max_ref_frames_ = std::min(max_ref_frames_, 16);
  CFRelease(extensions);
  CFRelease(atoms);
  CFRelease(par);
  if (codec_ctx_.is_annexb_extradata) {
    av_free(extradata);
  }

  return ret;
}

VideoCodecError VtbVideoDecoder::release_cm_format_desc() {
  if (cm_format_desc_) {
    CFRelease(cm_format_desc_);
  }
  cm_format_desc_ = nullptr;
  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::get_delayed_frames() {
  if (vt_decompress_session_) {
    VTDecompressionSessionWaitForAsynchronousFrames(vt_decompress_session_);
  }
  std::unique_lock<std::mutex> lock(buffer_queue_mutex_);
  while (!buffer_queue_.empty()) {
    Buffer *temp_buffer = buffer_queue_.top();
    buffer_queue_.pop();
    std::unique_ptr<Buffer> out_buffer;
    out_buffer.reset(temp_buffer);
    video_decoded_callback_->on_decoded_frame(std::move(out_buffer));
  }
  return VideoCodecError::kNoError;
}

VideoCodecError VtbVideoDecoder::get_delayed_frame() {
  if (vt_decompress_session_) {
    VTDecompressionSessionWaitForAsynchronousFrames(vt_decompress_session_);
  }
  std::unique_lock<std::mutex> lock(buffer_queue_mutex_);
  if (!buffer_queue_.empty()) {
    Buffer *temp_buffer = buffer_queue_.top();
    buffer_queue_.pop();
    std::unique_ptr<Buffer> out_buffer;
    out_buffer.reset(temp_buffer);
    video_decoded_callback_->on_decoded_frame(std::move(out_buffer));
    return VideoCodecError::kNoError;
  }
  return VideoCodecError::kEOF;
}

VideoCodecError VtbVideoDecoder::flush() {
  if (vt_decompress_session_) {
    VTDecompressionSessionWaitForAsynchronousFrames(vt_decompress_session_);
  }
  std::unique_lock<std::mutex> lock(buffer_queue_mutex_);
  while (!buffer_queue_.empty()) {
    Buffer *temp_buffer = buffer_queue_.top();
    buffer_queue_.pop();
    VideoFrameMeta *meta = temp_buffer->get_video_frame_meta();
    if (meta && meta->buffer_context) {
      VideoToolBufferContext *temp_buffer_ctx =
          (VideoToolBufferContext *)meta->buffer_context;
      if (temp_buffer_ctx) {
        CVBufferRelease((CVBufferRef)temp_buffer_ctx->buffer);
        delete temp_buffer_ctx;
      }
    }
    delete temp_buffer;
  }
  return VideoCodecError::kNoError;
}

VideoCodecError
VtbVideoDecoder::set_video_format_description(const Buffer *buffer) {
  if (buffer->get_type() != BufferType::kVideoFormatDesc ||
      !buffer->get_video_format_desc_meta()) {
    return VideoCodecError::kInvalidParameter;
  }
  for (auto &item : buffer->get_video_format_desc_meta()->items) {
    if (item.type == VideoFormatDescType::kExtraData) {
      CodecContext tmp;
      codec_ctx_ = tmp;
      continue_pts_reorder_error_ = 0;
      auto ret = init_cm_format_desc(buffer);
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
      ret = init_vt_decompress_session();
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
    }
  }
  return VideoCodecError::kNoError;
}
} // namespace reddecoder
