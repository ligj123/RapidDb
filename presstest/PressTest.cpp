// PressTest.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

#include "PressTest.h"
#include "../src/utils/Log.h"
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
static const std::string ROOT_PATH = "./dbTest";

void help() {
  std::cout << "Press test parameters:\n";
  std::cout << "First parameter: press test case name;\n";
  std::cout << "internal parameters for cases" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "Initialize press test.\n";
  utils::Logger::init(utils::ERROR);
  fs::path path(ROOT_PATH);
  if (!fs::exists(path))
    fs::create_directories(path);

  if (argc <= 1)
    help();
  std::string str(argv[1]);
  transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) -> unsigned char { return std::tolower(c); });

  if (str == "0") {
    storage::ArrayTest();
  } else if (str == "11" || str.compare("insertprimary") == 0) {
    storage::InsertSpeedPrimaryTest(argc >= 3 ? atol(argv[2]) : 0);
  } else if (str == "12" || str.compare("insertprimary") == 0) {
    storage::InsertSpeedUniqueTest(argc >= 3 ? atol(argv[2]) : 0);
  } else {
    help();
  }
}
