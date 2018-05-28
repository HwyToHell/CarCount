#include "stdafx.h"

#include "../include/frame_handler.h"

bool loadInset(std::string insetFilePath, cv::Size frameSize, cv::Mat& outInset);
void composeFrame(const cv::Mat& inset, cv::Mat& frame);

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	Size2i dispSize(320, 240);
	string insetImage("D:\\Users\\Holger\\counter\\inset3.png");
	Mat frame(dispSize.height, dispSize.width, CV_8UC3, gray);
	Mat inset;

	loadInset(insetImage, dispSize, inset);
	composeFrame(inset, frame);

	imshow("inset", inset);
	imshow("frame", frame);

	waitKey(0);

	
	/*cout << "Press <enter> to exit" << endl;
	string str;
	getline(std::cin, str);
	*/
	return 0;
}



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

void composeFrame(const cv::Mat& inset, cv::Mat& frame) {
	// show inset with vehicle icons and arrows
	if (inset.data) {
		// TODO adjust copy position depending on frame size
		int yInset = frame.rows - inset.rows; 
		inset.copyTo(frame(cv::Rect(0, yInset, inset.cols, inset.rows)));
	}

	int baseLine;
	cv::Size textSize = cv::getTextSize("10", 0, 0.5, 2, &baseLine);
	cv::Size textSize10 = cv::getTextSize("10", 0, 1, 2, &baseLine);
	cv::Size textSize20 = cv::getTextSize("10", 0, 2, 2, &baseLine);

	cv::Point bottomLeft(125,206);
	cv::Point topRight(bottomLeft.x + textSize.width, bottomLeft.y - textSize.height);
	cv::rectangle(frame, bottomLeft + cv::Point(0, 2), topRight, red);

	// show vehicle counter
	cv::putText(frame, "10", bottomLeft, 0, 0.48, green, 2);
	cv::putText(frame, "2", cv::Point(125,230), 0, 0.5, green, 2);
	cv::putText(frame, "11", cv::Point(185,206), 0, 0.5, green, 2);
	cv::putText(frame, "3", cv::Point(185,230), 0, 0.5, green, 2);
}