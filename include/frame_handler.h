#pragma once
#include <memory> // shared_ptr
#include "cam_cap_dshow.h"
#include "config.h"
#include "recorder.h"


typedef cv::Rect_ <double> Rect2d; // delete, if opencv > v3.0 
const cv::Scalar black	= cv::Scalar(0,0,0);
const cv::Scalar blue	= cv::Scalar(255,0,0);
const cv::Scalar red	= cv::Scalar(0,0,255);
const cv::Scalar green	= cv::Scalar(0,255,0);
const cv::Scalar orange = cv::Scalar(0,128,255);
const cv::Scalar yellow = cv::Scalar(0,255,255);
const cv::Scalar white	= cv::Scalar(255,255,255);

struct Line{
	enum thickness { thin=1, thick=2};
};

class FrameHandler : public Observer {
public:
	FrameHandler(Config* pConfig);
	void					adjustFrameSizeDependentParams(int new_size_x, int new_size_y); 
	std::list<TrackEntry>&	calcBBoxes();
	int						getFrameInfo(); // #channels, depth, type
	cv::Size2d				getFrameSize();
	bool					initCam(int camID = 0, int camResoutionID = 0);
	bool					initFileReader(std::string videoFilePath);
	bool					loadInset(std::string insetFile);
	std::string				locateFilePath(std::string fileName);
	bool					openCapSource(bool fromFile = true); // wraps initCam() and initFileReader()
	bool					openVideoOut(std::string fileName);
	bool					segmentFrame();
	void					showFrame(std::list<Track>& tracks, CountResults cr);
	void					writeFrame();
	// DEBUG
	int						getFrameCount();
private:
	void					update(); // updates observer with subject's parameters (Config)

	typedef cv::BackgroundSubtractorMOG2 BackgrndSubtrac;
	std::list<TrackEntry>		mBBoxes; // bounding boxes of newly detected objects
	struct AreaLimits {
		int min;
		int max; }				mBlobArea;
	struct CamProps {
		int deviceID;
		int resolutionID; }		m_camProps;
	cv::VideoCapture			m_capture;
	std::unique_ptr<CamInput>	m_captureWinCam; // substitute for cv::VideoCapture
	std::string					m_inVideoFilePath;
	cv::Mat						mFrame;
	cv::Mat						mFgrMask; // foreground mask of moving objects
	int							mFrameCounter; 
	cv::Size2d					m_frameSize;
	std::string					mFrameWndName;
	cv::Mat						m_inset;
	bool						m_isCaptureInitialized;
	bool						m_isFileCapture; 
	BackgrndSubtrac				mMog2;
	Rect2d						mRoi;	 // region of interest, within framesize
	cv::VideoWriter				mVideoOut;
};
