#pragma once
#include "../cache/Mallocator.h"
#include "../result/CacheResultSet.h"
#include "../result/IResultSet.h"

namespace storage {
enum class ResultStatus : uint8_t {
  INIT = 0, // Just initiate, wait to fill result
  FILLING, // For large query, it need to split the result into several parts to
           // fill
  FINISHED // Have filled all result into it.
};

struct StmtResult {
public:
  StmtResult() {}
  ~StmtResult() {
    if (_resultSet != nullptr) {
      delete _resultSet;
    }
  }
  /**
   * @brief To serialize the content to byte buffer
   * @param bys The buffer to save contents.
   * @param len The buffer's total length.
   * @return The bytes have used if passed, or UINT32_MAX if failed
   */
  uint32_t Serialize(Byte *bys, uint32_t len) { return 0; }
  /**
   * @brief To deserialize the conetent to variables
   * @param bys The buffer saved the contents
   * @param len The total length of buffer.
   * @return The actual data length to parsed if passed, or UINT32_MAX if failed
   */
  uint32_t Deserialize(Byte *bys, uint32_t len) { return 0; }

  void SetRowNum(uint32_t num) { _rowNum = num; }
  uint32_t GetRowNum() { return _rowNum; }

  void SetResultStatus(ResultStatus s) {
    _status.store(s, memory_order_release);
  }

  ResultStatus GetResultStatus() { return _status.load(memory_order_relaxed); }

  void Reset() {
    _rowNum = 0;
    _vctError.clear();
    _vctWarning.clear();
    if (_resultSet != nullptr) {
      delete _resultSet;
      _resultSet = nullptr;
    }
    _status.store(ResultStatus::INIT, memory_order_release);
  }

public:
  atomic<ResultStatus> _status{ResultStatus::INIT};
  bool _bFailed{false};
  // The id of session that result belong to
  uint32_t _sessionId;
  // The result id, start from 0, every time increase 1 in this session.
  uint32_t _stmtId;
  // Total rows affected or returned
  uint64_t _rowNum{0};
  // The error information
  MVector<MString> _vctError;
  // The vector of warnings
  MVector<MString> _vctWarning;
  // The result set for query,or nulpptr for other statement.
  IResultSet *_resultSet{nullptr};
};

inline std::ostream &operator<<(std::ostream &os, const ResultStatus &s) {
  switch (s) {
  case ResultStatus::INIT:
    os << "INIT(" << (int)ResultStatus::INIT << ")";
    break;
  case ResultStatus::FILLING:
    os << "FILLING(" << (int)ResultStatus::FILLING << ")";
    break;
  case ResultStatus::FINISHED:
    os << "FINISHED(" << (int)ResultStatus::FINISHED << ")";
    break;
  default:
    os << "ERROR ResultStatus(" << (int)s << ")";
    break;
  }

  return os;
}
} // namespace storage