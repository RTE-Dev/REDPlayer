#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"
#include <vector>

enum ValType {
  kTypeInt32,
  kTypeInt64,
  kTypeSize,
  kTypeFloat,
  kTypeDouble,
  kTypePointer,
  kTypeString,
  kTypeUnsignedInt,
  kTypeBoolean,
  kTypeConst,
  kTypeUnknown
};

class RedDict {
public:
  RedDict() = default;
  virtual ~RedDict();

  void Clear();

  void SetUnsignedInt(const char *name, unsigned int value);
  void SetInt32(const char *name, int32_t value);
  void SetInt64(const char *name, int64_t value);
  void SetSize(const char *name, size_t value);
  void SetFloat(const char *name, float value);
  void SetDouble(const char *name, double value);
  void SetPointer(const char *name, void *value);
  void SetBoolean(const char *name, bool value);

  bool FindUnsignedInt(const char *name, unsigned int *value) const;
  bool FindInt32(const char *name, int32_t *value) const;
  bool FindInt64(const char *name, int64_t *value) const;
  bool FindSize(const char *name, size_t *value) const;
  bool FindFloat(const char *name, float *value) const;
  bool FindDouble(const char *name, double *value) const;
  bool FindPointer(const char *name, void **value) const;
  bool FindBoolean(const char *name, bool *value) const;

  unsigned int GetUnsignedInt(const char *name,
                              unsigned int default_value) const;
  int32_t GetInt32(const char *name, int32_t default_value) const;
  int64_t GetInt64(const char *name, int64_t default_value) const;
  size_t GetSize(const char *name, size_t default_value) const;
  float GetFloat(const char *name, float default_value) const;
  double GetDouble(const char *name, double default_value) const;
  void *GetPointer(const char *name, void *default_value) const;
  bool GetBoolean(const char *name, bool default_value) const;

  void SetString(const char *name, const char *s, ssize_t len = -1);

  void SetString(const char *name, const std::string &s);

  bool FindString(const char *name, std::string *value) const;

  std::string GetString(const char *name, std::string *default_value) const;

  bool Contains(const char *name) const;

  bool Contains(const char *name, ValType type) const;

  size_t CountEntries() const;

  const char *GetEntryNameAt(size_t index, ValType *type) const;

protected:
  struct Item {
    union {
      unsigned int unsigned_int_value;
      bool boolean_value;
      int32_t int32_value;
      int64_t int64_value;
      size_t size_value;
      float float_value;
      double double_value;
      void *ptr_value;
      std::string *string_value;
    } u_;
    const char *name_;
    size_t name_length_;
    ValType type_;
    void SetName(const char *name, size_t len);
  };

  std::vector<Item *> items_;

  Item *AllocateItem(const char *name);
  void FreeItemValue(Item *item);
  const Item *FindItem(const char *name, ValType type) const;

  size_t FindItemIndex(const char *name, size_t len) const;

  DISALLOW_EVIL_CONSTRUCTORS(RedDict);
};
