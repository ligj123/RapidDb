#include "FilePagePool.h"

namespace storage {
FilePagePool *FilePagePool::_pool{nullptr};

void FilePagePool::Start() {}

void FilePagePool::Stop() {}

void FilePagePool::Run() {}

#ifdef LINUX_OS
void FilePagePool::InitHandle() {}
void FilePagePool::RWPage(MDeque<CachePage *> &readQueue,
                          MDeque<CachePage *> &writeQueue) {}
#endif
} // namespace storage