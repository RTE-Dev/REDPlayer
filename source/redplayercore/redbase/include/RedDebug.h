/*
 * RedDebug.h
 *
 *  Created on: 2022年3月29日
 *      Author: liuhongda
 */

#pragma once

#include "RedBase.h"
#include "RedError.h"
#include "RedLog.h"

#include <string.h>

inline static const char *AsString(status_t i, const char *def = "??") {
  switch (i) {
  case NO_ERROR:
    return "NO_ERROR";
  case UNKNOWN_ERROR:
    return "UNKNOWN_ERROR";
  case NO_MEMORY:
    return "NO_MEMORY";
  case INVALID_OPERATION:
    return "INVALID_OPERATION";
  case BAD_VALUE:
    return "BAD_VALUE";
  case BAD_TYPE:
    return "BAD_TYPE";
  case NAME_NOT_FOUND:
    return "NAME_NOT_FOUND";
  case PERMISSION_DENIED:
    return "PERMISSION_DENIED";
  case NO_INIT:
    return "NO_INIT";
  case ALREADY_EXISTS:
    return "ALREADY_EXISTS";
  case DEAD_OBJECT:
    return "DEAD_OBJECT";
  case FAILED_TRANSACTION:
    return "FAILED_TRANSACTION";
  case BAD_INDEX:
    return "BAD_INDEX";
  case NOT_ENOUGH_DATA:
    return "NOT_ENOUGH_DATA";
  case WOULD_BLOCK:
    return "WOULD_BLOCK";
  case TIMED_OUT:
    return "TIMED_OUT";
  case UNKNOWN_TRANSACTION:
    return "UNKNOWN_TRANSACTION";
  case FDS_NOT_ALLOWED:
    return "FDS_NOT_ALLOWED";
  default:
    return def;
  }
}

#define LITERAL_TO_STRING_INTERNAL(x) #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define DEBUG_TAG "RedDebug"

#ifndef CHECK_OP

#define CHECK(condition)                                                       \
  AV_LOGF_IF(!(condition), DEBUG_TAG, "%s",                                    \
             __FILE__ ":" LITERAL_TO_STRING(__LINE__) " CHECK(" #condition     \
                                                      ") failed.")

#define CHECK2(condition, fmt, ...)                                            \
  AV_LOGF_IF(!(condition), DEBUG_TAG, "%s" fmt,                                \
             __FILE__ ":" LITERAL_TO_STRING(__LINE__) " CHECK(" #condition     \
                                                      ") failed. ",            \
             ##__VA_ARGS__)

#define CHECK_OP(x, y, suffix, op)                                             \
  do {                                                                         \
    const auto &a = x;                                                         \
    const auto &b = y;                                                         \
    if (!(a op b)) {                                                           \
      std::string ___full =                                                    \
          __FILE__ ":" LITERAL_TO_STRING(__LINE__) " CHECK_" #suffix "(" #x    \
                                                   ", " #y ") failed";         \
      AV_LOGF(DEBUG_TAG, "%s", ___full.c_str());                               \
    }                                                                          \
  } while (false)

#define CHECK_EQ(x, y) CHECK_OP(x, y, EQ, ==)
#define CHECK_NE(x, y) CHECK_OP(x, y, NE, !=)
#define CHECK_LE(x, y) CHECK_OP(x, y, LE, <=)
#define CHECK_LT(x, y) CHECK_OP(x, y, LT, <)
#define CHECK_GE(x, y) CHECK_OP(x, y, GE, >=)
#define CHECK_GT(x, y) CHECK_OP(x, y, GT, >)

#endif

#define TRESPASS(...)                                                          \
  AV_LOGF(DEBUG_TAG, "%s",                                                     \
          __FILE__ ":" LITERAL_TO_STRING(__LINE__) " Should not be here. ");

#ifdef NDEBUG
#define CHECK_DBG CHECK
#define CHECK_EQ_DBG CHECK_EQ
#define CHECK_NE_DBG CHECK_NE
#define CHECK_LE_DBG CHECK_LE
#define CHECK_LT_DBG CHECK_LT
#define CHECK_GE_DBG CHECK_GE
#define CHECK_GT_DBG CHECK_GT
#define TRESPASS_DBG TRESPASS
#else
#define CHECK_DBG(condition)
#define CHECK_EQ_DBG(x, y)
#define CHECK_NE_DBG(x, y)
#define CHECK_LE_DBG(x, y)
#define CHECK_LT_DBG(x, y)
#define CHECK_GE_DBG(x, y)
#define CHECK_GT_DBG(x, y)
#define TRESPASS_DBG(...)
#endif
