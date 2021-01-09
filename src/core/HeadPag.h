#pragma once
#include "CachePage.h"
#include "IndexType.h"
#include "../config/FileVersion.h"

namespace storage {
	class HeadPage : public CachePage {
	public:
		static const uint32_t HEAD_PAGE_LENGTH;
		static const uint16_t VERSION_OFFSET;
		static const uint16_t INDEX_TYPE_OFFSET;
		static const uint16_t TOTAL_PAGES_NUM_OFFSET;
		static const uint16_t ROOT_PAGE_OFFSET;
		static const uint16_t BEGIN_LEAF_PAGE_OFFSET;
		static const uint16_t END_LEAF_PAGE_OFFSET;
		static const uint16_t TOTAL_RECORD_OFFSET;
		static const uint16_t AUTO_PRIMARY_KEY;
		static const uint16_t KEY_VARIABLE_FIELD_COUNT;
		static const uint16_t VALUE_VARIABLE_FIELD_COUNT;
		static const uint16_t AUTO_PRIMARY_KEY2;
		static const uint16_t AUTO_PRIMARY_KEY3;
		static const uint16_t FREE_PAGE_OFFSET;

		static const uint64_t NO_PARENT_POINTER;
		static const uint64_t NO_PREV_PAGE_POINTER;
		static const uint64_t NO_NEXT_PAGE_POINTER;

	protected:
		uint16_t _keyVariableFieldCount;
		uint16_t _valueVariableFieldCount;
		IndexType _indexType;
		uint64_t _totalPageNum;
		uint64_t _rootPageId;
		uint64_t _beginLeafPageId;
		uint64_t _endLeafPageId;
		uint64_t _totalRecordNum;
		uint64_t _autoPrimaryKey1;
		uint64_t _autoPrimaryKey2;
		uint64_t _autoPrimaryKey3;
		utils::SpinMutex _spinMutex;

	public:
		HeadPage(IndexTree* indexTree) : CachePage(indexTree, UINT64_MAX) {	}
			
		void ReadPage() override;
		void WritePage() override;
		void WriteFileVersion();
		FileVersion& ReadFileVersion();

		inline uint64_t GetAndIncTotalPageNum() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_bDirty = true;
			return _totalPageNum++;
		}

		inline uint64_t ReadTotalPageNum() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _totalPageNum;
		}

		void WriteTotalPageNum(uint64_t totalPage) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_totalPageNum = totalPage;
			_bDirty = true;
		}

		inline uint64_t ReadTotalRecordNum() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _totalRecordNum;
		}

		inline void WriteTotalRecordNum(uint64_t totalRecNum) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_totalRecordNum = totalRecNum;
			_bDirty = true;
		}

		inline long GetAndAddTotalRecordNum(uint64_t delta) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_bDirty = true;
			uint64_t tmp = _totalRecordNum;
			_totalRecordNum += delta;
			return tmp;
		}
		
		inline void WriteRootPagePointer(uint64_t pointer) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_rootPageId = pointer;
			_bDirty = true;
		}

		inline uint64_t ReadRootPagePointer() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _rootPageId;
		}

		inline void WriteBeginLeafPagePointer(uint64_t pointer) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_beginLeafPageId = pointer;
			_bDirty = true;
		}

		inline uint64_t ReadBeginLeafPagePointer() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _beginLeafPageId;
		}

		inline void WriteEndLeafPagePointer(uint64_t pointer) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_endLeafPageId = pointer;
			_bDirty = true;
		}

		inline uint64_t ReadEndLeafPagePointer() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _endLeafPageId;
		}

		inline void WriteIndexType(IndexType type) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_indexType = type;
			WriteByte(INDEX_TYPE_OFFSET, (Byte)type);
			_bDirty = true;
		}

		inline IndexType ReadIndexType() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _indexType;
		}
		
		inline uint64_t GetAndAddAutoPrimaryKey(uint64_t step) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t num = _autoPrimaryKey1;
			_autoPrimaryKey1 += step;
			_bDirty = true;
			return num;
		}

		inline uint64_t ReadAutoPrimaryKey() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _autoPrimaryKey1;
		}

		void WriteAutoPrimaryKey(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey1 = key;
			_bDirty = true;
		}

		uint64_t GetAndAddAutoPrimaryKey2(uint64_t step) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t num = _autoPrimaryKey2;
			_autoPrimaryKey2 += step;
			_bDirty = true;
			return num;
		}

		uint64_t ReadAutoPrimaryKey2() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _autoPrimaryKey2;
		}

		void WriteAutoPrimaryKey2(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey2 = key;
			_bDirty = true;
		}

		uint64_t GetAndAddAutoPrimaryKey3(uint64_t step) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t num = _autoPrimaryKey3;
			_autoPrimaryKey3 += step;
			_bDirty = true;
			return num;
		}

		uint64_t ReadAutoPrimaryKey3() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _autoPrimaryKey3;
		}

		void WriteAutoPrimaryKey3(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey3 = key;
			_bDirty = true;
		}

		uint16_t ReadKeyVariableFieldCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _keyVariableFieldCount;
		}

		void WriteKeyVariableFieldCount(uint16_t num) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_keyVariableFieldCount = num;
			WriteShort(KEY_VARIABLE_FIELD_COUNT, num);
			_bDirty = true;
		}

		uint16_t ReadValueVariableFieldCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex); 
			return _valueVariableFieldCount;
		}

		void WriteValueVariableFieldCount(uint16_t num) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_valueVariableFieldCount = num;
			WriteShort(VALUE_VARIABLE_FIELD_COUNT, num);
			_bDirty = true;
		}
	};
}