// SimpleSumDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>
#include "SimpleSumDLL.h"

int sumOfTwo(int a, int b) 
{
	return a + b;
}

void writeWelcomeToFile(int i)
{
	std::cout << "Welcome! "  << i << std::endl;
	return;
}