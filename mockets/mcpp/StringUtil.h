// ConvertString.h
// contains function to convert .NET strings to c++ strings
// jk, 10/2004

#ifndef _STRING_UTIL_H
#define _STRING_UTIL_H

#include <string>

using namespace System;
using namespace std;

string ConvertString(System::String ^dotNetString);
char * ConvertCString(System::String ^dotNetString);
void FreeCString(char *what);

#endif // _STRING_UTIL_H