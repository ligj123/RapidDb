#include "HeadPage.h"
#include "IndexTree.h"
#include "PageType.h"

namespace storage {
const uint16_t HeadPage::MAX_RECORD_VERSION_COUNT = 8;

const uint16_t HeadPage::PAGE_TYPE_OFFSET = 0;
const uint16_t HeadPage::INDEX_TYPE_OFFSET = 1;
const uint16_t HeadPage::RECORD_VERSION_COUNT_OFFSET = 2;
const uint16_t HeadPage::FILE_VERSION_OFFSET = 4;
const uint16_t HeadPage::KEY_ALTERABLE_FIELD_COUNT_OFFSET = 8;
const uint16_t HeadPage::VALUE_ALTERABLE_FIELD_COUNT_OFFSET = 10;
const uint16_t HeadPage::GARBAGE_PAGE_OFFSET = 12;
const uint16_t HeadPage::GARBAGE_SAVE_PAGES_NUM_OFFSET = 16;
const uint16_t HeadPage::GRABAGE_TOTAL_ITEMS_OFFSET = 20;
const uint16_t HeadPage::GARBAGE_CRC32_OFFSET = 24;
const uint16_t HeadPage::TOTAL_PAGES_COUNT_OFFSET = 28;
const uint16_t HeadPage::ROOT_PAGE_OFFSET = 32;
const uint16_t HeadPage::BEGIN_LEAF_PAGE_OFFSET = 36;
const uint16_t HeadPage::END_LEAF_PAGE_OFFSET = 40;
const uint16_t HeadPage::TOTAL_RECORD_COUNT_OFFSET = 48;
const uint16_t HeadPage::AUTO_INCREMENT_KEY = 56;
const uint16_t HeadPage::AUTO_INCREMENT_KEY2 = 64;
const uint16_t HeadPage::AUTO_INCREMENT_KEY3 = 72;
const uint16_t HeadPage::CURRENT_RECORD_STAMP_OFFSET = 80;
const uint16_t HeadPage::RECORD_VERSION_STAMP_OFFSET = 128;

void HeadPage::ReadPage(PageFile *pageFile) {
  lock_guard<SpinMutex> lock(_spinMutex);
  CachePage::ReadPage(pageFile);
  assert((PageType)ReadByte(PAGE_TYPE_OFFSET) == PageType::HEAD_PAGE);
  assert(CURRENT_FILE_VERSION == ReadFileVersion());

  _indexType = (IndexType)ReadByte(INDEX_TYPE_OFFSET);
  _keyAlterableFieldCount = ReadShort(KEY_ALTERABLE_FIELD_COUNT_OFFSET);
  _valueAlterableFieldCount = ReadShort(VALUE_ALTERABLE_FIELD_COUNT_OFFSET);

  _totalPageCount.store(ReadInt(TOTAL_PAGES_COUNT_OFFSET),
                        memory_order_relaxed);
  _rootPageId.store(ReadInt(ROOT_PAGE_OFFSET), memory_order_relaxed);
  _beginLeafPageId.store(ReadInt(BEGIN_LEAF_PAGE_OFFSET), memory_order_relaxed);
  _endLeafPageId.store(ReadInt(END_LEAF_PAGE_OFFSET), memory_order_relaxed);
  _totalRecordCount.store(ReadLong(TOTAL_RECORD_COUNT_OFFSET),
                          memory_order_relaxed);
  _currRecordStamp.store(ReadLong(CURRENT_RECORD_STAMP_OFFSET),
                         memory_order_relaxed);
  _autoIncrementKey1.store(ReadLong(AUTO_INCREMENT_KEY), memory_order_relaxed);
  _autoIncrementKey2.store(ReadLong(AUTO_INCREMENT_KEY2), memory_order_relaxed);
  _autoIncrementKey3.store(ReadLong(AUTO_INCREMENT_KEY3), memory_order_relaxed);

  if (_indexType == IndexType::PRIMARY) {
    Byte n = ReadByte(RECORD_VERSION_COUNT_OFFSET) * 2;
    for (Byte i = 0; i < n;) {
      uint64_t tm =
          ReadLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i++);
      uint64_t ver =
          ReadLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i++);
      _mapVerStamp.insert({tm, ver});
      _setVerStamp.insert(ver);
    }
  }

  _pageStatus = PageStatus::VALID;
}

void HeadPage::WritePage(PageFile *pageFile) {
  lock_guard<SpinMutex> lock(_spinMutex);
  WriteByte(PAGE_TYPE_OFFSET, (Byte)PageType::HEAD_PAGE);
  WriteByte(RECORD_VERSION_COUNT_OFFSET, (Byte)_mapVerStamp.size());
  WriteLong(TOTAL_PAGES_COUNT_OFFSET,
            _totalPageCount.load(memory_order_relaxed));
  WriteFileVersion();
  WriteLong(ROOT_PAGE_OFFSET, _rootPageId.load(memory_order_relaxed));
  WriteLong(BEGIN_LEAF_PAGE_OFFSET,
            _beginLeafPageId.load(memory_order_relaxed));
  WriteLong(END_LEAF_PAGE_OFFSET, _endLeafPageId.load(memory_order_relaxed));
  WriteLong(TOTAL_RECORD_COUNT_OFFSET,
            _totalRecordCount.load(memory_order_relaxed));
  WriteLong(CURRENT_RECORD_STAMP_OFFSET,
            _currRecordStamp.load(memory_order_relaxed));

  WriteLong(AUTO_INCREMENT_KEY, _autoIncrementKey1.load(memory_order_relaxed));
  WriteLong(AUTO_INCREMENT_KEY2, _autoIncrementKey2.load(memory_order_relaxed));
  WriteLong(AUTO_INCREMENT_KEY3, _autoIncrementKey3.load(memory_order_relaxed));

  int i = 0;
  for (auto iter = _mapVerStamp.begin(); iter != _mapVerStamp.end(); iter++) {
    WriteLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i++,
              iter->first);
    WriteLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i++,
              iter->second);
  }

  CachePage::WritePage(pageFile);
}

void HeadPage::WriteFileVersion() {
  WriteShort(FILE_VERSION_OFFSET, CURRENT_FILE_VERSION.GetMajorVersion());
  WriteByte(FILE_VERSION_OFFSET + 2, CURRENT_FILE_VERSION.GetMinorVersion());
  WriteByte(FILE_VERSION_OFFSET + 3, CURRENT_FILE_VERSION.GetPatchVersion());
  _bDirty = true;
  _bHeadChanged = true;
}

FileVersion HeadPage::ReadFileVersion() {
  FileVersion fs(ReadShort(FILE_VERSION_OFFSET),
                 ReadByte(FILE_VERSION_OFFSET + 2),
                 ReadByte(FILE_VERSION_OFFSET + 3));
  return fs;
}

void HeadPage::WriteKeyVariableFieldCount(uint16_t num) {
  lock_guard<SpinMutex> lock(_spinMutex);
  _keyAlterableFieldCount = num;
  WriteShort(KEY_ALTERABLE_FIELD_COUNT_OFFSET, num);
  _bDirty = true;
  _bHeadChanged = true;
}

void HeadPage::WriteValueVariableFieldCount(uint16_t num) {
  lock_guard<SpinMutex> lock(_spinMutex);
  _valueAlterableFieldCount = num;
  WriteShort(VALUE_ALTERABLE_FIELD_COUNT_OFFSET, num);
  _bDirty = true;

  _indexTree->_valVarLen = num * UI16_LEN;
  _indexTree->_valOffset = (num + 2) * UI16_LEN;
  _bHeadChanged = true;
}
} // namespace storage
