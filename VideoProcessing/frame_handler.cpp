#include "stdafx.h"
#include "../include/frame_handler.h"

#include <sstream>





FrameHandler::FrameHandler(Config* pConfig) : Observer(pConfig), mMog2(100, 25, false) {
	using namespace std;

	// instantiate cam input and enumerate available camera devices
	m_winCapture = std::unique_ptr<CamInput>(new CamInput);
	int iDevices = m_winCapture->enumerateDevices();

	// load inset image to display counting results on
	string inset_path = pConfig->getParam("video_path");
	inset_path += "inset3.png";
	cv::Mat inset_org = cv::imread(inset_path);
	if (!inset_org.data)
		cerr << "could not open " << inset_path << endl;
	cv::Size2d size;
	cv::resize(inset_org, mInset, size, 0.25, 0.25);

	update();
}

std::list<TrackEntry>& FrameHandler::calcBBoxes() {
	// find boundig boxes of newly detected objects, store them in mBBoxes and return them
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;

	mBBoxes.clear();

	cv::findContours(mFgrMask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	// calc bounding boxes of all detections and store in mBBoxes
	for (unsigned int i = 0; i < contours.size(); i++) { 
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > mBlobArea.min) && (bbox.area() < mBlobArea.max)) {
				mBBoxes.push_back(TrackEntry(bbox));
		}
	}
	
	return mBBoxes;
}

int FrameHandler::getFrameInfo() {
	int ch = mFrame.channels();
	int de = mFrame.depth();
	int type = mFrame.type();
	return type;
}


bool FrameHandler::openCapSource(bool fromFile) {
	using namespace std;

	//if (mCapture.isOpened())
	//	mCapture.release();
	if (fromFile) {
	/*	mCapture.open(mCapSource.path + mCapSource.fileName);
		if (!mCapture.isOpened()) {
			cerr << "Cannot open file: " << mCapSource.path << mCapSource.fileName << endl;
			return false;
		}*/
	} else {
	#if defined (_WIN32)
		m_winCapture->open(mCapSource.deviceName, 0); // TODO change resolution ID to private member
		if (!m_winCapture->isOpened()) {
			cerr << "Cannot open device: " << mCapSource.deviceName << endl;
			return false;
		}
	#elif defined (__linux__)
		mCapture.open(mCapSource.deviceName);
		if (!mCapture.isOpened()) {
			cerr << "Cannot open device: " << mCapSource.deviceName << endl;
			return false;
		}
	#else
		throw 1;
	#endif
	}
	
	mFrameCounter = 0;
	
	mFramesize.x = (int)m_winCapture->get(CV_CAP_PROP_FRAME_WIDTH);
	mFramesize.y = (int)m_winCapture->get(CV_CAP_PROP_FRAME_HEIGHT); 
	

	// roi for new frame size, based on normalized roi
	mRoi.x = mRoiNorm.x * mFramesize.x; 
	mRoi.width = mRoiNorm.width * mFramesize.x; 
	mRoi.y = mRoiNorm.y * mFramesize.y; 
	mRoi.height = mRoiNorm.height * mFramesize.y; 

	// area limits for new frame size, based on ratio of old frame area vs new frame area
	double min = mBlobArea.min;
	double max = mBlobArea.max;
	mBlobArea.min = (int) (min / mFrameArea * (double)(mFramesize.x * mFramesize.y) );
	mBlobArea.max = (int) (max / mFrameArea * (double)(mFramesize.x * mFramesize.y) );
	
	// set framesize and roi in config, notify all observers
	mSubject->setParam("frame_size_x", to_string((long long)mFramesize.x));
	mSubject->setParam("frame_size_y", to_string((long long)mFramesize.y));
	mSubject->setParam("roi_x", to_string((long long)mRoi.x));
	mSubject->setParam("roi_y", to_string((long long)mRoi.y));
	mSubject->setParam("roi_width", to_string((long long)mRoi.width));
	mSubject->setParam("roi_height", to_string((long long)mRoi.height));
	mSubject->setParam("blob_area_min", to_string((long long)mBlobArea.min));
	mSubject->setParam("blob_area_max", to_string((long long)mBlobArea.max));

	mSubject->notifyObservers();

	return true;
}


bool FrameHandler::openVideoOut(std::string fileName) {
	mVideoOut.open(mCapSource.path + fileName, CV_FOURCC('M','P','4','V'), 10, mFramesize);
	if (!mVideoOut.isOpened())
		return false;
	return true;
}


