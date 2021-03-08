#pragma once
#include "CachePage.h"
#include "IndexType.h"
#include "../config/FileVersion.h"
#include <vector>

namespace storage {
	class HeadPage : public CachePage {
	public:
		static const uint32_t HEAD_PAGE_LENGTH;
		/**The max versions can be support at the same time in a table*/
		static const uint16_t MAX_RECORD_VER_COUNT;
		static const uint16_t VERSION_OFFSET;
		static const uint16_t INDEX_TYPE_OFFSET;
		/**How much versions have been saved to this table*/
		static const uint16_t RECORD_VER_COUNT_OFFSET;
		static const uint16_t KEY_VARIABLE_FIELD_COUNT;
		static const uint16_t VALUE_VARIABLE_FIELD_COUNT;

		static const uint16_t TOTAL_PAGES_COUNT_OFFSET;
		static const uint16_t ROOT_PAGE_OFFSET;
		static const uint16_t BEGIN_LEAF_PAGE_OFFSET;
		static const uint16_t END_LEAF_PAGE_OFFSET;
		static const uint16_t TOTAL_RECORD_COUNT_OFFSET;
		static const uint16_t AUTO_PRIMARY_KEY;
		static const uint16_t AUTO_PRIMARY_KEY2;
		static const uint16_t AUTO_PRIMARY_KEY3;
		static const uint16_t RECORD_STAMP_OFFSET;
		static const uint16_t RECORD_VERSION_STAMP_OFFSET;
		static const uint16_t FREE_PAGE_OFFSET;

		static const uint64_t NO_PARENT_POINTER;
		static const uint64_t NO_PREV_PAGE_POINTER;
		static const uint64_t NO_NEXT_PAGE_POINTER;

	protected:
		uint16_t _keyVariableFieldCount = 0;
		uint16_t _valueVariableFieldCount = 0;
		/**The versions that this table support in current time*/
		uint8_t _recordVerCount = 0;
		IndexType _indexType = IndexType::PRIMARY;
		uint64_t _totalPageCount = 0;
		uint64_t _rootPageId = 0;
		uint64_t _beginLeafPageId = 0;
		uint64_t _endLeafPageId = 0;
		uint64_t _totalRecordCount = 0;
		/**In this table, any changes for a record will need a new record version stamp,
		it start from 0, and will add one every time. This stamp will be used for log transport,
		data snapshot etc.*/
		uint64_t _recordStamp = 0;
		uint64_t _autoPrimaryKey1 = 0;
		uint64_t _autoPrimaryKey2 = 0;
		uint64_t _autoPrimaryKey3 = 0;
		/***/
		std::vector<uint64_t> _vctRecVer;
		utils::SpinMutex _spinMutex;

	public:
		HeadPage(IndexTree* indexTree) : CachePage(indexTree, UINT64_MAX) {	}
			
		void ReadPage() override;
		void WritePage() override;
		void WriteFileVersion();
		FileVersion ReadFileVersion();

		inline Byte ReadRecordVersionCount() { return _recordVerCount; }

		inline Byte AddNewRecordVersion(uint64_t ver) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t min = 1;
			if (_recordVerCount > 0) min = _vctRecVer[_recordVerCount - 1] + 1;
			assert(ver < min || ver > _recordStamp);

			_vctRecVer.push_back(ver);
			_recordVerCount++;
			_bDirty = true;
			return _recordVerCount;
		}

		inline uint64_t GetRecordVersionStamp(Byte ver) {
			assert(ver < _recordVerCount);
			return _vctRecVer[ver];
		}

		inline void RemoveRecordVersionStamp(Byte ver) {
			assert(ver < _recordVerCount);
			_vctRecVer.erase(_vctRecVer.begin() + ver);
		}

		inline uint64_t GetLastVersionStamp() {
			if (_recordVerCount == 0) return 0;
			return _vctRecVer[_recordVerCount - 1];
		}

		inline uint64_t GetAndIncRecordStamp() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_bDirty = true;
			return _recordStamp++;
		}

		inline uint64_t ReadRecordStamp() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _recordStamp;
		}

		void WriteRecordStamp(uint64_t recordStamp) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_recordStamp = recordStamp;
			_bDirty = true;
		}

		inline uint64_t GetAndIncTotalPageCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_bDirty = true;
			return _totalPageCount++;
		}

		inline uint64_t ReadTotalPageCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _totalPageCount;
		}

		void WriteTotalPageCount(uint64_t totalPage) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_totalPageCount = totalPage;
			_bDirty = true;
		}

		inline uint64_t ReadTotalRecordCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _totalRecordCount;
		}

		inline void WriteTotalRecordCount(uint64_t totalRecNum) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_totalRecordCount = totalRecNum;
			_bDirty = true;
		}

		inline uint64_t GetAndIncTotalRecordCount() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_bDirty = true;
			return _totalRecordCount++;
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

		inline void WriteAutoPrimaryKey(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey1 = key;
			_bDirty = true;
		}

		inline uint64_t GetAndAddAutoPrimaryKey2(uint64_t step) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t num = _autoPrimaryKey2;
			_autoPrimaryKey2 += step;
			_bDirty = true;
			return num;
		}

		inline uint64_t ReadAutoPrimaryKey2() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _autoPrimaryKey2;
		}

		inline void WriteAutoPrimaryKey2(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey2 = key;
			_bDirty = true;
		}

		inline uint64_t GetAndAddAutoPrimaryKey3(uint64_t step) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			uint64_t num = _autoPrimaryKey3;
			_autoPrimaryKey3 += step;
			_bDirty = true;
			return num;
		}

		inline uint64_t ReadAutoPrimaryKey3() {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _autoPrimaryKey3;
		}

		void WriteAutoPrimaryKey3(uint64_t key) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_autoPrimaryKey3 = key;
			_bDirty = true;
		}

		inline uint16_t ReadKeyVariableFieldCount() {
			//lock_guard<utils::SpinMutex> lock(_spinMutex);
			return _keyVariableFieldCount;
		}

		void WriteKeyVariableFieldCount(uint16_t num) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_keyVariableFieldCount = num;
			WriteShort(KEY_VARIABLE_FIELD_COUNT, num);
			_bDirty = true;
		}

		inline uint16_t ReadValueVariableFieldCount() {
			//lock_guard<utils::SpinMutex> lock(_spinMutex); 
			return _valueVariableFieldCount;
		}

		inline void WriteValueVariableFieldCount(uint16_t num) {
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_valueVariableFieldCount = num;
			WriteShort(VALUE_VARIABLE_FIELD_COUNT, num);
			_bDirty = true;
		}
	};
}