/*
decoder.cc - c++ source to a base64 reference decoder

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include <decode.h>
#include <iostream>

int main()
{
	base64::decoder D;
	D.decode(std::cin, std::cout);
	return 0;
}

