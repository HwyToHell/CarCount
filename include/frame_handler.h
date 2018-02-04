#pragma once
#include "config.h"
#include "recorder.h"


typedef cv::Rect_ <double> Rect2d; // delete, if opencv > v3.0 
const cv::Scalar blue = cv::Scalar(255,0,0);
const cv::Scalar red = cv::Scalar(0,0,255);
const cv::Scalar green = cv::Scalar(0,255,0);
const cv::Scalar orange = cv::Scalar(0,128,255);
const cv::Scalar yellow = cv::Scalar(0,255,255);
const cv::Scalar white = cv::Scalar(255,255,255);

struct Line{
	enum thickness { thin=1, thick=2};
};

class FrameHandler : public Observer {
private:
	struct AreaLimits {
		int min;
		int max;
	} mBlobArea;
	double mFrameArea; // area of current framesize
	struct CaptureSource {
		int deviceName;
		std::string fileName;
		std::string path;
	} mCapSource;
	cv::VideoCapture mCapture;
	int mFrameCounter; 
	cv::Point2i mFramesize;
	Rect2d mRoi;	 // region of interest, within framesize
	Rect2d mRoiNorm; // normalized roi, related to framesize
	cv::Mat mFrame;
	std::string mFrameWndName;
	cv::Mat mFgrMask; // foreground mask of moving objects
	cv::Mat mInset;
	cv::BackgroundSubtractorMOG2 mMog2;
	std::list<TrackEntry> mBBoxes; // bounding boxes of newly detected objects
	cv::VideoWriter mVideoOut;
public:
	FrameHandler(Config* pConfig);
	std::list<TrackEntry>& calcBBoxes();
	int getFrameInfo();
	bool openCapSource(bool fromFile = true);
	bool openVideoOut(std::string fileName);
	bool segmentFrame();
	void showFrame(std::list<Track>& tracks, CountResults cr);
	void update(); // updates observer with subject's parameters (Config)
	void writeFrame();
	// DEBUG
	int getFrameCount();
};
