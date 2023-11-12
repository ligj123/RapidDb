
#include "../header.h"
#include <ostream>

namespace storage {
enum class ResourceStatus : uint8_t {
  Uninit,    // Uninitialized
  Preparing, // This resource is loading or fixing, can not offer service.
  Normal,    // This resource is in working normal status
  Droped // This resource has been droped and it can not offer service again. It
         // will be freed in following time.
};

inline std::ostream &operator<<(std::ostream &os, const ResourceStatus &type) {
  switch (type) {
  case ResourceStatus::Uninit:
    os << "Uninit(" << (int)ResourceStatus::Uninit << ")";
    break;
  case ResourceStatus::Preparing:
    os << "Preparing(" << (int)ResourceStatus::Preparing << ")";
    break;
  case ResourceStatus::Normal:
    os << "Normal(" << (int)ResourceStatus::Normal << ")";
    break;
  case ResourceStatus::Droped:
    os << "Droped(" << (int)ResourceStatus::Droped << ")";
    break;
  default:
    os << "UNKNOWN ResourceStatus";
    break;
  }
  return os;
}

} // namespace storage
