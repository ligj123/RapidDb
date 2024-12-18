#include "../cache/Mallocator.h"
#include "../result/IResultSet.h"

namespace storage {
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

public:
  // The od of session that result belong to
  uint32_t _sessionId;
  // The result id, start from 0, every time increase 1 in this session.
  uint32_t _resId;
  // The expression id of this statement
  int32_t _exprId;
  // Total rows affected or returned
  uint32_t _rowNum{0};
  // The error information
  MVector<MString> _vctError;
  // The vector of warnings
  MVector<MString> _vct_warning;
  // The result set for query,or nulpptr for other statement.
  IResultSet *_resultSet{nullptr};
};
} // namespace storage