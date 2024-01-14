#include <gtest/gtest.h>
#include "Test_GoogleTest.hpp"

// GTEST È®ÀÎ.
TEST_F( Test_MyGtest, MY_GTEST_01 )
{
	testClass->SetNum1( 100 );
	testClass->SetNum2( 200 );
	
	EXPECT_FALSE( testClass->is_Same_number( testClass->GetNum1(), testClass->GetNum2() ) );

	testClass->SetNum2( 100 );

	EXPECT_TRUE( testClass->is_Same_number( testClass->GetNum1(), testClass->GetNum2() ) );
}
