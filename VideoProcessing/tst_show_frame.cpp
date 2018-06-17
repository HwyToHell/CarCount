#include "stdafx.h"

#include "../include/frame_handler.h"



//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/// load inset from file
/// resize based on frame width
bool loadInset(std::string insetFilePath, cv::Size frameSize, cv::Mat& outInset) {
	using namespace std;

	// load inset image to display counting results on
	if (!isFileExist(insetFilePath)) {
		cerr << "loadInset: path to inset image does not exist: " << insetFilePath << endl;
	}

	cv::Mat inset_org = cv::imread(insetFilePath);
	if (inset_org.data) { 
		// use loaded image, fit to current frame size
		cv::Size insetSize = inset_org.size();
		double scaling = frameSize.width / (double)insetSize.width;
		cv::resize(inset_org, outInset, cv::Size(), scaling, scaling);
	} else { // create black matrix with same dimensions
		cerr << "loadInset: could not open " << insetFilePath << ", use substitute inset" << endl;
		int width = (int)frameSize.width;
		int height = 65;
		cv::Mat insetSubst(height, width, CV_8UC3, black);
		outInset = insetSubst;
	}
	return true;
}


/// create arrow pointing to left or right
/// based on inset origin
Arrow createArrow(const cv::Size inset, const Align pointsTo) {
	
	// layout
	// left border | arrow to left | divider | arrow to right | right border
	int length = inset.width * 10 / 24;
	int xBorder = inset.width * 1 / 24;
	int yBorder = inset.height * 3 / 24;

	// appearance
	int thickness = (inset.width > 200) ? (inset.width / 200) : 1;

	// arrow coords depending on points to
	cv::Point start(0,0), end(0,0);
	if (pointsTo == Align::left) {	// arrow to left
		start = cv::Point(xBorder + length, yBorder);
		end = cv::Point(xBorder, yBorder);
	} else {						// arrow to right
		start = cv::Point(inset.width - xBorder - length, yBorder);
		end = cv::Point(inset.width - xBorder, yBorder);
	}

	return Arrow(start, end, green, thickness);
}





/// copy inset into frame
void putInset(const cv::Mat& inset, cv::Mat& frame) {

	// show inset with vehicle icons and arrows
	if (inset.data) {
		// TODO adjust copy position depending on frame size
		int yInset = frame.rows - inset.rows; 
		inset.copyTo(frame(cv::Rect(0, yInset, inset.cols, inset.rows)));
	}
}





int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	//Size2i dispSize(160, 120);
	//Size2i dispSize(320, 240);
	//Size2i dispSize(480, 360);
	//Size2i dispSize(640, 480);
	Size2i dispSize(800, 600);
    string insetImage("/home/holger/counter/inset4.png");
	Mat frame(dispSize.height, dispSize.width, CV_8UC3, gray);
	Mat inset;

	loadInset(insetImage, dispSize, inset);

	//   create Arrow
	Arrow arrowLeft = createArrow(inset.size(), Align::left);
	Arrow arrowRight = createArrow(inset.size(), Align::right);

	//	 put arrow
	arrowLeft.put(inset);
	arrowRight.put(inset);


	putInset(inset, frame);
	
	imshow("frame", frame);
	imshow("inset", inset);

	waitKey(0);

	
	/*cout << "Press <enter> to exit" << endl;
	string str;
	getline(std::cin, str);
	*/
	return 0;
}
