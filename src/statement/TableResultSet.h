﻿#pragma once
#include "../table/Table.h"
#include "IResultSet.h"

namespace storage {
class TableResultSet : public IResultSet {
protected:
  TempTable _table;
};
} // namespace storage
