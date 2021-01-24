#include "PageFile.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
	PageFile::PageFile(const string& path, bool overflowFile) {
		_bOverflowFile = overflowFile;
		_path = path;
		_file.open(path, ios::in | ios::out | ios::binary);
		if (!_file.is_open()) {
			_file.open(path, ios::out);
			if (!_file.is_open()) throw utils::ErrorMsg(FILE_OPEN_FAILED, { path });
			_file.close();
			_file.open(path, ios::in | ios::out | ios::binary);
		}

		if (_bOverflowFile) {
			uint64_t len = Length();
			len += Configure::GetDiskClusterSize() - 1;
			len = len - len % Configure::GetDiskClusterSize();
			_overFileLength.store(len);
		}
	}

	uint32_t PageFile::ReadPage(uint64_t fileOffset, char* bys, uint32_t length) {
		assert(fileOffset % Configure::GetDiskClusterSize() == 0);
		assert(Length() > fileOffset);
		uint32_t len = 0;
		{
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_file.seekp(fileOffset, ios::beg);
			_file.read(bys, length);
			uint32_t len = (uint32_t)_file.gcount();
		}
		LOG_DEBUG << "Read a page, offset=" << fileOffset << "  length=" << length << "  name=" << _path;
		return len;
	}

	void PageFile::WritePage(uint64_t fileOffset, char* bys, uint32_t length) {
		assert(fileOffset % Configure::GetDiskClusterSize() == 0);
		{
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			_file.seekp(fileOffset, ios::beg);
			_file.write(bys, length);
		}

		LOG_DEBUG << "Write a page, offset=" << fileOffset << "  length=" << length << "  name=" << _path;
	}

	void PageFile::WriteDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset) {
		assert(offset % Configure::GetDiskClusterSize() == 0);
		lock_guard<utils::SpinMutex> lock(_spinMutex);
		_file.seekg(offset, ios::beg);
		Byte* buf = CachePool::Apply((uint32_t)Configure::GetCacheBlockSize());

		uint32_t pos = 0;
		for (int i = dvStart; i < vctDv.size(); i++) {
			uint32_t len = vctDv[i]->GetPersistenceLength(false);
			if (pos + len > Configure::GetCacheBlockSize()) {
				_file.write((char*)buf, pos);
				pos = 0;

				if (len > Configure::GetCacheBlockSize()) {
					vctDv[i]->WriteData(_file);
					continue;
				}
			}

			pos += vctDv[i]->WriteData(buf + pos, false);
		}

		_file.write((char*)buf, pos);
		CachePool::Release(buf, (uint32_t)Configure::GetCacheBlockSize());
		LOG_DEBUG << "Write overflow data, offset=" << offset << "  name=" << _path;
	}

	void PageFile::ReadDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset, uint32_t totalLen) {
		assert(Length() >= offset + totalLen);
		lock_guard<utils::SpinMutex> lock(_spinMutex);
		_file.seekg(offset, ios::beg);
		Byte* buf = CachePool::Apply((uint32_t)Configure::GetCacheBlockSize());
		if (totalLen < Configure::GetCacheBlockSize()) {
			uint32_t pos = 0;
			while (pos < totalLen) {
				_file.read((char*)(buf + pos), totalLen - pos);
				pos += (uint32_t)_file.gcount();
			}

			pos = 0;
			for (size_t i = dvStart; i < vctDv.size(); i++) {
				pos += vctDv[i]->ReadData(buf + pos, -1);
			}
		}
		else
		{
			for (size_t i = dvStart; i < vctDv.size(); i++) {
				vctDv[i]->ReadData(_file);
			}
		}

		LOG_DEBUG << "Read overflow data, offset=" << offset << "  name=" << _path;
	}
}