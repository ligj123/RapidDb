#pragma once
#include <ostream>

namespace storage {
enum class PageType : uint8_t {
  UNKNOWN = 0,
  HEAD_PAGE,
  LEAF_PAGE,
  BRANCH_PAGE,
  OVERFLOW_PAGE,       // LeafRecord save overflow data to this type pages.
  GARBAGE_FREE_PAGE,   // free garbage page
  GARBAGE_COLLECT_PAGE // The page to collect and manage grabage page.
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
    os << "UNKNOWN(" << (int)type << ")";
    break;
  }

  return os;
}
} // namespace storage
