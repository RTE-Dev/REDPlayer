/*  OpenFormat
 *
 *  Created by xhs_lvpeng 20221209
 */

#include "libavformat/avformat.h"
#include "libavformat/url.h"
#include "libavformat/version.h"

#define RED_REGISTER_PROTOCOL(x)                                               \
  {                                                                            \
    extern URLProtocol redmp_ff_##x##_protocol;                                \
    int redav_register_##x##_protocol(URLProtocol *protocol,                   \
                                      int protocol_size);                      \
    redav_register_##x##_protocol(&redmp_ff_##x##_protocol,                    \
                                  sizeof(URLProtocol));                        \
  }

void redav_register_all() {
  static int initialized;

  if (initialized)
    return;
  initialized = 1;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif

  /* protocols */
  RED_REGISTER_PROTOCOL(reddownload_adapter);
}
