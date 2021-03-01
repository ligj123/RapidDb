#include "IndexPage.h"
#include "BranchPage.h"
#include "IndexTree.h"
#include "../pool/PageDividePool.h"
#include "../pool/StoragePool.h"
#include "BranchRecord.h"

namespace storage {
	const float IndexPage::LOAD_FACTOR = 0.8f;
	const uint16_t IndexPage::PAGE_LEVEL_OFFSET = 0;
	const uint16_t IndexPage::PAGE_REFERENCE_COUNT = 2;
	const uint16_t IndexPage::NUM_RECORD_OFFSET = 4;
	const uint16_t IndexPage::TOTAL_DATA_LENGTH_OFFSET = 6;
	const uint16_t IndexPage::PARENT_PAGE_POINTER_OFFSET = 8;

	IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId) :
		CachePage(indexTree, pageId) {
		_dtPageLastUpdate = GetMsFromEpoch();
	}

	IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel, uint64_t parentPageId)
		: CachePage(indexTree, pageId) {
		_bysPage[PAGE_LEVEL_OFFSET] = (Byte)pageLevel;
		_dtPageLastUpdate = GetMsFromEpoch();
		_parentPageId = parentPageId;
	}

	IndexPage::~IndexPage() {
	}

	void IndexPage::Init() {
		_vctRecord.clear();
		_recordRefCount = 0;
		_recordNum = ReadShort(NUM_RECORD_OFFSET);
		_totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
		_parentPageId = ReadLong(PARENT_PAGE_POINTER_OFFSET);
		_dtPageLastUpdate = GetMsFromEpoch();
	}

	bool IndexPage::PageDivide() {
		if (_recordRefCount > 0) {
			return false;
		}
		if (_indexTree->IsClosed() || _totalDataLength > GetMaxDataLength() * PageDividePool::PAGE_DIVIDE_LIMIT) {
			WriteLock();
		}
		else {
			if (!WriteTryLock())
				return false;
		}

		if (_recordRefCount > 0) {
			WriteUnlock();
			return false;
		}
				
		BranchRecord* brParentOld = nullptr;
		BranchPage* parentPage = nullptr;
		int posInParent = 0;
		if (_parentPageId == HeadPage::NO_PARENT_POINTER) {
			parentPage = (BranchPage*)_indexTree->AllocateNewPage(HeadPage::NO_PARENT_POINTER, GetPageLevel() + 1);
			parentPage->WriteLock();
			_parentPageId = parentPage->GetPageId();
			_indexTree->UpdateRootPage(parentPage);
		}
		else {
			parentPage = (BranchPage*)_indexTree->GetPage(_parentPageId, false);
			if (!parentPage->WriteTryLock()) {
				WriteUnlock();
				return false;
			}

			BranchRecord br(_indexTree, _vctRecord[_recordNum - 1], GetPageId());
			bool bFind;
			int posInParent = parentPage->SearchRecord(br, bFind);
			if (!bFind) posInParent = parentPage->GetRecordNum();
			brParentOld = parentPage->DeleteRecord(posInParent);
		}
		
		// System.out.println("pageDivide");
		try {
			// Calc this page's records
			int maxLen = (int)(GetMaxDataLength() * LOAD_FACTOR);
			int pos = 0;
			int len = 0;
			for (; pos < _vctRecord.size(); pos++) {
				RawRecord* rr = _vctRecord[pos];
				len += rr->GetTotalLength() + sizeof(uint16_t);

				if (len > maxLen) {
					if (len > GetMaxDataLength()) {
						len -= rr->GetTotalLength() + sizeof(uint16_t);
					}
					else {
						pos++;
					}

					break;
				}
			}

			// Split the surplus records to following page
			_totalDataLength = len;
			_recordNum = pos;
			vector<IndexPage*> vctPage;
			IndexPage* newPage = nullptr;

			len = 0;
			int startPos = pos;
			Byte level = GetPageLevel();

			for (int i = pos; i < _vctRecord.size(); i++) {
				RawRecord* rr = _vctRecord[i];

				len += rr->GetTotalLength() + sizeof(uint16_t);
				if (len > maxLen) {
					if (len > GetMaxDataLength()) {
						len -= rr->GetTotalLength() + sizeof(uint16_t);
						i--;
					}

					newPage = _indexTree->AllocateNewPage(parentPage->GetPageId(), level);
					newPage->SetDirty(true);
					newPage->WriteLock();
					newPage->_vctRecord.insert(newPage->_vctRecord.end(),
						_vctRecord.begin() + startPos, _vctRecord.begin() + i + 1);

					newPage->_totalDataLength = len;
					newPage->_recordNum = (int32_t)newPage->_vctRecord.size();
					vctPage.push_back(newPage);
					len = 0;
					startPos = i + 1;
					continue;
				}
			}

			if (startPos < _vctRecord.size()) {
				newPage = _indexTree->AllocateNewPage(parentPage->GetPageId(), level);
				newPage->SetDirty(true);
				newPage->WriteLock();
				newPage->_vctRecord.insert(newPage->_vctRecord.end(),
					_vctRecord.begin() + startPos, _vctRecord.end());

				newPage->_totalDataLength = len;
				newPage->_recordNum = (int32_t)newPage->_vctRecord.size();
				vctPage.push_back(newPage);
			}

			_vctRecord.erase(_vctRecord.begin() + pos, _vctRecord.end());

			if (level == 0) {
				// if is leaf page, set left and right page
				uint64_t lastPointer = ((LeafPage*)this)->GetNextPageId();

				((LeafPage*)this)->SetNextPageId(vctPage[0]->GetPageId());
				uint64_t prevPointer = GetPageId();

				for (int i = 0; i < vctPage.size() - 1; i++) {
					((LeafPage*)vctPage[i])->SetPrevPageId(prevPointer);
					((LeafPage*)vctPage[i])->SetNextPageId(vctPage[i + 1]->GetPageId());
					prevPointer = vctPage[i]->GetPageId();
				}

				((LeafPage*)vctPage[vctPage.size() - 1])->SetPrevPageId(prevPointer);
				((LeafPage*)vctPage[vctPage.size() - 1])->SetNextPageId(lastPointer);

				if (lastPointer == HeadPage::NO_NEXT_PAGE_POINTER) {
					_indexTree->GetHeadPage()->WriteEndLeafPagePointer(((LeafPage*)vctPage[vctPage.size() - 1])->GetPageId());
				}
				else {
					LeafPage* lastPage = (LeafPage*)_indexTree->GetPage(lastPointer, true);
					lastPage->SetPrevPageId(((LeafPage*)vctPage[vctPage.size() - 1])->GetPageId());
					lastPage->SetDirty(true);
					PageDividePool::AddCachePage(lastPage);
					lastPage->DecRefCount();
				}
			}

			// Insert this page' key and id to parent page
			RawRecord* last = _vctRecord[_vctRecord.size() - 1];
			BranchRecord* rec = new BranchRecord(_indexTree, last, GetPageId());
			parentPage->InsertRecord(rec, posInParent);
			posInParent++;

			// Insert new page' key and id to parent page
			for (int i = 0; i < vctPage.size(); i++) {
				IndexPage* indexPage = vctPage[i];
				last = indexPage->_vctRecord[indexPage->_recordNum - 1];

				rec = nullptr;
				if (i == vctPage.size() - 1 && brParentOld != nullptr && brParentOld->CompareKey(*last) > 0) {
					rec = new BranchRecord(_indexTree, brParentOld, indexPage->GetPageId());
				}
				else {
					rec = new BranchRecord(_indexTree, last, indexPage->GetPageId());
				}

				parentPage->InsertRecord(rec, posInParent + i);

				indexPage->_bRecordUpdate = true;
				indexPage->SaveRecord();
				indexPage->WriteUnlock();
			}

			if (brParentOld != nullptr) {
				brParentOld->ReleaseRecord();
			}
			
			_bRecordUpdate = true;
			SaveRecord();
			PageDividePool::AddCachePage(parentPage);
			StoragePool::WriteCachePage(_indexTree->GetHeadPage());

			parentPage->SaveRecord();
			parentPage->WriteUnlock();
			WriteUnlock();
			return true;
		}
		catch (...) {
			parentPage->WriteUnlock();
			parentPage->DecRefCount();
			WriteUnlock();
			return false;
		}
	}
}