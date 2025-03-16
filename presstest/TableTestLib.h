#pragma once
#include "../src/cache/Mallocator.h"
#include "../src/core/BranchPage.h"
#include "../src/core/IndexTree.h"
#include "../src/core/LeafPage.h"
#include "../src/core/LeafRecord.h"
#include "../src/dataType/DataValueDigit.h"
#include "../src/dataType/DataValueVarChar.h"
#include "../src/table/Table.h"

namespace storage {
enum class OpRedio : uint8_t {
  INS, // Insert
  UPD, // Update
  DEL, // Delete
  SEL  // Select
};

struct ResultStat {
  int _insertPassed{0};
  int _insertFailed{0};
  int _deletePassed{0};
  int _deleteFailed{0};
  int _updatePassed{0};
  int _updateFailed{0};
  int _selectPassed{0};
  int _selectFailed{0};
};

extern PhysTable *table;
extern thread_local string varchar;

inline uint32_t GenTestKey(uint32_t num) {
  uint32_t by1 = num & 0xff;
  uint32_t by2 = (num >> 8) & 0xff;
  uint32_t by3 = (num >> 16) & 0xff;
  uint32_t by4 = (num >> 24) & 0xff;
  return ((by1 & 0x05) + (by2 & 0x0A) + (by3 & 0x50) + (by4 & 0xA0)) +
         (((by1 & 0xA0) + (by2 & 0x05) + (by3 & 0x0A) + (by4 & 0x50)) << 8) +
         (((by1 & 0x50) + (by2 & 0xA0) + (by3 & 0x05) + (by4 & 0x0A)) << 16) +
         (((by1 & 0x0A) + (by2 & 0x50) + (by3 & 0xA0) + (by4 & 0x05)) << 24);
}

inline int64_t GenPrimaryKey(uint32_t num) {
  uint64_t val = GenTestKey(num);
  return val * val + val;
}

inline VectorRow GenRow(uint32_t num) {
  VectorDataValue vctDv;
  uint32_t val = GenTestKey(num);
  DataValueLong *dvLong = new DataValueLong((int64_t)val * val + val);
  DataValueInt *dvInt = new DataValueInt(BytesSwap32(val));

  sprintf(varchar.data() + 30, "0x%08X", (val / 10));
  const char *p = varchar.c_str();
  DataValueVarChar *dvVar = new DataValueVarChar(p, strlen(p), 50);

  vctDv.push_back(dvLong);
  vctDv.push_back(dvInt);
  vctDv.push_back(dvVar);
  VectorRow vctRow;
  vctRow.push_back(move(vctDv));

  return vctRow;
}

void InsertProc(uint16_t tid, MVector<uint32_t> vctSessId, int recStart,
                int recNum);
void CheckSelectResult(uint32_t num, VectorDataValue &vctDv);
} // namespace storage