bool FrameHandler::segmentFrame() {
		bool isSuccess = m_winCapture->read(mFrame);
		if (!isSuccess) {
			std::cout << "Cannot read frame from capture" << std::endl;
			return false;
		}

		++mFrameCounter;

		// apply roi
		cv::Mat frame_roi = mFrame(mRoi);
		cv::Mat fPreProcessed, fgmask, fThresh, fMedBlur;

		// pre-processing
		cv::GaussianBlur(frame_roi, fPreProcessed, cv::Size(9,9), 2);

		// background segmentation 
		mMog2(fPreProcessed, fgmask);

		// post-processing
		cv::threshold(fgmask, fThresh, 0, 255, cv::THRESH_BINARY);	
		cv::medianBlur(fThresh, fMedBlur, 5);
		cv::dilate(fMedBlur, mFgrMask, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),1);
		return true;
}


void FrameHandler::showFrame(std::list<Track>& tracks, CountResults cr) {
		// show inset with vehicle icons and arrows
		mInset.copyTo(mFrame(cv::Rect(0,177, mInset.cols, mInset.rows)));
		
		// show frame counter, int font = cnt % 8;
		cv::putText(mFrame, std::to_string((long long)mFrameCounter), cv::Point(10,20), 0, 0.5, green, 1);
		cv::rectangle(mFrame, mRoi, blue);
		cv::Scalar boxColor = green;
		int line = Line::thin;

		// show tracking boxes around vehicles
		cv::Rect rec;
		std::list<Track>::iterator iTrack = tracks.begin();
		while (iTrack != tracks.end()){
			rec = iTrack->getActualEntry().rect();
			rec.x += (int)mRoi.x;
			rec.y += (int)mRoi.y;
			boxColor = red;
			line = Line::thin;
			if (iTrack->getConfidence() > 3)
				boxColor = orange;
			if (iTrack->isCounted()) {
				boxColor = green;
				line = Line::thick;
			}
			cv::rectangle(mFrame, rec, boxColor, line);
			++iTrack;
		}

		// show vehicle counter
		cv::putText(mFrame, std::to_string((long long)cr.carLeft), cv::Point(125,206), 0, 0.5, green, 2);
		cv::putText(mFrame, std::to_string((long long)cr.truckLeft), cv::Point(125,230), 0, 0.5, green, 2);
		cv::putText(mFrame, std::to_string((long long)cr.carRight), cv::Point(185,206), 0, 0.5, green, 2);
		cv::putText(mFrame, std::to_string((long long)cr.truckRight), cv::Point(185,230), 0, 0.5, green, 2);

		cv::imshow(mFrameWndName, mFrame);
}

void FrameHandler::update() {
	mCapSource.deviceName = stoi(mSubject->getParam("video_device"));
	mCapSource.fileName = mSubject->getParam("video_file");
	mCapSource.path = mSubject->getParam("video_path");
	mFramesize.x = stoi(mSubject->getParam("frame_size_x"));
	mFramesize.y = stoi(mSubject->getParam("frame_size_y"));
	// region of interest
	mRoi.x = stod(mSubject->getParam("roi_x"));
	mRoi.y = stod(mSubject->getParam("roi_y"));
	mRoi.width = stod(mSubject->getParam("roi_width"));
	mRoi.height = stod(mSubject->getParam("roi_height"));
	// normalized roi, related to framesize
	mRoiNorm.x = mRoi.x / mFramesize.x; 
	mRoiNorm.width = mRoi.width / mFramesize.x;
	mRoiNorm.y = mRoi.y / mFramesize.y;
	mRoiNorm.height = mRoi.height / mFramesize.y;
	
	// mBlobArea.min(200)		-> smaller blobs will be discarded 320x240=100   640x480=500
	// mBlobArea.max(20000)		-> larger blobs will be discarded  320x240=10000 640x480=60000
	mBlobArea.min = stoi(mSubject->getParam("blob_area_min"));
	mBlobArea.max = stoi(mSubject->getParam("blob_area_max"));
	// normalized blob area, related to frame size
	mFrameArea = mFramesize.x * mFramesize.y;
}


void FrameHandler::writeFrame() {
	mVideoOut.write(mFrame);
}

int FrameHandler::getFrameCount() {
	return mFrameCounter;
}