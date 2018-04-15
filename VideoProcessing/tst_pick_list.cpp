#include "stdafx.h"
#include "../../../cpp/inc/pick_list.h"
#include "../../../cpp/inc/rlutil.h"



int pick_list_main(int argc, char* argv[]) {
	using namespace std;
	using namespace rlutil;

	PickList::StrArray itemArray;
	itemArray.push_back("first item");
	itemArray.push_back("second item");
	itemArray.push_back("third item");
	itemArray.push_back("fourth item");

	// origin
	int offsetX = 1;
	int offsetY = 1;
	cout << "Here comes the pick list ..." << endl;
	cout << "press <enter> to create pick list at: " << 
		"x=" << offsetX << ", y=" << offsetY << endl;
	string str;
	getline(std::cin, str);

	// create first pick list at origin
	PickList device("video device", &itemArray, offsetY);
	int selIdx = device.getSelection();
	cout << "selected index: " << selIdx << endl << endl; 

	// create second pick list right after (offsetY = 0)
	PickList resolution("resolution", &itemArray);
	selIdx = resolution.getSelection();
	cout << "selected index: " << selIdx << endl; 
	
	cout << "Press <enter> to exit" << endl;
	getline(std::cin, str);
	return 0;
}