#include "REDURLParser.h"

#include "RedDownloadConfig.h"

UrlParser::UrlParser(const string &url, const string &separator) {
  if (!url.empty()) {
    murlparam = std::unique_ptr<UrlParam>(new UrlParam());
    parse_proto(url);
    if (!separator.empty()) {
      parseUrlList(url, separator, murlparam->protocol);
      if (!murlList.empty()) {
        murl = murlList.front().c_str();
      }
    } else {
      murl = url.c_str();
    }
    ParseUrl(murl);
  } else {
    murl = nullptr;
  }
}

UrlParser::~UrlParser() {}
bool UrlParser::operator==(const UrlParser &dst) {
  if (murlparam != nullptr && dst.murlparam != nullptr) {
    return murlparam->uri ==
           dst.murlparam->uri; // The same uri means the same resource
  }
  return false;
}

bool UrlParser::compare(const string &src, const string &dst) { return false; }

string UrlParser::geturi() {
  string uri;
  if (murlparam != nullptr) {
    uri = murlparam->uri;
    return uri;
  }
  return uri;
}

string UrlParser::getdomain() {
  string domain;
  if (murlparam != nullptr) {
    domain = murlparam->host;
  }
  return domain;
}

string UrlParser::getprotocol() {
  string protocol;
  if (murlparam != nullptr) {
    protocol = murlparam->protocol;
  }
  return protocol;
}

std::list<std::string> UrlParser::getUrlList() { return murlList; }

bool UrlParser::parse_proto(const string &url) {
  string_pos protoSplitPos = url.find("://");
  if (protoSplitPos == std::string::npos)
    return false;
  murlparam->protocol = url.substr(0, protoSplitPos);
  return true;
}

bool UrlParser::ParseUrl(const char *url) {
  const char *posend = url + strlen(url) - 1;
  murlparam->uri = url;
  const char *pos = url;
  int point = 0;
  if ((point = string_find(pos, "://")) >= 0) {
    murlparam->protocol = string(url, point);
  } else {
    return false;
  }
  pos += point + 3; // strlen("://")
  if (pos >= posend)
    return false;
  if ((point = string_find(pos, "/")) >= 0) {
    murlparam->host = string(pos, point);
    const char *end = pos + point;
    parse_domain(pos, end);
    int md5_point = 0;
    if ((md5_point = string_find(end, "?")) >= 0) {
      murlparam->uri = string(end + 1, md5_point - 1);
    } else {
      murlparam->uri = string(pos + point + 1);
    }
  } else {
    // the left all is domain
    int hlen = static_cast<int>(posend - pos + 1);
    murlparam->host = string(pos, hlen);
    const char *end = pos + hlen - 1;
    parse_domain(pos, end);
    murlparam->uri = "/";
    return true;
  }
  // delete ".mp4, .mov, .mkv"
  int suffix_point = 0;
  pos = murlparam->uri.c_str();
  if ((suffix_point = string_find(pos, ".mp4")) >= 0) {
    murlparam->uri = string(pos, suffix_point);
  }
  pos = murlparam->uri.c_str();
  if ((suffix_point = string_find(pos, ".mov")) >= 0) {
    murlparam->uri = string(pos, suffix_point);
  }
  pos = murlparam->uri.c_str();
  if ((suffix_point = string_find(pos, ".mkv")) >= 0) {
    murlparam->uri = string(pos, suffix_point);
  }
  replace(murlparam->uri.begin(), murlparam->uri.end(), '/', '_');
  return true;
}

bool UrlParser::parse_domain(const char *pos, const char *posend) {
  int point = string_find(pos, ":");
  if (point >= 0) {
    murlparam->host = string(pos, point);
    pos += point + 1;
    if (posend - pos <= 0)
      return false;
    string tmp = string(pos, posend - pos);
    if (IsNumber(tmp.c_str()))
      murlparam->port = atoi(tmp.c_str());
    return true;
  }
  return false;
}

bool UrlParser::IsNumber(const char *num) {
  int length = static_cast<int>(strlen(num));
  for (int i = 0; i < length; i++) {
    if (i == 0 && (num[i] == '+' || num[i] == '-')) {
      if (length > 1)
        continue;
      return false;
    }
    if (!isdigit(num[i]))
      return false;
  }
  return true;
}

int UrlParser::string_find(const char *u, const char *compare) {
  size_t clen = strlen(compare);
  size_t ulen = strlen(u);
  if (clen > ulen)
    return -1;
  const char *pos = u;
  const char *posend = u + ulen - 1;
  for (; pos <= posend - clen + 1; pos++) {
    if (equeal(pos, compare, clen)) {
      return static_cast<int>(pos - u);
    }
  }
  return -1;
}

const char *UrlParser::string_find_pos(const char *u, const char *compare) {
  size_t clen = strlen(compare);
  size_t ulen = strlen(u);
  if (clen > ulen)
    return NULL;
  const char *pos = u;
  const char *posend = u + ulen - 1;
  for (; pos <= posend - clen + 1; pos++) {
    if (equeal(pos, compare, clen)) {
      return pos;
    }
  }
  return NULL;
}

bool UrlParser::equeal(const char *pos, const char *compare,
                       const size_t &clen) {
  for (size_t i = 0; i < clen; i++) {
    if (pos[i] != compare[i])
      return false;
  }
  return true;
}

void UrlParser::parseUrlList(const string &urlStringList,
                             const string &separator, const string &proto) {
  for (string_pos start = 0;;) {
    string_pos splitPos;
    splitPos = urlStringList.find(separator, start);

    if (splitPos == std::string::npos) {
      murlList.push_back(
          urlStringList.substr(start, urlStringList.length() - start));
      break;
    }
    murlList.push_back(urlStringList.substr(start, splitPos - start));
    start = splitPos + separator.length();
  }
}
