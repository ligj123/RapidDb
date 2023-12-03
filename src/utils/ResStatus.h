#pragma once
#include "../header.h"
#include <ostream>

namespace storage {
// Resource status
//  Every resource like page, table or other should has one status from create
//  to obsolete and recycle. To avoid lock, it should change one status to other
//  status and not reback old status. At the same time, the program should
//  ensure only one thread do this change. If can not make this assurance,
//  please add atomic lock to avoid data consistency problem.
enum class ResStatus : uint8_t {
  Uninit,    // Just create and does not initialize it.
  Preparing, // This resource is loading or parpare data, can not supply
             // service.
  Valid,     // This resource is in working status and can supply service
  Obsolete,  // This resource is obsolete and will be free in following time. It
             // can not offer service again.
  Invalid // The resource is invalid and need to fix. In this period, it can not
          // supply service.
};

inline std::ostream &operator<<(std::ostream &os, const ResStatus status) {
  switch (status) {
  case ResStatus::Uninit:
    os << "Uninit(" << (int)ResStatus::Uninit << ")";
    break;
  case ResStatus::Preparing:
    os << "Preparing(" << (int)ResStatus::Preparing << ")";
    break;
  case ResStatus::Valid:
    os << "Valid(" << (int)ResStatus::Valid << ")";
    break;
  case ResStatus::Obsolete:
    os << "Obsolete(" << (int)ResStatus::Obsolete << ")";
    break;
  case ResStatus::Invalid:
    os << "Invalid(" << (int)ResStatus::Invalid << ")";
    break;
  default:
    os << "UNKNOWN ResourceStatus(" << (int)status << ")";
    break;
  }
  return os;
}

} // namespace storage
