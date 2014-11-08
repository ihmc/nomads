#include "StringUtil.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace std;

// convert .NET string to C++ string
string ConvertString(System::String ^dotNetString)
{
    string retval;
    IntPtr strPtr = Marshal::StringToHGlobalAnsi(dotNetString);
    retval.assign((char *)strPtr.ToPointer());
    Marshal::FreeHGlobal(strPtr);
    return retval;
}

// convert to C string
char * ConvertCString(System::String ^dotNetString)
{
    IntPtr strPtr = Marshal::StringToHGlobalAnsi(dotNetString);
    return (char*)strPtr.ToPointer();
}

// free c string
void FreeCString(char *what)
{
    Marshal::FreeHGlobal((System::IntPtr)what);
}
