#pragma once

class CFObjHelper {
public:
  static void dict_set_string(CFMutableDictionaryRef dict, CFStringRef key,
                              const char *value) {
    CFStringRef string;
    string = CFStringCreateWithCString(NULL, value, kCFStringEncodingASCII);
    CFDictionarySetValue(dict, key, string);
    CFRelease(string);
  }

  static void dict_set_boolean(CFMutableDictionaryRef dict, CFStringRef key,
                               bool value) {
    CFDictionarySetValue(dict, key, value ? kCFBooleanTrue : kCFBooleanFalse);
  }

  static void dict_set_object(CFMutableDictionaryRef dict, CFStringRef key,
                              CFTypeRef *value) {
    CFDictionarySetValue(dict, key, value);
  }

  static void dict_set_data(CFMutableDictionaryRef dict, CFStringRef key,
                            uint8_t *value, uint64_t length) {
    CFDataRef data;
    data = CFDataCreate(NULL, value, (CFIndex)length);
    CFDictionarySetValue(dict, key, data);
    CFRelease(data);
  }

  static void dict_set_i32(CFMutableDictionaryRef dict, CFStringRef key,
                           int32_t value) {
    CFNumberRef number;
    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
    CFDictionarySetValue(dict, key, number);
    CFRelease(number);
  }
};
