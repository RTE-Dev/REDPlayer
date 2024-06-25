#pragma once
#include <map>
#include <string>
#include <vector>

#include "REDDownloader.h"

using namespace std;

// need to sync with Source
enum class CDNType {
  kNormalCDN = 0,
};

class RedDownLoadFactory {
public:
  static RedDownLoad *create(RedDownLoadPara *task,
                             CDNType cdn_type = CDNType::kNormalCDN,
                             int range_size = 0);
  virtual ~RedDownLoadFactory() = default;
};
