/*
 * @Author: chengyifeng
 * @Date: 2024-03-12 11:50:27
 * @LastEditors: chengyifeng
 * @LastEditTime: 2024-03-12 16:30:55
 * @Description: 请填写简介
 */
#pragma once

#include "RedPlaylist.h"
#include <map>
#include <string>
#include <vector>
namespace redstrategycenter {
namespace playlist {
class playListParser {
public:
  playListParser() = default;

  virtual ~playListParser() = default;

  virtual PlayList *parse(const std::string &str) = 0;
};
} // namespace playlist
} // namespace redstrategycenter