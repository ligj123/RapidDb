#include "CharsetConvert.h"

namespace utils {
  /*ostream& operator<< (ostream& os, const Charsets& set);
  {

  }*/

  ostream& operator<< (ostream& os, const ConvResult& res)
  {
		switch (res)
		{
		case ConvResult::OK:
			os << "OK(" << (int)ConvResult::OK << ")";
			break;
		case ConvResult::PARTIAL:
			os << "PARTIAL(" << (int)ConvResult::PARTIAL << ")";
			break;
		case ConvResult::ERROR:
			os << "ERROR(" << (int)ConvResult::ERROR << ")";
			break;
		case ConvResult::IGNORE:
			os << "IGNORE(" << (int)ConvResult::IGNORE << ")";
			break;
		}

		return os;
  }

  unordered_map<Charsets, string> CodeConvert::mapCharset = {
    {Charsets::GBK, "gbk"},
    {Charsets::UTF8, "utf-8"}
  };

}