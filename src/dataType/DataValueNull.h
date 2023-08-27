#pragma once
#include "../cache/CachePool.h"
#include "../cache/Mallocator.h"
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "DataType.h"
#include <any>
#include <cassert>
#include <cstring>
#include <fstream>

namespace storage {
class DataValueNull {
public:
  DataValueNull(const DataValueNull &dv) : IDataValue(dv) {}
  DataValueNull() : IDataValue(DataType::VAL_NULL, ValueType::SOLE_VALUE) {}

  bool Copy(const IDataValue &dv, bool bMove = false) override { return true; }
  DataValueNull *Clone(bool incVal = false) override { new DataValueNull(); }
  std::any GetValue() override { return std::any(); }
  bool PutValue(std::any val) override{};
  void SetNull() override{};
  uint32_t WriteData(Byte *buf, SavePosition svPos) const override { return 0; }
  uint32_t ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                    bool bSole = true) override {
    return 0;
  }
  uint32_t WriteData(Byte *buf) const override { return 0; }
  uint32_t ReadData(Byte *buf) override { return 0; }
  uint32_t GetDataLength() const override { return 0; }
  uint32_t GetMaxLength() const override { return 0; }
  uint32_t GetPersistenceLength(SavePosition dtPos) const override { return 0; }
  void SetMinValue() override {}
  void SetMaxValue() override {}
  void SetDefaultValue() override {}
  void ToString(StrBuff &sb) override {}
  size_t Hash() override { return 0; }
  int64_t GetLong() const override { return 0; }
  double GetDouble() const override { return 0; }
  Byte *GetBuff() const override { return nullptr; }

  bool EQ(const IDataValue &dv) const override { abort(); }
  bool GT(const IDataValue &dv) const override { abort(); };
  bool LT(const IDataValue &dv) const override { abort(); };
};
} // namespace storage
