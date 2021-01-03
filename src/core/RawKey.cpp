#include "RawKey.h"

namespace storage {
	 RawKey::RawKey(vector<IDataValue> vctKey) {
		startPos = 0;
		length = 0;
		for (size_t i = 0; i < vctKey.size(); i++) {
			if (vctKey[i].IsNull()) {
				vctKey[i].SetDefaultValue();
			}

			length += vctKey[i].GetPersistenceLength();
		}

		key.bysVal = ObjectPool.applyByteArray(key.length);

		int pos = 0;
		for (int i = 0; i < arKey.length; i++) {
			pos += arKey[i].writeData(key.bysVal, pos, true);
		}

		return key;
	}

	RawKey init(byte[] bys, int start, int len) {
		RawKey key = ObjectPool.applyRawKey();
		key.bysVal = bys;
		key.startPos = start;
		key.length = len;
		return key;
	}
}