#include "stdafx.h"

using namespace std;

bool MakeDir(string& dir)
{
	#if defined (_WIN32)
		wstring wDir;
		wDir.assign(dir.begin(), dir.end());

		if (CreateDirectory(wDir.c_str(), 0))
			return true;
		DWORD error = GetLastError();
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			return true;
		else 
			return false;
	#elif defined (__linux__)
		//use mkdir from <unistd.h>
		throw 2;
	#else
		throw 1;
	#endif
}

bool MakePath(string& path)
{
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw 1;
	#endif
	bool success = true;
	size_t pos = 0;

	// path must end with delim character
	if (path.back() != delim)
		path += delim;

	pos = path.find_first_of(delim);
	while (success == true && pos != string::npos)
	{
		if (pos != 0)
			success = MakeDir(path.substr(0, pos));
		++pos;
		pos = path.find_first_of(delim, pos);
	}
	return success;
}