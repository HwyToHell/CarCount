#include "stdafx.h"
#include "tracking.h"


using namespace std;
using namespace cv;



cv::Mat loadMask();	
void shiftMaskToRight(const cv::Mat& fgMask, cv::Mat& fgMask_shifted);
void FindBlobs(const cv::Mat& mask, cv::Mat& frame, cv::Mat& maskMovingObj);




TEST_CASE( "findContours", "[FGMaskPostProcessing]" ) 
{
	Mat fgMask = loadMask();

	Scene scene;
	Mat fgMask_act, fgMask_next;
	fgMask.copyTo(fgMask_act);

	for (int n=0; n<9; ++n)
	{
		// find contours
		vector<vector<cv::Point>> contours, movingContours;
		vector<cv::Vec4i> hierarchy;
		cv::Mat fgMaskContour;
		fgMask_act.copyTo(fgMaskContour); // findContours algorithm destroys original image
		cv::findContours(fgMaskContour, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
		
		scene.UpdateTracksFromContours(contours, movingContours);
		scene.CombineTracks();
		std::vector<int> movingContourIndices;
		movingContourIndices = scene.getAllContourIndices();


		// build mask from moving contours
		cv::Mat fgMovingMask(fgMask_act.size(), CV_8UC1, Scalar(0));
		for (unsigned int i = 0; i < movingContourIndices.size(); ++i)
		{
			int contourIdx = movingContourIndices[i];
			cv::drawContours(fgMovingMask, contours, contourIdx, 255, CV_FILLED, 8);
		}

		// output for debugging
		imshow("Scene Input", fgMask_act);
		imshow("MovingMask Output", fgMovingMask);

		// shift current mask to right 
		shiftMaskToRight(fgMask_act, fgMask_next);
		fgMask_act = fgMask_next;

	}
	cv::waitKey(10);

}



cv::Mat loadMask()
{
	// read test file and threshold it
	string fgMaskFile = "..\\Dataset\\f200.jpg";
	Mat fgMask, fgMask_raw;
	fgMask_raw = imread(fgMaskFile, CV_LOAD_IMAGE_GRAYSCALE);
	cv::threshold(fgMask_raw, fgMask, 200, 255, cv::THRESH_BINARY);
	imshow("Threshold", fgMask);
	return fgMask;
}

void shiftMaskToRight(const cv::Mat& fgMask, cv::Mat& fgMask_shifted)
{
	// ToDo: copy fgMask to fgMask_shifted and move content 2 pixels to the right
	int shift = 2;
	cv::Size fgSize = fgMask.size();
	cv::Size sliceSize(2, fgSize.height);
	// 2 background pixel columns introduces from left
	cv::Mat slice(sliceSize, CV_8UC1, Scalar(0));
	// merged to new fgMask_shifted;
	cv::hconcat(slice, fgMask(Range::all(), Range(0, fgSize.width-shift)), fgMask_shifted);
}

void buildMask(const std::vector<std::vector<cv::Point>>& movingContours, cv::Mat& fgMovingMask)
{
	for (unsigned int i = 0; i < movingContours.size(); i++)
		cv::drawContours(fgMovingMask, movingContours, i, 255, CV_FILLED, 8);
}




