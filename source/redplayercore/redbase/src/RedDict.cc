#include "RedDict.h"

RedDict::~RedDict() { Clear(); }

void RedDict::Clear() {
  for (size_t i = 0; i < items_.size(); ++i) {
    Item *item = items_[i];
    if (item) {
      if (item->name_) {
        delete[] item->name_;
        item->name_ = nullptr;
      }
      FreeItemValue(item);
      delete item;
    }
  }
  items_.clear();
}

#define BASIC_TYPE(NAME, FIELDNAME, TYPENAME)                                  \
  void RedDict::Set##NAME(const char *name, TYPENAME value) {                  \
    Item *item = AllocateItem(name);                                           \
                                                                               \
    if (item) {                                                                \
      item->type_ = kType##NAME;                                               \
      item->u_.FIELDNAME = value;                                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  bool RedDict::Find##NAME(const char *name, TYPENAME *value) const {          \
    const Item *item = FindItem(name, kType##NAME);                            \
    if (item) {                                                                \
      *value = item->u_.FIELDNAME;                                             \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  TYPENAME RedDict::Get##NAME(const char *name, TYPENAME default_value)        \
      const {                                                                  \
    const Item *item = FindItem(name, kType##NAME);                            \
    if (item) {                                                                \
      return item->u_.FIELDNAME;                                               \
    }                                                                          \
    return default_value;                                                      \
  }
BASIC_TYPE(UnsignedInt, unsigned_int_value, unsigned int);
BASIC_TYPE(Int32, int32_value, int32_t)
BASIC_TYPE(Int64, int64_value, int64_t)
BASIC_TYPE(Size, size_value, size_t)
BASIC_TYPE(Float, float_value, float)
BASIC_TYPE(Double, double_value, double)
BASIC_TYPE(Pointer, ptr_value, void *)
BASIC_TYPE(Boolean, boolean_value, bool);

void RedDict::SetString(const char *name, const char *s, ssize_t len) {
  if (!s)
    return;
  auto str = new std::string(s, len <= 0 ? strlen(s) : len);
  Item *item = AllocateItem(name);
  if (item) {
    item->type_ = kTypeString;
    item->u_.string_value = str;
  }
}

void RedDict::SetString(const char *name, const std::string &s) {
  auto str = new std::string(s);
  Item *item = AllocateItem(name);
  if (item) {
    item->type_ = kTypeString;
    item->u_.string_value = str;
  }
}

bool RedDict::FindString(const char *name, std::string *value) const {
  const Item *item = FindItem(name, kTypeString);
  if (item) {
    *value = *(item->u_.string_value);
    return true;
  }
  return false;
}

std::string RedDict::GetString(const char *name,
                               std::string *default_value) const {
  const Item *item = FindItem(name, kTypeString);
  if (item) {
    return *(item->u_.string_value);
  }
  return default_value ? (*default_value) : "";
}

bool RedDict::Contains(const char *name) const {
  size_t i = FindItemIndex(name, strlen(name));
  return i < items_.size();
}

bool RedDict::Contains(const char *name, ValType type) const {
  if (FindItem(name, type)) {
    return true;
  }
  return false;
}

void RedDict::Item::SetName(const char *name, size_t len) {
  name_length_ = len;
  name_ = new char[len + 1];
  memcpy((void *)name_, name, len + 1);
}

RedDict::Item *RedDict::AllocateItem(const char *name) {
  size_t len = strlen(name);
  size_t i = FindItemIndex(name, len);
  Item *item;

  if (i < items_.size()) {
    item = items_[i];
    FreeItemValue(item);
  } else {
    item = new Item();
    if (!item) {
      return item;
    }
    item->SetName(name, len);
    items_.emplace_back(item);
  }

  return item;
}

void RedDict::FreeItemValue(Item *item) {
  switch (item->type_) {
  case kTypeString: {
    delete item->u_.string_value;
    break;
  }

  default:
    break;
  }
}

const RedDict::Item *RedDict::FindItem(const char *name, ValType type) const {
  size_t i = FindItemIndex(name, strlen(name));

  if (i < items_.size()) {
    const Item *item = items_[i];
    return item->type_ == type ? item : nullptr;
  }

  return nullptr;
}

size_t RedDict::FindItemIndex(const char *name, size_t len) const {
  size_t i = 0;
  for (; i < items_.size(); i++) {
    if (len != items_[i]->name_length_) {
      continue;
    }
    if (!memcmp(items_[i]->name_, name, len)) {
      break;
    }
  }
  return i;
}

size_t RedDict::CountEntries() const { return items_.size(); }

const char *RedDict::GetEntryNameAt(size_t index, ValType *type) const {
  if (index >= items_.size()) {
    *type = kTypeInt32;

    return nullptr;
  }

  *type = items_[index]->type_;

  return items_[index]->name_;
}
