#include "IndexPage.h"
#include "BranchPage.h"

namespace storage {
	 const float IndexPage::LOAD_FACTOR = 0.8f;
	 const uint16_t IndexPage::PAGE_LEVEL_OFFSET = 0;
	 const uint16_t IndexPage::PAGE_REFERENCE_COUNT = 2;
	 const uint16_t IndexPage::NUM_RECORD_OFFSET = 4;
	 const uint16_t IndexPage::TOTAL_DATA_LENGTH_OFFSET = 6;
	 const uint16_t IndexPage::PARENT_PAGE_POINTER_OFFSET = 8;

	 IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId) :
		 CachePage(indexTree, pageId)	{ }

	 IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel, uint64_t parentPageId)
		 : CachePage(indexTree, pageId) {
		 _recordNum = 0;
		 _totalDataLength = 0;
		 _bysPage[PAGE_LEVEL_OFFSET] = (Byte)pageLevel;
		 _dtPageLastUpdate = GetMsFromEpoch();
		 _parentPageId = parentPageId;
	 }

	 IndexPage::~IndexPage() {
	 }
	
	bool IndexPage::PageDivide() {
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

return false;
}
		
}