#include "stdafx.h"

using namespace std;

int main(int argc, char* argv[]) {
	if ( (argc < 2) || (argc > 2) ) {
		cout << "usage: fourcc \"filename\"" << endl;
		return -1;
	}
	cv::VideoCapture cap;
	cap.open(argv[1]);
	if (!cap.isOpened()) {
		cerr << "cannot open file: " << argv[1] << endl;
		return -1;
	}
	int prop = (int)cap.get(CV_CAP_PROP_FOURCC);
	char fourcc[] = { (char)(prop & 0xFF), (char)((prop & 0xFF00) >> 8), (char)((prop & 0xFF0000) >> 16), (char)((prop & 0xFF000000) >> 24), 0 };
	cout << "fourcc: " << fourcc << endl;
	return 0;
}
