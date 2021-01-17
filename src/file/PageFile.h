#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include "../utils/ErrorMsg.h"
#include "../config/ErrorID.h"
#include "../utils/SpinMutex.h"
#include "../dataType/IDataValue.h"

namespace storage {
	using namespace std;

  class PageFile
  {
	protected:
		bool _bOverflowFile;
		string _path;
		fstream _file;
		utils::SpinMutex _spinMutex;
		atomic<uint64_t> _overFileLength;
	public:
		PageFile(const string& path, bool overflowFile = false) {
			_bOverflowFile = overflowFile;
			_path = path;
			_file.open(path, ios::in | ios::out | ios::binary);
			if (!_file.is_open())
				throw utils::ErrorMsg(FILE_OPEN_FAILED, { path });

			uint64_t len = Length();
			if (len > 0) {
				len += Configure::GetDiskClusterSize() - 1;
				len = len - len % Configure::GetDiskClusterSize();
				_overFileLength.store(len);
			}
		}

		~PageFile() {
			if (_file.is_open()) _file.close();
		}

		uint32_t ReadPage(uint64_t fileOffset, char* bys, uint32_t length);

		void WritePage(uint64_t fileOffset, char* bys, uint32_t length);
		
		void WriteDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset);

		void ReadDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset, uint32_t totalLen);

		uint64_t Length() {
			_file.seekp(0, ios::end);
			return _file.tellp();
		}

		uint64_t GetOffsetAddLength(uint32_t len) {
			return _overFileLength.fetch_add(len);
		}
  };
}

