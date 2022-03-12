#pragma once
#include "CachePage.h"

namespace storage {
class GarbagePage : public CachePage {
public:
  static const uint16_t PAGE_TYPE_OFFSET;

protected:
};
} // namespace storage