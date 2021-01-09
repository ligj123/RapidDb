#include "HeadPag.h"

namespace storage {
	const uint32_t HeadPage::HEAD_PAGE_LENGTH = Configure::GetDiskClusterSize();
	const uint16_t HeadPage::VERSION_OFFSET = 0;
	const uint16_t HeadPage::INDEX_TYPE_OFFSET = 4;
	const uint16_t HeadPage::TOTAL_PAGES_NUM_OFFSET = 8;
	const uint16_t HeadPage::ROOT_PAGE_OFFSET = 16;
	const uint16_t HeadPage::BEGIN_LEAF_PAGE_OFFSET = 24;
	const uint16_t HeadPage::END_LEAF_PAGE_OFFSET = 32;
	const uint16_t HeadPage::TOTAL_RECORD_OFFSET = 40;
	const uint16_t HeadPage::AUTO_PRIMARY_KEY = 48;
	const uint16_t HeadPage::KEY_VARIABLE_FIELD_COUNT = 56;
	const uint16_t HeadPage::VALUE_VARIABLE_FIELD_COUNT = 58;
	const uint16_t HeadPage::AUTO_PRIMARY_KEY2 = 64;
	const uint16_t HeadPage::AUTO_PRIMARY_KEY3 = 72;
	const uint16_t HeadPage::FREE_PAGE_OFFSET = 128;

	const uint64_t HeadPage::NO_PARENT_POINTER = UINT64_MAX; 
	const uint64_t HeadPage::NO_PREV_PAGE_POINTER = UINT64_MAX;
	const uint64_t HeadPage::NO_NEXT_PAGE_POINTER = UINT64_MAX;

	void HeadPage::ReadPage() {
		CachePage::ReadPage();

		lock_guard<utils::SpinMutex> lock(_spinMutex);
		_indexType = (IndexType)ReadByte(INDEX_TYPE_OFFSET);
		_keyVariableFieldCount = ReadShort(KEY_VARIABLE_FIELD_COUNT);
		_valueVariableFieldCount = ReadShort(VALUE_VARIABLE_FIELD_COUNT);

		_totalPageNum = ReadLong(TOTAL_PAGES_NUM_OFFSET);
		_rootPageId = ReadLong(ROOT_PAGE_OFFSET);
		_beginLeafPageId = ReadLong(BEGIN_LEAF_PAGE_OFFSET);
		_endLeafPageId = ReadLong(END_LEAF_PAGE_OFFSET);
		_totalRecordNum = ReadLong(TOTAL_RECORD_OFFSET);

		_autoPrimaryKey1 = ReadLong(AUTO_PRIMARY_KEY);
		_autoPrimaryKey2 = ReadLong(AUTO_PRIMARY_KEY2);
		_autoPrimaryKey3 = ReadLong(AUTO_PRIMARY_KEY3);
	}

	void HeadPage::WritePage() {
		lock_guard<utils::SpinMutex> lock(_spinMutex);
		WriteLong(TOTAL_PAGES_NUM_OFFSET, _totalPageNum);
		WriteLong(ROOT_PAGE_OFFSET, _rootPageId);
		WriteLong(BEGIN_LEAF_PAGE_OFFSET, _beginLeafPageId);
		WriteLong(END_LEAF_PAGE_OFFSET, _endLeafPageId);
		WriteLong(TOTAL_RECORD_OFFSET, _totalRecordNum);

		WriteLong(AUTO_PRIMARY_KEY, _autoPrimaryKey1);
		WriteLong(AUTO_PRIMARY_KEY2, _autoPrimaryKey2);
		WriteLong(AUTO_PRIMARY_KEY3, _autoPrimaryKey3);
		
		CachePage::WritePage();
	}

	void HeadPage::WriteFileVersion() {
		WriteShort(VERSION_OFFSET, INDEX_FILE_VERSION.GetMajorVersion());
		WriteByte(VERSION_OFFSET + 2, INDEX_FILE_VERSION.GetMinorVersion());
		WriteByte(VERSION_OFFSET + 3, INDEX_FILE_VERSION.GetPatchVersion());
		_bDirty = true;
	}

	FileVersion& HeadPage::ReadFileVersion() {
		FileVersion fs(ReadShort(VERSION_OFFSET),
			ReadByte(VERSION_OFFSET + 2),
			ReadByte(VERSION_OFFSET + 3));
		return fs;
	}
}