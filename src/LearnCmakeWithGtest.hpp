#pragma once
#include<cstdint>


class LearnCmakeWithGtest
{
public :
	bool is_Same_number( int32_t a, int32_t b )
	{
		if ( a == b )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	void SetNum1( int32_t data )
	{
		num1 = data;
	}
	void SetNum2( int32_t data )
	{
		num2 = data;
	}

	int32_t GetNum1()
	{
		return num1;
	}
	int32_t GetNum2()
	{
		return num2;
	}

private:
	int32_t num1;
	int32_t num2;


};