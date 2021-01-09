#include "IndexTree.h"

namespace storage {
  vector<IDataValue*>& IndexTree::CloneKeys()
  {
    vector<IDataValue*> vct(_vctKey.size());
    for (IDataValue* dv : _vctKey) {
      vct.push_back(dv->CloneDataValue(false));
    }

    return vct;
  }

  vector<IDataValue*>& IndexTree::CloneValues()
  {
    vector<IDataValue*> vct(_vctValue.size());
    for (IDataValue* dv : _vctValue) {
      vct.push_back(dv->CloneDataValue(false));
    }

    return vct;
  }
}