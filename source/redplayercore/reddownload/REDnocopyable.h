#pragma once

class REDnocopyable {
protected:
  REDnocopyable() {}
  ~REDnocopyable() {}

private:
  REDnocopyable(const REDnocopyable &that);
  REDnocopyable(REDnocopyable &&that);
  REDnocopyable &operator=(const REDnocopyable &that);
  REDnocopyable &operator=(REDnocopyable &&that);
};
