#include <string>

namespace storage {
extern const std::string ROOT_PATH;

uint32_t GenTestPrimaryKey(uint32_t num) {
  uint64_t by1 = num & 0xff;
  uint64_t by2 = (num >> 8) & 0xff;
  uint64_t by3 = (num >> 16) & 0xff;
  uint64_t by4 = (num >> 24) & 0xff;
  return (((by1 & 0x55) + (by4 & 0xAA)) << 24) +
         (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
         (((by2 & 0xAA) + (by3 & 0x55)) << 8) + (((by1 & 0xAA) + (by4 & 0x55)));
}
} // namespace storage