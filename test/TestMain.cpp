#define BOOST_TEST_MODULE RapidDb
#include  <boost/test/unit_test.hpp>
#include "../src/utils/log.h"

struct GlobalFixTure {
  GlobalFixTure() {
    std::cout << "Start global fixture." << std::endl;
    utils::Logger::init();
  };
 
  ~GlobalFixTure() {
    std::cout << "Stop global fixture." << std::endl;
  }
};
// ����ȫ�ּо�
BOOST_GLOBAL_FIXTURE(GlobalFixTure);