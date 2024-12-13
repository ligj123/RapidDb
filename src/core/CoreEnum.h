#pragma once
#include "../header.h"
#include <cassert>
#include <ostream>

namespace storage {
enum class IndexType : int8_t {
  UNKNOWN = 0,
  PRIMARY,
  HIDE_PRIMARY,
  UNIQUE,
  NON_UNIQUE
};

enum class PageType : uint8_t {
  UNKNOWN = 0,
  HEAD_PAGE,
  LEAF_PAGE,
  BRANCH_PAGE,
  OVERFLOW_PAGE,       // LeafRecord save overflow data to this type pages.
  GARBAGE_FREE_PAGE,   // free garbage page
  GARBAGE_COLLECT_PAGE // The page to collect and manage grabage page.
};

enum class PageStatus : uint8_t {
  // Not load data from disk
  EMPTY = 0,
  // This page has added reading queue and wait to read
  READING,
  // This page has just finished to read and need to sync memory
  READED,
  // This page has added writing queue and wait to write
  WRITING,
  // The page has been loaded and the data is valid
  VALID,
  // This page has marked as obsolete and can not be visit again.
  OBSOLETE,
  // The page has been loaded and failed the crc32 verify, need to fix
  INVALID
};

// How to operate the record
enum ActionType : uint8_t {
  NO_ACTION = 0,      // No Lock for this record
  READ_SHARE = 0x1,   // Read with read lock
  READ_UPDATE = 0x10, // Read with write lock
  INSERT = 0x20,      // Insert this record with write lock
  UPDATE = 0x30,      // Update this record with write lock
  DELETE = 0x40,      // Delete this record with write lock
  UPSERT = 0x80,      // Insert if not exist or update with write lock

