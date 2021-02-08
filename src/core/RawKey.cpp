#include "RawKey.h"
#include <iomanip>

namespace storage {
	 RawKey::RawKey(VectorDataValue& vctKey) : _bSole(true) {
		_length = 0;
		for (size_t i = 0; i < vctKey.size(); i++) {
			if (vctKey[i]->IsNull()) {
				vctKey[i]->SetDefaultValue();
			}

			_length += vctKey[i]->GetPersistenceLength(true);
		}

		_bysVal = CachePool::ApplyBys(_length);

		int pos = 0;
		for (int i = 0; i < vctKey.size(); i++) {
			pos += vctKey[i]->WriteData(_bysVal + pos, true);
		}
	}

	RawKey::RawKey(Byte* bys, uint32_t len) : _bysVal(bys), _length(len), _bSole(false)
	{ }

	RawKey::~RawKey()
	{
		if (_bSole) CachePool::ReleaseBys(_bysVal, _length);
	}

	bool RawKey::operator > (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return hr > 0;
		return (_length > key._length);
	}
	bool RawKey::operator < (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return hr < 0;
		return (_length < key._length);
	}

	bool RawKey::operator >= (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return hr >= 0;
		return (_length >= key._length);
	}

	bool RawKey::operator <= (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return hr < 0;
		return (_length <= key._length);
	}

	bool RawKey::operator == (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return false;
		return (_length == key._length);
	}

	bool RawKey::operator != (const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return true;
		return (_length != key._length);
	}

	int RawKey::CompareTo(const RawKey& key) const
	{
		int hr = std::memcmp(_bysVal, key._bysVal, std::min(_length, key._length));
		if (hr != 0) return hr;
		return (_length - key._length);
	}

	std::ostream& operator<< (std::ostream& os, const RawKey& key)
	{
		os << "Length=" << key._length << std::uppercase << std::hex << std::setfill('0') << "\tKey=0x";
		for (uint32_t i = 0; i < key._length; i++) {
			os << std::setw(2) << key._bysVal[i];
			if (i % 4 == 0) os << ' ';
		}

		return os;
	}
}