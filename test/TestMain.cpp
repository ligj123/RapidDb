#define BOOST_TEST_MODULE RapidDb
#include "../src/utils/Log.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
static const std::string ROOT_PATH = "./dbTest";

struct GlobalFixTure {
  GlobalFixTure() {
    std::cout << "Start global fixture." << std::endl;
    utils::Logger::init(utils::ERROR);

    fs::path path(ROOT_PATH);
    if (!fs::exists(path))
      fs::create_directories(path);
  };

  ~GlobalFixTure() { std::cout << "Stop global fixture." << std::endl; }
};
// 定义全局夹具
BOOST_GLOBAL_FIXTURE(GlobalFixTure);