  WRITE_LOCK_MASK = 0xF0, // The mask to know if it has write lock
  UPDATE_MASK = 0xE0      // The mask of Insert,Update,Delete
};

// The record's status
enum class RecordStatus : uint8_t {
  INIT = 0,   // The record has been created and waitting to commit or abort
  LOCK_ONLY,  // Only lock current record without update, ActionType=QUERY_SHARE
              // or QUERY_UPDATE
  COMMITED,   // The transaction has commited, only valid for WriteLock
  ROLLBACKED, // The current statement has rollbacked, and it is not effect
              // previous statements in same transaction. Only valid for
              // WriteLock
  FREEED      // The lock has been freed from LOCK_ONLY status.
};

enum class RecordResult : uint8_t {
  INIT = 0, // The record just create and not insert into page
  IN_PAGE,  // The record has inserted into page successfully
  ERROR     // The record failed to insert into page due to error
};

/**The result to read list value*/
enum class ReadResult : int8_t {
  // Passed to read values and does not add lock for current read.
  OK_NOLOCK = 0,
  // Passed to read values and added lock for current read, need to release it.
  OK_LOCK,
  // Failed to read values due to it has been locked by other transaction;
  LOCKED,
  // The record has been deleted
  REC_DELETE,
  // No version to fit and failed to read
  INVALID_VERSION
};

// The status to release a lock from the record
enum class ReleaseResult {
  FISHED = 0, // The record has been commited or rollbacked, it still has data.
  DELETED,    // The record has been deleted and need to remove from page.
  UNFINISH // Only the last version is rollbacked and there still has uncommited
           // statement.
};

inline std::ostream &operator<<(std::ostream &os, const IndexType &it) {
  switch (it) {
  case IndexType::PRIMARY:
    os << "PRIMARY(" << (int)IndexType::PRIMARY << ")";
    break;
  case IndexType::HIDE_PRIMARY:
    os << "HIDEPRIMARY(" << (int)IndexType::HIDE_PRIMARY << ")";
    break;
  case IndexType::UNIQUE:
    os << "UNIQUE(" << (int)IndexType::UNIQUE << ")";
    break;
  case IndexType::NON_UNIQUE:
    os << "NON_UNIQUE(" << (int)IndexType::NON_UNIQUE << ")";
    break;
  case IndexType::UNKNOWN:
  default:
    os << "UNKNOWN(" << (int)IndexType::UNKNOWN << ")";
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const PageType &type) {
  switch (type) {
  case PageType::HEAD_PAGE:
    os << "HEAD_PAGE(" << (int)PageType::HEAD_PAGE << ")";
    break;
  case PageType::LEAF_PAGE:
    os << "LEAF_PAGE(" << (int)PageType::LEAF_PAGE << ")";
    break;
  case PageType::BRANCH_PAGE:
    os << "BRANCH_PAGE(" << (int)PageType::BRANCH_PAGE << ")";
    break;
  case PageType::OVERFLOW_PAGE:
    os << "OVERFLOW_PAGE(" << (int)PageType::OVERFLOW_PAGE << ")";
    break;
  case PageType::GARBAGE_FREE_PAGE:
    os << "GARBAGE_FREE_PAGE(" << (int)PageType::GARBAGE_FREE_PAGE << ")";
    break;
  case PageType::GARBAGE_COLLECT_PAGE:
    os << "GARBAGE_COLLECT_PAGE(" << (int)PageType::GARBAGE_COLLECT_PAGE << ")";
    break;
  case PageType::UNKNOWN:
    os << "UNKNOWN(" << (int)PageType::UNKNOWN << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const PageStatus &status) {
  switch (status) {
  case PageStatus::EMPTY:
    os << "EMPTY(" << (int)PageStatus::EMPTY << ")";
    break;
  case PageStatus::READING:
    os << "READING(" << (int)PageStatus::READING << ")";
    break;
  case PageStatus::READED:
    os << "READED(" << (int)PageStatus::READED << ")";
    break;
  case PageStatus::WRITING:
    os << "WRITING(" << (int)PageStatus::WRITING << ")";
    break;
  case PageStatus::VALID:
    os << "VALID(" << (int)PageStatus::VALID << ")";
    break;
  case PageStatus::OBSOLETE:
    os << "OBSOLETE(" << (int)PageStatus::OBSOLETE << ")";
    break;
  case PageStatus::INVALID:
    os << "INVALID(" << (int)PageStatus::INVALID << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ActionType &type) {
  switch (type) {
  case ActionType::NO_ACTION:
    os << "NO_ACTION(" << (int)ActionType::NO_ACTION << ")";
    break;
  case ActionType::READ_SHARE:
    os << "READ_SHARE(" << (int)ActionType::READ_SHARE << ")";
    break;
  case ActionType::READ_UPDATE:
    os << "READ_UPDATE(" << (int)ActionType::READ_UPDATE << ")";
    break;
  case ActionType::INSERT:
    os << "INSERT(" << (int)ActionType::INSERT << ")";
    break;
  case ActionType::UPDATE:
    os << "UPDATE(" << (int)ActionType::UPDATE << ")";
    break;
  case ActionType::DELETE:
    os << "DELETE(" << (int)ActionType::DELETE << ")";
    break;
  case ActionType::WRITE_LOCK_MASK:
    os << "WRITE_LOCK_MASK(" << (int)ActionType::WRITE_LOCK_MASK << ")";
    break;
  case ActionType::UPDATE_MASK:
    os << "UPDATE_MASK(" << (int)ActionType::UPDATE_MASK << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const RecordStatus &status) {
  switch (status) {
  case RecordStatus::INIT:
    os << "INIT(" << (int)RecordStatus::INIT << ")";
    break;
  case RecordStatus::LOCK_ONLY:
    os << "LOCK_ONLY(" << (int)RecordStatus::LOCK_ONLY << ")";
    break;
  case RecordStatus::COMMITED:
    os << "COMMITED(" << (int)RecordStatus::COMMITED << ")";
    break;
  case RecordStatus::ROLLBACKED:
    os << "ROLLBACKED(" << (int)RecordStatus::ROLLBACKED << ")";
    break;
  case RecordStatus::FREEED:
    os << "FREEED(" << (int)RecordStatus::FREEED << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ReadResult &result) {
  switch (result) {
  case ReadResult::OK_NOLOCK:
    os << "OK_NOLOCK(" << (int)ReadResult::OK_NOLOCK << ")";
    break;
  case ReadResult::OK_LOCK:
    os << "OK_LOCK(" << (int)ReadResult::OK_LOCK << ")";
    break;
  case ReadResult::LOCKED:
    os << "LOCKED(" << (int)ReadResult::LOCKED << ")";
    break;
  case ReadResult::REC_DELETE:
    os << "REC_DELETE(" << (int)ReadResult::REC_DELETE << ")";
    break;
  case ReadResult::INVALID_VERSION:
    os << "INVALID_VERSION(" << (int)ReadResult::INVALID_VERSION << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ReleaseResult &res) {
  switch (res) {
  case ReleaseResult::FISHED:
    os << "FISHED(" << (int)ReleaseResult::FISHED << ")";
    break;
  case ReleaseResult::DELETED:
    os << "DELETED(" << (int)ReleaseResult::DELETED << ")";
    break;
  case ReleaseResult::UNFINISH:
    os << "UNFINISH(" << (int)ReleaseResult::UNFINISH << ")";
    break;
  default:
    assert(false);
    break;
  }

  return os;
}

} // namespace storage
