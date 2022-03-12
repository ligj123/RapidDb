#pragma once
#include <ostream>

namespace storage {
enum class LogType : uint8_t {
  UNKNOWN = 0,
  REC_INSERT, // Insert log type
  REC_UPDATE, //
  REC_DELETE,
  PAGE_SPLIT, //
  TABLE_CREATE,
  TABLE_DELETE,
  COLUMN_ADD,
  COLUMN_DELETE,
  SECOND_INDEX_CREATE,
  SECOND_INDEX_DELETE,
  STAMP_ADD,
  STAMP_REMOVE,
  TRAN_START,
  TRAN_END
};

inline std::ostream &operator<<(std::ostream &os, const LogType &type) {
  switch (type) {
  case LogType::REC_INSERT:
    os << "REC_INSERT(" << (int)LogType::REC_INSERT << ")";
    break;
  case LogType::REC_UPDATE:
    os << "REC_UPDATE(" << (int)LogType::REC_UPDATE << ")";
    break;
  case LogType::REC_DELETE:
    os << "DELETE(" << (int)LogType::REC_DELETE << ")";
    break;
  case LogType::PAGE_SPLIT:
    os << "PAGE_SPLIT(" << (int)LogType::PAGE_SPLIT << ")";
    break;
  case LogType::TABLE_CREATE:
    os << "TABLE_CREATE(" << (int)LogType::TABLE_CREATE << ")";
    break;
  case LogType::TABLE_DELETE:
    os << "TABLE_DELETE(" << (int)LogType::TABLE_DELETE << ")";
    break;
  case LogType::COLUMN_ADD:
    os << "COLUMN_ADD(" << (int)LogType::COLUMN_ADD << ")";
    break;
  case LogType::COLUMN_DELETE:
    os << "COLUMN_DELETE(" << (int)LogType::COLUMN_DELETE << ")";
    break;
  case LogType::SECOND_INDEX_CREATE:
    os << "SECOND_INDEX_CREATE(" << (int)LogType::SECOND_INDEX_CREATE << ")";
    break;
  case LogType::SECOND_INDEX_DELETE:
    os << "SECOND_INDEX_DELETE(" << (int)LogType::SECOND_INDEX_DELETE << ")";
    break;
  case LogType::STAMP_ADD:
    os << "STAMP_ADD(" << (int)LogType::STAMP_ADD << ")";
    break;
  case LogType::TRAN_END:
    os << "TRAN_END(" << (int)LogType::TRAN_END << ")";
    break;
  case LogType::TRAN_START:
    os << "TRAN_START(" << (int)LogType::TRAN_START << ")";
    break;
  case LogType::STAMP_REMOVE:
    os << "STAMP_REMOVE(" << (int)LogType::STAMP_REMOVE << ")";
    break;
  case LogType::UNKNOWN:
    os << "UNKNOWN(" << (int)LogType::UNKNOWN << ")";
    break;
  default:
    os << "ERROR LogType(" << (int)type << ")";
    break;
  }

  return os;
}
} // namespace storage