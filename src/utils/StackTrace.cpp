#include "StackTrace.h"

#ifdef _MSVC_LANG
#include <string>
#else
#define BOOST_STACKTRACE_LINK
#include <boost/stacktrace.hpp>
#include <iostream>
#include <sstream>
#include <string>
#endif

namespace storage {
#ifdef _MSVC_LANG
std::string StackTrace(int depth) {
  static int count = 0;
  count++;
  return "NO: " + std::to_string(count);
}
#else
std::string StackTrace(int depth) {
  static int count = 0;
  count++;
  boost::stacktrace::stacktrace st;
  auto iter = st.begin() + 1;
  std::stringstream ss;

  for (int i = 0; iter != st.end() && i < depth; iter++, i++) {
    // if (iter->name().size() == 0)
    //   break;
    std::string ff = iter->source_file();
    ff = ff.substr(ff.rfind('/', ff.rfind('/') - 1) + 1);
    ss << i << "# " << iter->name() << " at " << ff << " : "
       << iter->source_line() << "\n";
  }

  return "NO: " + std::to_string(count) + "\n" + ss.str();
}
#endif // _MSVC_LANG

} // namespace storage