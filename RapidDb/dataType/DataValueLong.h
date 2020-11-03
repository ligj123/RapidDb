#pragma once
#include "TDataValue.h"

class DataValueLong : public TDataValue<int64_t> 
{
public:
	DataValueLong();
	DataValueLong(int64_t val);
	DataValueLong(char* byArray, int spos);
public :
	operator int64_t();
	virtual void SetMinValue();
	virtual void SetMaxValue();
	virtual void SetDefaultValue();
protected:

private:
	

};

