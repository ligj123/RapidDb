#include "RawKey.h"

namespace storage {
	 RawKey::RawKey(vector<IDataValue> vctKey) : _bSole(false) {
		_length = 0;
		for (size_t i = 0; i < vctKey.size(); i++) {
			if (vctKey[i].IsNull()) {
				vctKey[i].SetDefaultValue();
			}

			_length += vctKey[i].GetPersistenceLength();
		}

		_bysVal = CachePool::ApplyBys(_length);

		int pos = 0;
		for (int i = 0; i < vctKey.size(); i++) {
			pos += vctKey[i].WriteData(_bysVal + pos, true);
		}
	}

	RawKey::RawKey(Byte* bys, uint32_t len) : _bysVal(bys), _length(len), _bSole(false)
	{ }

	RawKey::~RawKey()
	{
		if (_bSole) CachePool::ReleaseBys(_bysVal, _length);
	}
}