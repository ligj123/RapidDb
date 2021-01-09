#pragma once
#include "CachePage.h"
#include "RawRecord.h"

namespace storage {
	class BranchPage;
  class IndexPage : public CachePage
  {
	public:
		static float LOAD_FACTOR;
		 static short PAGE_LEVEL_OFFSET;
		 static short NUM_RECORD_OFFSET;
		 static short TOTAL_DATA_LENGTH_OFFSET;
		 static short PARENT_PAGE_POINTER_OFFSET;

	protected:
		vector<RawRecord*> _vctRecord;
		 uint32_t _totalDataLength;
		 uint32_t _recordNum = 0;
		 uint64_t _parentPageId;
		 uint64_t _dtPageLastUpdate;
		 std::atomic<uint32_t> _recordRefCount;
		 bool _bRecordUpdate = false;

	public:
		 IndexPage(IndexTree* indexTree, uint64_t pageId);
		 IndexPage(IndexTree* indexTree, uint64_t pageId, uint32_t pageLevel, uint64_t parentPageId);
		 void LoadPage();



		inline void SetParentPageID(uint64_t parentPageId) { _parentPageId = parentPageId; }
	
		inline uint64_t GetParentPageId() {	return _parentPageId;	}

		inline Byte GetPageLevel() {	return _bysPage[PAGE_LEVEL_OFFSET]; }

		inline uint64_t GetPageLastUpdateTime() {	return _dtPageLastUpdate;	}

		inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsSinceEpoch();	}

		inline uint32_t GetTotalDataLength() { return _totalDataLength; }

		inline uint32_t GetRecordSize() {	return _recordNum; }

		//inline uint16_t GetKeyVariableFieldCount() {
		//	return _indexTree.GetHeadPage().ReadKeyVariableFieldCount();
		//}

		//inline uint16_t getValueVariableFieldCount() {
		//	return _indexTree.GetHeadPage().readValueVariableFieldCount();
		//}

