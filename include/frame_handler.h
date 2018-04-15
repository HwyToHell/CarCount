#pragma once
#include <memory> // shared_ptr
#include "cam_cap_dshow.h"
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
public:
	FrameHandler(Config* pConfig);
	std::list<TrackEntry>&	calcBBoxes();
	int						getFrameInfo();
	bool					openCapSource(bool fromFile = true);
	bool					openVideoOut(std::string fileName);
	bool					segmentFrame();
	cv::Size2d				setCamResolution(int defaultFrameSizeId);
	void					showFrame(std::list<Track>& tracks, CountResults cr);
	void					update(); // updates observer with subject's parameters (Config)
	void					writeFrame();
	// DEBUG
	int						getFrameCount();
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

	typedef cv::BackgroundSubtractorMOG2 BackgrndSubtrac;
	
	std::list<TrackEntry>		mBBoxes; // bounding boxes of newly detected objects
	//cv::VideoCapture			mCapture;
	std::unique_ptr<CamInput>	m_winCapture; // substitute for cv::VideoCapture
	cv::Mat						mFrame;
	cv::Mat						mFgrMask; // foreground mask of moving objects
	int							mFrameCounter; 
	cv::Point2i					mFramesize;
	std::string					mFrameWndName;
	cv::Mat						mInset;
	BackgrndSubtrac				mMog2;
	Rect2d						mRoi;	 // region of interest, within framesize
	// TODO check, if can be deleted
	Rect2d						mRoiNorm; // normalized roi, related to framesize
	cv::VideoWriter				mVideoOut;
};
