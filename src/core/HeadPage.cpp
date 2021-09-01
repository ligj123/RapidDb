#include "HeadPage.h"
#include "IndexTree.h"

namespace storage {
const uint32_t HeadPage::HEAD_PAGE_LENGTH =
    (uint32_t)Configure::GetDiskClusterSize();
const uint16_t HeadPage::MAX_RECORD_VER_COUNT = 8;

const uint16_t HeadPage::VERSION_OFFSET = 0;
const uint16_t HeadPage::INDEX_TYPE_OFFSET = 4;
const uint16_t HeadPage::RECORD_VER_COUNT_OFFSET = 5;
const uint16_t HeadPage::KEY_VARIABLE_FIELD_COUNT = 8;
const uint16_t HeadPage::VALUE_VARIABLE_FIELD_COUNT = 10;
const uint16_t HeadPage::TOTAL_PAGES_COUNT_OFFSET = 16;
const uint16_t HeadPage::ROOT_PAGE_OFFSET = 24;
const uint16_t HeadPage::BEGIN_LEAF_PAGE_OFFSET = 32;
const uint16_t HeadPage::END_LEAF_PAGE_OFFSET = 40;
const uint16_t HeadPage::TOTAL_RECORD_COUNT_OFFSET = 48;
const uint16_t HeadPage::AUTO_PRIMARY_KEY = 56;
const uint16_t HeadPage::AUTO_PRIMARY_KEY2 = 64;
const uint16_t HeadPage::AUTO_PRIMARY_KEY3 = 72;
const uint16_t HeadPage::RECORD_STAMP_OFFSET = 80;
const uint16_t HeadPage::RECORD_VERSION_STAMP_OFFSET = 128;
const uint16_t HeadPage::FREE_PAGE_OFFSET = 256;

const uint64_t HeadPage::NO_PARENT_POINTER = UINT64_MAX;
const uint64_t HeadPage::NO_PREV_PAGE_POINTER = UINT64_MAX;
const uint64_t HeadPage::NO_NEXT_PAGE_POINTER = UINT64_MAX;

void HeadPage::ReadPage() {
  lock_guard<SpinMutex> lock(_spinMutex);
  CachePage::ReadPage();

  _indexType = (IndexType)ReadByte(INDEX_TYPE_OFFSET);
  _recordVerCount = ReadByte(RECORD_VER_COUNT_OFFSET);
  _keyVariableFieldCount = ReadShort(KEY_VARIABLE_FIELD_COUNT);
  _valueVariableFieldCount = ReadShort(VALUE_VARIABLE_FIELD_COUNT);

  _totalPageCount = ReadLong(TOTAL_PAGES_COUNT_OFFSET);
  _rootPageId = ReadLong(ROOT_PAGE_OFFSET);
  _beginLeafPageId = ReadLong(BEGIN_LEAF_PAGE_OFFSET);
  _endLeafPageId = ReadLong(END_LEAF_PAGE_OFFSET);
  _totalRecordCount = ReadLong(TOTAL_RECORD_COUNT_OFFSET);
  _recordStamp = ReadLong(RECORD_STAMP_OFFSET);
  _autoPrimaryKey1 = ReadLong(AUTO_PRIMARY_KEY);
  _autoPrimaryKey2 = ReadLong(AUTO_PRIMARY_KEY2);
  _autoPrimaryKey3 = ReadLong(AUTO_PRIMARY_KEY3);

  _vctRecVer.reserve(_recordVerCount);
  for (Byte i = 0; i < _recordVerCount; i++) {
    _vctRecVer.push_back(
        ReadLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i));
  }
}

void HeadPage::WritePage() {
  lock_guard<SpinMutex> lock(_spinMutex);
  WriteByte(RECORD_VER_COUNT_OFFSET, _recordVerCount);
  WriteLong(TOTAL_PAGES_COUNT_OFFSET, _totalPageCount);
  WriteLong(ROOT_PAGE_OFFSET, _rootPageId);
  WriteLong(BEGIN_LEAF_PAGE_OFFSET, _beginLeafPageId);
  WriteLong(END_LEAF_PAGE_OFFSET, _endLeafPageId);
  WriteLong(TOTAL_RECORD_COUNT_OFFSET, _totalRecordCount);
  WriteLong(RECORD_STAMP_OFFSET, _recordStamp);

  WriteLong(AUTO_PRIMARY_KEY, _autoPrimaryKey1);
  WriteLong(AUTO_PRIMARY_KEY2, _autoPrimaryKey2);
  WriteLong(AUTO_PRIMARY_KEY3, _autoPrimaryKey3);

  for (Byte i = 0; i < _recordVerCount; i++) {
    WriteLong(RECORD_VERSION_STAMP_OFFSET + sizeof(uint64_t) * i,
              _vctRecVer[i]);
  }

  CachePage::WritePage();
}

void HeadPage::WriteFileVersion() {
  WriteShort(VERSION_OFFSET, CURRENT_FILE_VERSION.GetMajorVersion());
  WriteByte(VERSION_OFFSET + 2, CURRENT_FILE_VERSION.GetMinorVersion());
  WriteByte(VERSION_OFFSET + 3, CURRENT_FILE_VERSION.GetPatchVersion());
  _bDirty = true;
}

FileVersion HeadPage::ReadFileVersion() {
  FileVersion fs(ReadShort(VERSION_OFFSET), ReadByte(VERSION_OFFSET + 2),
                 ReadByte(VERSION_OFFSET + 3));
  return fs;
}

void HeadPage::WriteKeyVariableFieldCount(uint16_t num) {
  lock_guard<SpinMutex> lock(_spinMutex);
  _keyVariableFieldCount = num;
  WriteShort(KEY_VARIABLE_FIELD_COUNT, num);
  _bDirty = true;

  _indexTree->_keyVarLen = num * sizeof(uint16_t);
  _indexTree->_keyOffset = (num + 2) * sizeof(uint16_t);
}

void HeadPage::WriteValueVariableFieldCount(uint16_t num) {
  lock_guard<SpinMutex> lock(_spinMutex);
  _valueVariableFieldCount = num;
  WriteShort(VALUE_VARIABLE_FIELD_COUNT, num);
  _bDirty = true;

  _indexTree->_valVarLen = num * sizeof(uint16_t);
  _indexTree->_valOffset = (num + 2) * sizeof(uint16_t);
}
} // namespace storage
