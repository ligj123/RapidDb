#include "Session.h"

namespace storage {
bool Session::CreateStatement(MString sql, VectorRow &paras) { return false; }

uint64_t Session::PrepareStatement(MString sql) { return 0; }

bool Session::AddStatement(uint64_t sid, VectorRow &paras) { return false; }

} // namespace storage