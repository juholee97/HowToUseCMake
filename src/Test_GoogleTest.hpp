#pragma once
#include <memory>
#include "LearnCmakeWithGtest.hpp"

class Test_MyGtest : public testing::Test
{
protected:
	void SetUp() override
	{
		testClass = std::make_unique< LearnCmakeWithGtest >();
	}

	void TearDown() override
	{
	}
	std::unique_ptr< LearnCmakeWithGtest > testClass;
};