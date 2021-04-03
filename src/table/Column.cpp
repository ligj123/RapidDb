#include "Column.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueLong.h"
#include "../dataType/DataValueVarChar.h"
#include <cstring>

namespace storage {
PersistColumn::PersistColumn(const std::string &name, uint32_t pos,
                             DataType dataType, const string &comments,
                             bool bNullable, uint32_t maxLen, uint32_t incStep,
                             utils::Charsets charset, IDataValue *defaultVal)
    : BaseColumn(name, pos, dataType), bNullable_(bNullable),
      comments_(comments), maxLength_(maxLen), incStep_(incStep),
      charset_(charset), pDefaultVal_(defaultVal) {}
PersistColumn::~PersistColumn() { delete pDefaultVal_; }

uint32_t PersistColumn::ReadData(Byte *pBuf) {
  Byte *p = pBuf;
  *((uint32_t *)p) = (uint32_t)name_.size();
  p += sizeof(uint32_t);
  std::memcpy(p, name_.c_str(), name_.size());
  p += name_.size();

  *((uint32_t *)p) = position_;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)dataType_;
  p += sizeof(uint32_t);

  *p = (bNullable_ ? 0 : 1);
  p++;

  *((uint32_t *)p) = maxLength_;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = incStep_;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)charset_;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)comments_.size();
  p += sizeof(uint32_t);
  std::memcpy(p, comments_.c_str(), comments_.size());
  p += comments_.size();

  *p = (pDefaultVal_ == nullptr ? 0 : 1);
  p++;
  if (pDefaultVal_ != nullptr) {
    p += pDefaultVal_->WriteData(p);
  }

  return (int32_t)(p - pBuf);
}

uint32_t PersistColumn::WriteData(Byte *pBuf) {
  Byte *p = pBuf;

  uint32_t len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  name_ = string((char *)p, len);
  p += len;

  position_ = *((uint32_t *)p);
  p += sizeof(uint32_t);

  dataType_ = (DataType) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  bNullable_ = (*p == 0);
  p++;

  maxLength_ = *((uint32_t *)p);
  p += sizeof(uint32_t);

  incStep_ = *((uint32_t *)p);
  p += sizeof(uint32_t);

  charset_ = (utils::Charsets) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  comments_ = string((char *)p, len);
  p += len;

  bool bDefault = (*p != 0);
  p++;
  if (bDefault) {
    switch (dataType_) {
    case DataType::LONG:
      pDefaultVal_ = new DataValueLong(*((int64_t *)p));
      p += sizeof(uint64_t);
      break;
    case DataType::VARCHAR:
      len = (*(int32_t *)p);
      p += sizeof(uint32_t);
      pDefaultVal_ = new DataValueVarChar((char *)p);
      p += len;
      break;
    case DataType::FIXCHAR:
      if (p[maxLength_ - 1] == '\0')
        pDefaultVal_ = new DataValueFixChar((char *)p);
      else
        pDefaultVal_ = new DataValueFixChar((char *)p, maxLength_);
      p += maxLength_;
    default:
      break;
    }
  }

  return (uint32_t)(p - pBuf);
}
} // namespace storage
