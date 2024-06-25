#pragma once

#include <string>

typedef struct ByteArray {
  uint8_t *data{nullptr};
  int size{0};

  ByteArray() = default;

  ByteArray(const void *bytes, int len) {
    if (len > 0) {
      data = reinterpret_cast<uint8_t *>(malloc(len));
      if (!data)
        return;
      memcpy(data, bytes, len);
      size = len;
    }
  }

  ~ByteArray() { clear(); }

  ByteArray(const ByteArray &other) {
    if (other.size > 0) {
      data = reinterpret_cast<uint8_t *>(malloc(other.size));
      if (!data)
        return;
      memcpy(data, other.data, other.size);
      size = other.size;
    }
  }

  ByteArray &operator=(const ByteArray &other) {
    if (this != &other) {
      if (size > 0) {
        free(data);
        data = nullptr;
        size = 0;
      }
      if (other.size > 0) {
        data = reinterpret_cast<uint8_t *>(malloc(other.size));
        if (!data)
          return *this;
        memcpy(data, other.data, other.size);
        size = other.size;
      }
    }
    return *this;
  }

  ByteArray(ByteArray &&other) noexcept {
    data = other.data;
    size = other.size;
    other.data = nullptr;
    other.size = 0;
  }

  ByteArray &operator=(ByteArray &&other) noexcept {
    if (this != &other) {
      if (size > 0) {
        free(data);
        data = nullptr;
        size = 0;
      }
      data = other.data;
      size = other.size;
      other.data = nullptr;
      other.size = 0;
    }
    return *this;
  }

  bool isEmpty() const { return size <= 0; }

  bool isEqual(const ByteArray &other) const {
    return size == other.size && memcmp(data, other.data, size) == 0;
  }

  void clear() {
    if (size > 0) {
      free(data);
      data = nullptr;
      size = 0;
    }
  }

  // if prefix is true, take '0x' as prefix
  std::string toHex(bool prefix = false, const std::string &separator = "") {
    char ch[3] = {0};
    std::string hexString;
    for (int i = 0; i < size; ++i) {
      snprintf(ch, sizeof(ch), "%02X", data[i]);
      if (prefix) {
        hexString = hexString + "0x" + ch;
      } else {
        hexString += ch;
      }
      if (i != size - 1) {
        hexString += separator;
      }
    }
    return hexString;
  }
} ByteArray;

class Cipher {
public:
#pragma mark - Base Function
  Cipher() = default;
  ~Cipher() = default;
  void setData(const ByteArray &bytes);
  void setData(const void *bytes, const int size);
  void setData(const std::string &string);

public:
#pragma mark - Encode/Decode
  ByteArray base64Encode(bool newLine);
  ByteArray base64Decode(bool newLine);

  ByteArray md5Encode();
  // check file md5
  ByteArray md5Encode(const std::string &filePath, const int startPos = 0,
                      const int endPos = INT_MAX);

#pragma mark - Encrypt/Decrypt
  enum class CipherType : uint8_t {
    NONE,
    AES128_CBC_WITHOUT_PADDING,
    AES128_CBC_PKCS7
  };
  ByteArray Encrypt(CipherType type, const ByteArray &key, const ByteArray &iv);
  ByteArray Decrypt(CipherType type, const ByteArray &key, const ByteArray &iv);

private:
  ByteArray data;
};

using CipherType = Cipher::CipherType;
