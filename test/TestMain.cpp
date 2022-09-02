#define BOOST_TEST_MODULE RapidDb
#include "../src/utils/Log.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <string>

namespace storage {
namespace fs = std::filesystem;
static const std::string ROOT_PATH = "./dbTest";

struct GlobalFixTure {
  GlobalFixTure() {
    fs::path path(ROOT_PATH);
    if (!fs::exists(path))
      fs::create_directories(path);

    Logger::init("./", INFO, INFO);

    LOG_INFO << "Start global fixture." << std::endl;
  };

  ~GlobalFixTure() {
    LOG_INFO << "Stop global fixture." << std::endl;

    for (auto const &dir_entry :
         std::filesystem::directory_iterator{ROOT_PATH}) {
      if (dir_entry.is_regular_file()) {
        std::filesystem::remove(dir_entry);
      }
    }
#ifdef DEBUG_TEST
    LOG_INFO << "MemoryUsed: " << CachePool::GetMemoryUsed() << std::endl;
    unordered_map<Byte *, string> &map = CachePool::_mapApply;
    for (auto iter = map.begin(); iter != map.end(); iter++) {
      LOG_INFO << (void *)iter->first << "    " << iter->second;
    }
#endif // DEBUG_TEST
  }
};
// 定义全局夹具
BOOST_GLOBAL_FIXTURE(GlobalFixTure);
} // namespace storage
