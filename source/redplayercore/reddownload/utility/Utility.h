//
//  Utility.h
//  RedDownload
//
//  Created by xhs_zgc on 2023/2/16.
//

#pragma once

#ifndef Utility_45604516_ADAD_11ED_AF58_3E641D0BFBDC_H
#define Utility_45604516_ADAD_11ED_AF58_3E641D0BFBDC_H

#include <mutex>

static std::once_flag globalCurlInitOnceFlag;

int64_t getTimestampMs();

#endif /* Utility_45604516_ADAD_11ED_AF58_3E641D0BFBDC_H */
