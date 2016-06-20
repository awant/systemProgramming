// SimpleExampleForDLLUse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include "SimpleSumDLL.h"


int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << sumOfTwo(5, 2) << std::endl;
	return 0;
}