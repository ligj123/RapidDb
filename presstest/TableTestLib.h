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

extern const char *ROOT_PATH;
extern const char *DB_NAME;
extern const char *TBL_NAME;
extern const char *DB_TBL_NAME;
extern const char *INSERT_STMT;
extern const char *UPDATE_STMT;
extern const char *UPDATE_STMT2;
extern const char *DELETE_STMT;
extern const char *SELECT_STMT;
extern thread_local string varchar;

extern OpRedio arrRadio[];
extern int redioCount;
extern Byte *arrResult;

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

inline string FulleTblName(const string &dbPrefix, int sn,
                           const string &tblName) {
  return dbPrefix + to_string(sn) + "." + tblName;
}

void CreateDbTable(const string &dbName, bool bExclusive, int sessionGroup);

void InsertProc(uint16_t tid, const MVector<uint32_t> &vctSessId, int recStart,
                int recNum);
void CheckSelectResult(uint32_t num, VectorDataValue &vctDv);

void CheckAllRecord(const string &fullTblName, int rowNum);

void GetRecordValue(const string &fullTblName, int num);
} // namespace storage