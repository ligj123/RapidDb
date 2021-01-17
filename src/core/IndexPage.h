#pragma once
#include "CachePage.h"
#include "RawRecord.h"
#include "IndexType.h"

namespace storage {
	class BranchPage;
	class IndexPage : public CachePage
	{
	public:
		static const float LOAD_FACTOR;
		static const uint16_t PAGE_LEVEL_OFFSET;
		static const uint16_t PAGE_REFERENCE_COUNT;
		static const uint16_t NUM_RECORD_OFFSET;
		static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
		static const uint16_t PARENT_PAGE_POINTER_OFFSET;

	protected:
		vector<RawRecord*> _vctRecord;
		uint32_t _totalDataLength;
		uint32_t _recordNum = 0;
		uint64_t _parentPageId;
		uint64_t _dtPageLastUpdate;

	public:
		IndexPage(IndexTree* indexTree, uint64_t pageId);
		IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel, uint64_t parentPageId);
		~IndexPage();
		void LoadPage();
		bool PageDivide();

		void ClearRecords();
		inline void SetParentPageID(uint64_t parentPageId) { _parentPageId = parentPageId; }
		inline uint64_t GetParentPageId() { return _parentPageId; }
		inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
		inline uint64_t GetPageLastUpdateTime() { return _dtPageLastUpdate; }
		inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsFromEpoch(); }
		inline uint32_t GetTotalDataLength() { return _totalDataLength; }
		inline uint32_t GetRecordSize() { return _recordNum; }

		/**Load records information from byte array to mapRecord */
		virtual void LoadRecords() = 0;

		/**
		 * Save changed records information from mapRecord to byte array
		 * @return saved this page to byte array or not
		 */
		virtual bool SaveRecord() = 0;


		/**
		 * Get the first key in this page
		 * @return the first key, if the page is empty, return null
		 */
		virtual RawKey* GetFirstKey() const = 0;

		/**
		 * Get the last key in this page
		 * @return the last key, if the page is empty, return null
		 */
		virtual RawKey* GetLastKey() const = 0;

		/**
		 * get and return the first record if exist, or null
		 * @return the first record if exist, or null
		 */
		virtual RawRecord* GetFirstRecord() const = 0;

		/**
		 * get and return the last record if exist, or null
		 * @return the last record if exist, or null
		 */
		virtual RawRecord* GetLastRecord()const = 0;

		/**
		 * Judge if the key saved in this page
		 * @return true find the key; false not find
		 */
		virtual bool RecordExist(const RawKey& key) const = 0;

		/**
		 * Return all keys in this page
		 * @return all keys in this page
		 */
		virtual vector<RawKey*>& GetAllKeys() const = 0;

		/**
		 * Return all records in this page
		 * @return all records
		 */
		virtual vector<RawRecord*>& GetAllRecords() const = 0;

		/**
		 * get a record from this page
		 * @param key  the key for the record
		 * @return the record, if not find, return null
		 */
		virtual RawRecord* GetRecord(const RawKey& key) const = 0;

		/**
		 * get a record from this page
		 * @param key  the key for the records
		 * @return the records
		 */
		virtual vector<RawRecord*> GetRecords(const RawKey& key) = 0;

		/**
		 * To judge if this page' space has been used more than 100%
		 * @return
		 */
		virtual bool IsPageFull() = 0;

		/**
		 * @return MAX_DATA_LENGTH
		 */
		virtual uint16_t GetMaxDataLength();

		/**
		 * add a record to this page's end position, only used sorted records in bulk load.
		 * @return true: passed to add the record; false: failed to add the record due
		 *         to over length
		 */
		virtual bool AddRecord(RawRecord* record) = 0;
		/**
	 * Insert a record to this page
	 * @param record The record information
	 *  @return true: passed to insert the record; false: failed to insert.
	 */
		virtual bool InsertRecord(RawRecord* record) = 0;

		bool Releaseable() override { return _refCount.load() == 0; }

		virtual int CompareTo(const IndexPage& other) = 0;
	};
}