#pragma once
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
#include <string>

using namespace std;

class UrlParser {
public:
  UrlParser(
      const string &url,
      const string &separator = ""); // Support several urls splitted by
                                     // separator, while those MD5 are the same
  ~UrlParser();
  bool operator==(const UrlParser &dst);
  string geturi();
  std::list<std::string> getUrlList();
  string getdomain();
  string getprotocol();
  static bool compare(const string &src, const string &dst);

private:
  using string_pos = decltype(std::string("UrlParser").find("U"));
  struct UrlParam {
    string protocol;
    string host;
    unsigned short port = 80;
    string uri;
  };
  unique_ptr<UrlParam> murlparam = nullptr;
  const char *murl;
  bool parse_proto(const string &url);
  bool parse_domain(const char *pos, const char *posend);
  bool ParseUrl(const char *url);
  bool IsNumber(const char *num);
  bool equeal(const char *pos, const char *compare, const size_t &clen);
  int string_find(const char *u, const char *compare);
  const char *string_find_pos(const char *u, const char *compare);
  void parseUrlList(const string &urlStringList, const string &separator,
                    const string &proto);
  std::list<std::string> murlList;
};
