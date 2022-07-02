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
    std::cout << "Start global fixture." << std::endl;
#ifdef _DEBUG_TEST
    std::cout << "MemoryUsed: " << CachePool::GetMemoryUsed() << std::endl;
#endif // _DEBUG_TEST
    Logger::init(INFO);

    fs::path path(ROOT_PATH);
    if (!fs::exists(path))
      fs::create_directories(path);
  };

  ~GlobalFixTure() {
    std::cout << "Stop global fixture." << std::endl;
#ifdef _DEBUG_TEST
    std::cout << "MemoryUsed: " << CachePool::GetMemoryUsed() << std::endl;
    unordered_map<Byte *, string> &map = CachePool::_mapApply;
    for (auto iter = map.begin(); iter != map.end(); iter++) {
      std::cout << (void *)iter->first << "    " << iter->second;
    }
#endif // _DEBUG_TEST
  }
};
// 定义全局夹具
BOOST_GLOBAL_FIXTURE(GlobalFixTure);
} // namespace storage
