/*
encoder.cc - c++ source to a base64 reference encoder

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include <encode.h>
#include <iostream>

int main()
{
	base64::encoder E;
	E.encode(std::cin, std::cout);
	return 0;
}
