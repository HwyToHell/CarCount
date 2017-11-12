#include "stdafx.h"
#include "../include/config.h"

using namespace std;





int main(int argc, char *argv[])
{
	Config cfgstr("test.sqlite");
	Config cfg;

	double roi_x = cfg.getDouble("roi_x");
	
	cout << endl << "hit enter to exit";
	string str;
	getline(cin, str);
	return 0;
}