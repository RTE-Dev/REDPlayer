#include "Cipher.h"

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#include <fstream>

#pragma mark - Base Function
void Cipher::setData(const ByteArray &bytes) { data = bytes; }

void Cipher::setData(const void *bytes, const int size) {
  data = {bytes, size};
}

void Cipher::setData(const std::string &string) {
  data = {string.c_str(), static_cast<int>(string.length())};
}

#pragma mark - Encode/Decode
ByteArray Cipher::base64Encode(bool newLine) {
  do {
    if (data.isEmpty())
      break;

    BIO *bio = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    if (!newLine) {
      BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    }
    bio = BIO_push(b64, bio);
    BIO_write(bio, data.data, data.size);
    BIO_flush(bio);
    BUF_MEM *buffer = nullptr;
    BIO_get_mem_ptr(bio, &buffer);
    if (!buffer)
      break;
    ByteArray output(buffer->data, static_cast<int>(buffer->length));
    BIO_free_all(bio); // free bio, b64 and buffer

    return output;
  } while (false);

  return ByteArray();
}

ByteArray Cipher::base64Decode(bool newLine) {
  do {
    if (data.isEmpty())
      break;

    std::string encodedString(reinterpret_cast<char *>(data.data), data.size);
    if (!newLine) {
      // openssl: use '=' to fill the input string, if the size of the
      // string can not be divided exactly by 4
      int diff = static_cast<int>(data.size) % 4;
      encodedString = encodedString + std::string(4 - diff, '=');
    }

    const int stringLength = static_cast<int>(encodedString.length());
    uint8_t *buffer = reinterpret_cast<uint8_t *>(malloc(stringLength));
    if (!buffer)
      break;
    memset(buffer, 0, stringLength);
    BIO *bio = BIO_new_mem_buf(encodedString.c_str(), stringLength);
    BIO *b64 = BIO_new(BIO_f_base64());
    if (!newLine) {
      BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    }
    bio = BIO_push(b64, bio);
    int len = BIO_read(bio, buffer, stringLength);
    BIO_free_all(bio);
    ByteArray output(buffer, len);
    free(buffer);

    return output;
  } while (false);

  return ByteArray();
}

ByteArray Cipher::md5Encode() {
  do {
    if (data.isEmpty())
      break;

    unsigned char
        md5[MD5_DIGEST_LENGTH]; // MD5_DIGEST_LENGTH is only 16, do not
                                // need to apply for memory in the heap
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data.data, data.size);
    MD5_Final(md5, &ctx);

    return {md5, MD5_DIGEST_LENGTH};
  } while (false);

  return ByteArray();
}

ByteArray Cipher::md5Encode(const std::string &filePath, const int startPos,
                            const int endPos) {
  do {
    if (filePath.empty() || startPos >= endPos)
      break;

    MD5_CTX ctx;
    unsigned char md5[MD5_DIGEST_LENGTH];
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file)
      break;
    file.seekg(startPos);
    int nowPos = startPos;
    MD5_Init(&ctx);
    char buffer[1024];
    while (file.good() && nowPos < endPos) {
      int restSize = endPos - nowPos;
      file.read(buffer, restSize < 1024 ? restSize : 1024);
      MD5_Update(&ctx, buffer, file.gcount());
      nowPos += file.gcount();
    }
    MD5_Final(md5, &ctx);

    return {md5, MD5_DIGEST_LENGTH};
  } while (false);

  return ByteArray();
}

#pragma mark - Encrypt/Decrypt
ByteArray Cipher::Encrypt(CipherType type, const ByteArray &key,
                          const ByteArray &iv) {
  do {
    if (data.isEmpty())
      break;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    switch (type) {
    case CipherType::AES128_CBC_WITHOUT_PADDING: {
      EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                         reinterpret_cast<unsigned char *>(key.data),
                         reinterpret_cast<unsigned char *>(iv.data));
      EVP_CIPHER_CTX_set_padding(ctx, 0);
    } break;
    case CipherType::AES128_CBC_PKCS7: {
      EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                         reinterpret_cast<unsigned char *>(key.data),
                         reinterpret_cast<unsigned char *>(iv.data));
      EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);
    } break;
    default:
      return ByteArray();
    }

    unsigned char *buffer = nullptr;
    buffer =
        reinterpret_cast<unsigned char *>(malloc(data.size + AES_BLOCK_SIZE));
    if (!buffer)
      break;
    int outlen = 0;
    std::string output;
    EVP_EncryptUpdate(ctx, buffer, &outlen, (unsigned char *)data.data,
                      static_cast<int>(data.size));
    output.append(reinterpret_cast<char *>(buffer), outlen);
    EVP_EncryptFinal_ex(ctx, buffer, &outlen);
    output.append(reinterpret_cast<char *>(buffer), outlen);
    EVP_CIPHER_CTX_free(ctx);
    free(buffer);

    return {output.c_str(), static_cast<int>(output.length())};
  } while (false);

  return ByteArray();
}

ByteArray Cipher::Decrypt(CipherType type, const ByteArray &key,
                          const ByteArray &iv) {
  do {
    if (data.isEmpty())
      break;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    switch (type) {
    case CipherType::AES128_CBC_WITHOUT_PADDING: {
      EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                         reinterpret_cast<unsigned char *>(key.data),
                         reinterpret_cast<unsigned char *>(iv.data));
      EVP_CIPHER_CTX_set_padding(ctx, 0);
    } break;
    case CipherType::AES128_CBC_PKCS7: {
      EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                         reinterpret_cast<unsigned char *>(key.data),
                         reinterpret_cast<unsigned char *>(iv.data));
      EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);
    } break;
    default:
      return ByteArray();
    }

    unsigned char *buffer = nullptr;
    buffer =
        reinterpret_cast<unsigned char *>(malloc(data.size + AES_BLOCK_SIZE));
    if (!buffer)
      break;
    int outlen = 0;
    std::string output;
    EVP_DecryptUpdate(ctx, buffer, &outlen, (unsigned char *)data.data,
                      static_cast<int>(data.size));
    output.append(reinterpret_cast<char *>(buffer), outlen);
    EVP_DecryptFinal_ex(ctx, buffer, &outlen);
    output.append(reinterpret_cast<char *>(buffer), outlen);
    EVP_CIPHER_CTX_free(ctx);
    free(buffer);

    return {output.c_str(), static_cast<int>(output.length())};
  } while (false);

  return ByteArray();
}
