#pragma once
#include "../cache/CachePool.h"
#include "../cache/Mallocator.h"
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "DataType.h"
#include "IDataValue.h"

#include <any>
#include <cassert>
#include <cstring>
#include <fstream>

namespace storage {
class DataValueNull : public IDataValue {
public:
  DataValueNull(const DataValueNull &dv) : IDataValue(dv) {}
  DataValueNull() : IDataValue(DataType::VAL_NULL, ValueType::SOLE_VALUE) {}

  bool Copy(const IDataValue &dv, bool bMove = false) override { return true; }
  DataValueNull *Clone(bool incVal = false) override {
    return new DataValueNull();
  }
  std::any GetValue() const override { return std::any(); }
  bool PutValue(std::any val) override { return true; };
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
  void ToString(StrBuff &sb) const override {}
  size_t Hash() const override { return 0; }
  int64_t GetLong() const override { return 0; }
  double GetDouble() const override { return 0; }
  Byte *GetBuff() const override { return nullptr; }

  bool EQ(const IDataValue &dv) const override {
    return dv.GetDataType() == DataType::VAL_NULL;
  }
  bool GT(const IDataValue &dv) const override { return false; };
  bool LT(const IDataValue &dv) const override {
    return dv.GetDataType() != DataType::VAL_NULL && !dv.IsNull();
  };
};
} // namespace storage