		//public IndexType getIndexType() {
		//	return _indexTree.GetHeadPage().readIndexType();
		//}
//
//		public int releaseRecordRefCount() {
//			return recordRefCount.decrementAndGet();
//		}
//
//		public int increaseRecordRefCount() {
//			return recordRefCount.incrementAndGet();
//		}
//
//		public int addRecordRefCount(int delta) {
//			return recordRefCount.addAndGet(delta);
//		}
//
//		public int getRecordRefCount() {
//			return recordRefCount.get();
//		}
//
//		public boolean pageDivide() throws StorageOverLengthException, StorageIOException{
//			if (recordRefCount.get() > 0) {
//				return false;
//			}
//			if (treeFile.isClosed()/* || totalDataLength > getMaxDataLength() * TreeDividePool.PAGE_DIVIDE_LIMIT */) {
//				writeLock();
//			}
//	 else {
//	if (!writeTryLock())
//		return false;
//}
//
//if (recordRefCount.get() > 0) {
//	writeUnlock();
//	return false;
//}
//
//BranchPage parentPage = getParentPage();
//BranchRecord brParentOld = null;
//
//if (parentPage == null) {
//	parentPage = (BranchPage)treeFile.getMemoryTree().allocateNewPage(null, (byte)(getPageLevel() + 1));
//	parentPage.writeLock();
//	parentPage.setParentPage(null, -1);
//
//	setParentPage(parentPage, 0);
//	treeFile.getMemoryTree().updateRootPage(parentPage);
//}
//else {
//if (!parentPage.writeTryLock()) {
//	writeUnlock();
//	return false;
//}
//
//if (getParentPage() != parentPage) {
//	parentPage.writeUnlock();
//	writeUnlock();
//	return false;
//}
//
//parentPage.addRefCount();
//brParentOld = parentPage.deleteRecord(indexInParent);
//}
//
//// System.out.println("pageDivide");
//try {
//	// Calc this page's records
//	int maxLen = (int)(getMaxDataLength() * LOAD_FACTOR);
//	int pos = 0;
//	int len = 0;
//	for (; pos < lstRecord.size(); pos++) {
//		RawRecord rr = lstRecord.get(pos);
//		len += rr.getTotalLength() + Short.BYTES;
//
//		if (len > maxLen) {
//			if (len > getMaxDataLength()) {
//				len -= rr.getTotalLength() + Short.BYTES;
//			}
//else {
//pos++;
//}
//
//break;
//}
//}
//
//	// Split the surplus records to following page
//	totalDataLength = len;
//	recordNum = pos;
//	ArrayList<IndexPage> lstPage = new ArrayList<>();
//	IndexPage newPage = null;
//
//	len = 0;
//	int startPos = pos;
//	byte level = getPageLevel();
//
//	for (int i = pos; i < lstRecord.size(); i++) {
//		RawRecord rr = lstRecord.get(i);
//
//		len += rr.getTotalLength() + Short.BYTES;
//		if (len > maxLen) {
//			if (len > getMaxDataLength()) {
//				len -= lstRecord.get(i).getTotalLength() + Short.BYTES;
//				i--;
//			}
//
//			newPage = treeFile.getMemoryTree().allocateNewPage(parentPage, level);
//			newPage.setDirty(true);
//			newPage.writeLock();
//			newPage.lstRecord.addAll(lstRecord.subList(startPos, i + 1));
//			if (level > 0) {
//				((BranchPage)newPage).lstPageChildren
//						.addAll(((BranchPage)this).lstPageChildren.subList(startPos, i + 1));
//			}
//
//			newPage.totalDataLength = len;
//
//			for (int k = 0; k < newPage.lstRecord.size(); k++) {
//				newPage.lstRecord.get(k).setParentPage(newPage);
//				if (level > 0) {
//					IndexPage child = ((BranchPage)newPage).lstPageChildren.get(k);
//					if (child == null && level == 1) {
//						child = treeFile.getMemoryTree().getLeafPage(
//								((BranchRecord)newPage.lstRecord.get(k)).getChildPageId());
//					}
//else {
//child.addRefCount();
//}
//
//((BranchPage)newPage).lstPageChildren.set(k, child);
//child.setParentPage((BranchPage)newPage, k);
//TreeDividePool.addCachePage(child);
//child.releaseRefCount();
//}
//}
//
//newPage.recordNum = newPage.lstRecord.size();
//lstPage.add(newPage);
//len = 0;
//startPos = i + 1;
//continue;
//}
//}
//
//if (startPos < lstRecord.size()) {
//	newPage = treeFile.getMemoryTree().allocateNewPage(parentPage, level);
//	newPage.setDirty(true);
//	newPage.writeLock();
//	newPage.lstRecord.addAll(lstRecord.subList(startPos, lstRecord.size()));
//	if (level > 0) {
//		((BranchPage)newPage).lstPageChildren
//				.addAll(((BranchPage)this).lstPageChildren.subList(startPos, lstRecord.size()));
//	}
//	newPage.totalDataLength = len;
//
//	for (int k = 0; k < newPage.lstRecord.size(); k++) {
//		newPage.lstRecord.get(k).setParentPage(newPage);
//		if (level > 0) {
//			IndexPage child = ((BranchPage)newPage).lstPageChildren.get(k);
//			if (child == null && level == 1) {
//				child = treeFile.getMemoryTree().getLeafPage(
//						((BranchRecord)newPage.lstRecord.get(k)).getChildPageId());
//			}
//else {
//child.addRefCount();
//}
//
//((BranchPage)newPage).lstPageChildren.set(k, child);
//child.setParentPage((BranchPage)newPage, k);
//TreeDividePool.addCachePage(child);
//child.releaseRefCount();
//}
//}
//
//newPage.recordNum = newPage.lstRecord.size();
//lstPage.add(newPage);
//}
//
//lstRecord.subList(pos, lstRecord.size()).clear();
//if (level > 0) {
//	((BranchPage)this).lstPageChildren.subList(pos, ((BranchPage)this).lstPageChildren.size()).clear();
//}
//
//if (level == 0) {
//	// if is leaf page, set left and right page
//	long lastPointer = ((LeafPage)this).nextPagePointer;
//
//	((LeafPage)this).nextPagePointer = lstPage.get(0).getPageId();
//	long prevPointer = getPageId();
//
//	for (int i = 0; i < lstPage.size() - 1; i++) {
//		((LeafPage)lstPage.get(i)).prevPagePointer = prevPointer;
//		((LeafPage)lstPage.get(i)).nextPagePointer = lstPage.get(i + 1).getPageId();
//		prevPointer = lstPage.get(i).getPageId();
//	}
//
//	((LeafPage)lstPage.get(lstPage.size() - 1)).prevPagePointer = prevPointer;
//	((LeafPage)lstPage.get(lstPage.size() - 1)).nextPagePointer = lastPointer;
//
//	if (lastPointer == HeadPage.NO_NEXT_PAGE_POINTER) {
//		treeFile.getHeadPage()
//				.writeEndLeafPagePointer(((LeafPage)lstPage.get(lstPage.size() - 1)).getPageId());
//	}
//else {
//LeafPage lastPage = treeFile.getMemoryTree().getLeafPage(lastPointer);
//lastPage.prevPagePointer = ((LeafPage)lstPage.get(lstPage.size() - 1)).getPageId();
//lastPage.setDirty(true);
//TreeDividePool.addCachePage(lastPage);
//lastPage.releaseRefCount();
//}
//}
//
//// Insert this page' key and id to parent page
//RawRecord last = lstRecord.get(lstRecord.size() - 1);
//BranchRecord rec = BranchRecord.init(treeFile, last, getPageId());
//parentPage.insertRecord(indexInParent, rec, this);
//
//// Insert new page' key and id to parent page
//for (int i = 0; i < lstPage.size(); i++) {
//	IndexPage indexPage = lstPage.get(i);
//	indexPage.setParentPage(parentPage, indexInParent + i + 1);
//	last = indexPage.lstRecord.get(indexPage.lstRecord.size() - 1);
//
//	rec = null;
//	RawKey key = last.getKey();
//	if (i == lstPage.size() - 1 && brParentOld != null && brParentOld.compareTo(key) > 0) {
//		rec = BranchRecord.init(treeFile, brParentOld, indexPage.getPageId());
//	}
//else {
//rec = BranchRecord.init(treeFile, last, indexPage.getPageId());
//}
//
//parentPage.insertRecord(indexInParent + i + 1, rec, indexPage);
//key.release();
//
//indexPage.bRecordUpdate = true;
//indexPage.saveRecord();
//indexPage.writeUnlock();
//indexPage.releaseRefCount();
//}
//
//if (brParentOld != null) {
//	brParentOld.release(true, false);
//}
//
//for (int i = indexInParent + lstPage.size() + 1; i < parentPage.lstPageChildren.size(); i++) {
//	IndexPage inp = parentPage.lstPageChildren.get(i);
//	if (inp != null) {
//		inp.setIndexInParent(i);
//	}
//}
//
//////////test
////			if (parentPage.recordNum == 0) {
////				System.out.println("error recordNum");
////			}
////			if (parentPage.lstRecord.size() != parentPage.lstPageChildren.size()) {
////				System.out.println("error size");
////			}
////
////			for (int i = 0; i < parentPage.lstRecord.size(); i++) {
////				IndexPage idp = parentPage.lstPageChildren.get(i);
////				if (idp == null) {
////					continue;
////				}
////				if (((BranchRecord) parentPage.lstRecord.get(i)).getChildPageId() != idp.getPageId()) {
////					System.out.println("error parentPage");
////				}
////
////				if (idp.indexInParent != i) {
////					System.out.println("error indexInParent");
////				}
////			}
////
////			if (level > 0) {
////				for (int i = 0; i < lstRecord.size(); i++) {
////					IndexPage idp = ((BranchPage) this).lstPageChildren.get(i);
////					if (idp == null) {
////						continue;
////					}
////					if (((BranchRecord) lstRecord.get(i)).getChildPageId() != idp.getPageId()) {
////						System.out.println("error this");
////					}
////
////					if (idp.indexInParent != i) {
////						System.out.println("error indexInParent");
////					}
////					
////					if (idp.pageParent.pageId != idp.parentPointer) {
////						System.out.println("error Parent");
////					}
////				}
////				
////				for (IndexPage dp : lstPage) {
////					for (int i = 0; i < dp.lstRecord.size(); i++) {
////						IndexPage idp = ((BranchPage) dp).lstPageChildren.get(i);
////						if (idp == null) {
////							continue;
////						}
////						if (((BranchRecord) dp.lstRecord.get(i)).getChildPageId() != idp.getPageId()) {
////							System.out.println("error this");
////						}
////
////						if (idp.indexInParent != i) {
////							System.out.println("error indexInParent");
////						}
////						if (idp.pageParent.pageId != idp.parentPointer) {
////							System.out.println("error Parent");
////						}
////					}
////				}
////			}
//			//////////////
//
//			bRecordUpdate = true;
//			saveRecord();
//			TreeDividePool.addCachePage(parentPage);
//			StoragePool.writeCachePage(treeFile.getHeadPage());
//
//			return true;
//		}
// finally {
//parentPage.writeUnlock();
//parentPage.releaseRefCount();
//writeUnlock();
//// releaseRefCount();
//}
//		}
//
//			/**
//			 * Load records information from byte array to mapRecord
//			 */
//		public abstract void loadRecords();
//
//		/**
//		 * Save changed records information from mapRecord to byte array
//		 *
//		 * @return saved this page to byte array or not
//		 */
//		public abstract boolean saveRecord();
//
//		/**
//		 * Insert a record to this page
//		 *
//		 * @param record The record information
//		 */
//		 // public abstract void insertRecord(RawRecord record) throws
//		 // StorageKeyRepeatException;
//
//		 /**
//			* Delete a record from this page
//			*
//			* @param record the record to delete
//			* @return delete or not
//			*/
//			// public abstract boolean deleteRecord(RawRecord record);
//
//			/**
//			 * Get the first key in this page
//			 *
//			 * @param type to lock this record for read or write or not
//			 * @return the first key, if the page is empty, return null
//			 */
//		public abstract RawKey getFirstKey();
//
//		/**
//		 * Get the last key in this page
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @return the last key, if the page is empty, return null
//		 */
//		public abstract RawKey getLastKey();
//
//		/**
//		 * get and return the first record if exist, or null
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @return the first record if exist, or null
//		 */
//		public abstract RawRecord getFirstRecord();
//
//		/**
//		 * get and return the last record if exist, or null
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @return the last record if exist, or null
//		 */
//		public abstract RawRecord getLastRecord();
//
//		/**
//		 * Judge if the key saved in this page
//		 *
//		 * @param key the key to judge
//		 * @return true find the key; false not find
//		 */
//		public abstract boolean recordExist(RawKey key);
//
//		/**
//		 * Return all keys in this page
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @return all keys in this page
//		 */
//		public abstract RawKey[] getAllKeys();
//
//		/**
//		 * Return all records in this page
//		 *
//		 * @param isEdit to lock this record for read or write
//		 * @return all records
//		 */
//		public abstract RawRecord[] getAllRecords();
//
//		/**
//		 * get a record from this page
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @param key  the key for the record
//		 * @return the record
//		 */
//		public abstract RawRecord getRecord(RawKey key);
//
//		/**
//		 * get a record from this page
//		 *
//		 * @param type to lock this record for read or write or not
//		 * @param key  the key for the record
//		 * @return the records
//		 */
//		public abstract List<RawRecord> getRecords(RawKey key);
//
//		/**
//		 * To judge if this page' space has been used more than 100%
//		 *
//		 * @return
//		 */
//		public abstract boolean isPageFull();
//
//		/**
//		 * return MAX_DATA_LENGTH
//		 *
//		 * @return MAX_DATA_LENGTH
//		 */
//		public abstract short getMaxDataLength();
//
//		/**
//		 * add a record to this page's end position, only used sorted records in bulk
//		 * load.
//		 *
//		 * @return true: passed to add the record; false: failed to add the record due
//		 *         to over length
//		 */
//		public abstract boolean addRecord(RawRecord record);
//
//		@Override
//			public boolean releaseable() {
//			return refCount.get() == 0 && recordRefCount.get() == 0;
//		}
	};

}