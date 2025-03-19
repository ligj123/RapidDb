// PressTest.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

#include "PressTest.h"
#include "../src/cache/Mallocator.h"
#include "../src/dataType/DataValueBlob.h"
#include "../src/dataType/DataValueDigit.h"
#include "../src/dataType/DataValueFactory.h"
#include "../src/dataType/DataValueFixChar.h"
#include "../src/dataType/DataValueVarChar.h"
#include "../src/utils/Log.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
static const std::string ROOT_PATH = "./dbTest";
using namespace storage;

void help() {
  std::cout << "Press test parameters:\n";
  std::cout << "First parameter: press test case name;\n";
  std::cout << "internal parameters for cases" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "Initialize press test.\n";
  fs::path path(ROOT_PATH);
  if (!fs::exists(path))
    fs::create_directories(path);

  Logger::init("./", ERROR, INFO);

  if (argc <= 1) {
    help();
    exit(0);
  }
  string str(argv[1]);
  transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) -> unsigned char { return std::tolower(c); });

  if (str == "0") {
    storage::ArrayTest();
  } else if (str == "1") {
    storage::MutexTest();
  } else if (str == "2") {
  } else if (str == "3") {
  } else if (str == "4") {

  } else if (str == "11") {
    storage::TablePointTest(1, 6, 2, 2, 100, 2000000, 10000000, true);
  } else if (str == "12") {
    int tNum = (argc >= 3 ? atoi(argv[2]) : 1);
    if (tNum < 1 || tNum > 10) {
      tNum = 1;
    }
    int rowNum = (argc >= 4 ? atoi(argv[3]) : 1000000);
    if (rowNum < 10000 || rowNum > 1000000000) {
      rowNum = 1000000;
    }
    storage::MultiThreadInsertTest(tNum, rowNum);
  } else if (str == "13") {
    int tblNum = argc >= 3 ? atoi(argv[2]) : 1;
    int sessGroupNum = argc >= 4 ? atoi(argv[3]) : 1;
    int sessNum = argc >= 5 ? atoi(argv[4]) : 10;
    int rowNum = argc >= 6 ? atoi(argv[5]) : 1000000;
    int opTimes = argc >= 7 ? atoi(argv[6]) : 1000000;

    storage::TestMultiTable(tblNum, sessGroupNum, sessNum, rowNum, opTimes);
  } else if (str == "21") {
    // int threadNum = argc >= 3 ? atol(argv[2]) : 0;
    // uint64_t recordNum = argc >= 4 ? atoll(argv[3]) : 0;
    // storage::MultiThreadInsertSpeedPrimaryTest(threadNum, recordNum);
  } else {
    help();
  }

  exit(0);
}
