#include "stdafx.h"
#include <cstdlib>

using namespace std;

string getHome();


int main(int argc, char* argv[])
{
	string str;
	str = getHome();
	cout << "GetHome: " << str << endl;
	getline(cin, str);
	return 0;
}

string getHome()
{
	string home;
	#if defined (_WIN32)
		char *pHomeDrive, *pHomePath;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive != 0)
			home.append(string(pHomeDrive));
		pHomePath = getenv("HOMEPATH");
		if (pHomePath !=0)
			home.append(string(pHomePath));
	#elif defined (_UNIX)
		char *pHome;
		pHomePath = getenv("HOME");
		if (pHome !=0)
			home.append(string(pHome));
	#else
		throw 1;
	#endif
	
	return home;
}