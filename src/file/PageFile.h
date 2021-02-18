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
	public:
		PageFile(const string& path, bool overflowFile = false);

		~PageFile() {
			if (_file.is_open()) _file.close();
		}

		uint32_t ReadPage(uint64_t fileOffset, char* bys, uint32_t length);

		void WritePage(uint64_t fileOffset, char* bys, uint32_t length);
		
		void WriteDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset);

		void ReadDataValue(vector<IDataValue*> vctDv, uint32_t dvStart, uint64_t offset, uint32_t totalLen);

		void MoveOverflowData(uint64_t fileOffsetSrc, uint64_t fileOffsetDest, uint32_t length);

		uint64_t Length() {
			unique_lock< utils::SpinMutex> lock;
			_file.seekp(0, ios::end);
			return _file.tellp();
		}

		uint64_t GetOffsetAddLength(uint32_t len) {
			assert(_bOverflowFile);
			return _overFileLength.fetch_add(len);
		}

		void close() { _file.close(); }
	protected:
		/***/
		bool _bOverflowFile;
		string _path;
		fstream _file;
		utils::SpinMutex _spinMutex;
		atomic<uint64_t> _overFileLength;
	};
}

