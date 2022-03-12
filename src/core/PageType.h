#pragma once
#include <ostream>

namespace storage {
enum class PageType : uint8_t {
  UNKNOWN = 0,
  HEAD_PAGE,
  LEAF_PAGE,
  BRANCH_PAGE,
  OVERFLOW_PAGE,
  GARBAGE_PAGE
};

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
  case PageType::GARBAGE_PAGE:
    os << "GARBAGE_PAGE(" << (int)PageType::GARBAGE_PAGE << ")";
    break;
  case PageType::UNKNOWN:
  default:
    os << "UNKNOWN(" << (int)PageType::UNKNOWN << ")";
    break;
  }

  return os;
}
} // namespace storage
