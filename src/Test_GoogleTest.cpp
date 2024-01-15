#include <string>
#include <gtest/gtest.h>
#include "Test_GoogleTest.hpp"

// GTEST 확인.
TEST_F( Test_MyGtest, MY_GTEST_01 )
{
	testClass->SetNum1( 100 );
	testClass->SetNum2( 200 );
	
	EXPECT_FALSE( testClass->is_Same_number( testClass->GetNum1(), testClass->GetNum2() ) );

	testClass->SetNum2( 100 );

	EXPECT_TRUE( testClass->is_Same_number( testClass->GetNum1(), testClass->GetNum2() ) );
}

// GTEST 확인.
TEST_F( Test_MyGtest, MY_GTEST_02 )
{
	std::string str1 = "GoogleTest";
	std::string str2 = "GoogleTest";

	EXPECT_EQ( str1, str2 );
}
