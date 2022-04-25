#pragma once
#include "../cache/CachePool.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include <cstdint>
#include <cstring>

namespace storage
{
  class RawKey
  {
  public:
    RawKey() : _bysVal(nullptr), _bSole(false) {}
    RawKey(VectorDataValue &vctKey) : _bSole(true)
    {
      uint16_t len = 0;
      for (size_t i = 0; i < vctKey.size(); i++)
      {
        len += vctKey[i]->GetPersistenceLength(true);
      }

      _bysVal = CachePool::Apply(len + 2);
      *(uint16_t *)_bysVal = len;

      int pos = 2;
      for (int i = 0; i < vctKey.size(); i++)
      {
        pos += vctKey[i]->WriteData(_bysVal + pos, true);
      }
    }

    RawKey(Byte *bys, bool sole = false) : _bysVal(bys), _bSole(sole) {}

    ~RawKey()
    {
      if (_bSole)
        CachePool::Release(_bysVal, Length() + 2);
    }

    Byte *GetBysVal() const { return _bysVal; }
    uint16_t Length() const
    {
      return _bysVal == nullptr ? 0 : *(uint16_t *)_bysVal;
    }

    static void *operator new(size_t size)
    {
      return CachePool::Apply((uint32_t)size);
    }

    static void operator delete(void *ptr, size_t size)
    {
      CachePool::Release((Byte *)ptr, (uint32_t)size);
    }

    bool operator>(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) > 0;
    }
    bool operator<(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) < 0;
    }
    bool operator>=(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) >= 0;
    }
    bool operator<=(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) <= 0;
    }
    bool operator==(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) == 0;
    }
    bool operator!=(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length()) != 0;
    }
    int CompareTo(const RawKey &key) const
    {
      return BytesCompare(_bysVal + UI16_LEN, Length(), key._bysVal + UI16_LEN, key.Length());
    }

  protected:
    Byte *_bysVal;
    bool _bSole;

    friend std::ostream &operator<<(std::ostream &os, const RawKey &key);
  };

  inline std::ostream &operator<<(std::ostream &os, const RawKey &key)
  {
    os << "Length=" << key.Length() << std::uppercase << std::hex
       << std::setfill('0') << "\tKey=0x";
    for (uint32_t i = 0; i < key.Length(); i++)
    {
      os << std::setw(2) << key._bysVal[i];
      if (i % 4 == 0)
        os << ' ';
    }

    return os;
  }

  class VectorRawKey : public MVector<RawKey *>::Type
  {
  public:
    using vector::vector;
    ~VectorRawKey()
    {
      for (auto iter = begin(); iter != end(); iter++)
      {
        delete (*iter);
      }
    }

    void RemoveAll()
    {
      for (auto iter = begin(); iter != end(); iter++)
      {
        delete (*iter);
      }

      clear();
    }
  };
} // namespace storage
