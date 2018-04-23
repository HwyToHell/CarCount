#include "stdafx.h"

#include "../include/config.h"


int main(int argc, char* argv[]) {
	using namespace std;
	
	string home = getHome();
	appendDirToPath(home, "\\traffic.avi");
	
	cout << "Press <enter> to exit" << endl;
	string str;
	getline(std::cin, str);
	return 0;
}



