#pragma once
#include "RawRecord.h"
#include "LeafPage.h"
#include "../dataType/IDataValue.h"
#include <cstring>

namespace storage {
  static const Byte LAST_RECORD_OVERFLOW = 0x80;
  static const Byte RECORD_OVERFLOW = 0x40;
  static const Byte RECORD_DELETED = 0x20;

  struct PriValStruct {
  public:
    PriValStruct() {
      std::memset(this, 0, sizeof(PriValStruct));
    }

    PriValStruct(Byte* bys) {
      std::memset(this, 0, sizeof(PriValStruct));
      uint16_t pos = sizeof(uint16_t) * 2 + *(uint16_t*)&bys[sizeof(uint16_t)];
      bLastOvf = bys[pos] & LAST_RECORD_OVERFLOW;
      bOvf = bys[pos] & RECORD_OVERFLOW;
      bDelete = bys[pos] & RECORD_DELETED;
      verCount = bys[pos] & 0xf;
      pos++;
      arrStamp = (uint64_t*)&bys[pos];
      pos += sizeof(uint64_t) * verCount;

      if (bOvf) {
        ovfOffset = *(uint64_t*)&bys[pos];
        pos += sizeof(uint64_t);
        ovfRange = *(uint32_t*)&bys[pos];
        pos += sizeof(uint32_t);
        indexOvfStart = *(uint16_t*)&bys[pos];
        pos += sizeof(uint16_t);
        lenInPage = *(uint16_t*)&bys[pos];
        pos += sizeof(uint16_t);
        arrOvfLen = (uint32_t*)&bys[pos];
        pageValOffset = pos + sizeof(uint32_t) * verCount;
      }
      else {
        arrPageLen = (uint16_t*)&bys[pos];
        pageValOffset = pos + sizeof(uint16_t) * verCount;
      }
    }

    static void* operator new(size_t size)
    {
      return CachePool::Apply((uint32_t)size);
    }
    static void operator delete(void* ptr, size_t size)
    {
      CachePool::Release((Byte*)ptr, (uint32_t)size);
    }

  public:
    bool bLastOvf;
    bool bOvf;
    bool bDelete;//If the last snapshot has been deleted
    Byte verCount;
    uint16_t pageValOffset;
    uint64_t* arrStamp;
    union {
      uint16_t* arrPageLen;
      struct {
        uint64_t ovfOffset;
        uint32_t ovfRange;
        uint16_t indexOvfStart;
        uint16_t lenInPage;
        uint32_t* arrOvfLen;
      };
    };

  };


  class LeafPage;
  class LeafRecord : public RawRecord
  {
  protected:
    ~LeafRecord();

  public:
    LeafRecord(LeafPage* indexPage, Byte* bys);
    LeafRecord(IndexTree* indexTree, Byte* bys);
    LeafRecord(const LeafRecord& src) = delete;
    /**Constructor for not primary key*/
    LeafRecord(IndexTree* indexTree, const VectorDataValue& vctKey, Byte* bysPri, uint32_t lenPri,
      ActionType type = ActionType::UNKNOWN, uint64_t tranId = 0, LeafRecord* old = nullptr);
    LeafRecord(IndexTree* indexTree, const VectorDataValue& vctKey, const VectorDataValue& vctVal,
      uint64_t recStamp, ActionType type = ActionType::UNKNOWN, uint64_t tranId = 0, LeafRecord* old = nullptr);
    void CleanUndoRecord();
    /**To calc the length of snapshot for multi versions' record. To add last version record or not,
    decide by its record stamp < last version stamp in head page or not*/
    uint32_t GetSnapshotLength() const;

    inline void ReleaseRecord() {
      _refCount--;
      if (_refCount == 0)
        delete this;
    }
    inline LeafRecord* ReferenceRecord() { _refCount++; return this; }
    inline ActionType GetActionType() { return _actionType; }
    inline uint64_t GetTrasactionId() { return _tranId; }
    inline bool IsTranFinished() { return _bTranFinished; }
    inline void SetTranFinished(bool bFinished) { _bTranFinished = bFinished; }

    void GetListKey(VectorDataValue& vct) const;
    /**If passed to get values, return true, or false*/
    bool GetListValue(VectorDataValue& vct, uint64_t verStamp = UINT64_MAX) const;
    void GetListOverflow(vector<IDataValue*>& vctVal) const;

    int CompareTo(const LeafRecord& lr) const;
    int CompareKey(const RawKey& key) const;
    int CompareKey(const LeafRecord& lr) const;
    RawKey* GetKey() const;

    inline uint16_t GetValueLength() const override {
      return (*((uint16_t*)_bysVal) - *((uint16_t*)(_bysVal + sizeof(uint16_t))) - sizeof(uint16_t) * 2);
    }

    inline uint16_t GetIndexOvfStart() const {
      return (GetPriValStruct()->bLastOvf) ? GetPriValStruct()->indexOvfStart : UINT16_MAX;
    }

    inline uint16_t SaveData(Byte* bysPage) {
      uint16_t len = GetTotalLength();
      std::memcpy(bysPage, _bysVal, len);
      return len;
    }

    inline PriValStruct* GetPriValStruct() const {
      if (_priStru == nullptr) {
        _priStru = new PriValStruct(_bysVal);
      }

      return _priStru;
    }
  protected:
    /**Save old records for recover*/
    LeafRecord* _undoRecords;
    /**The transaction id if it is running in a transaction, or UINT64_MAX*/
    uint64_t _tranId;
    mutable PriValStruct* _priStru = nullptr;

    friend std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
  };

  std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
  class VectorLeafRecord : public vector<LeafRecord*> {
  public:
    using vector::vector;
    ~VectorLeafRecord() {
      for (auto iter = begin(); iter != end(); iter++) {
        (*iter)->ReleaseRecord();
      }
    }

    void RemoveAll() {
      for (auto iter = begin(); iter != end(); iter++) {
        (*iter)->ReleaseRecord();
      }

      clear();
    }
  };
}
