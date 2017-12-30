#include "stdafx.h"
#include "../include/frame_handler.h"

FrameHandler::FrameHandler(Config* pConfig) : Observer(pConfig), mMog2(100, 25, false) {
	update();
}

void FrameHandler::update() {
	mCapSource.deviceName = stoi(mSubject->getParam("video_device"));
	mCapSource.fileName = mSubject->getParam("video_file");
	mCapSource.path = mSubject->getParam("video_path");
	mFramesize.x = stoi(mSubject->getParam("framesize_x"));
	mFramesize.y = stoi(mSubject->getParam("framesize_y"));
	// region of interest
	mRoi.x = mSubject->getDouble("roi_x");
	mRoi.y = mSubject->getDouble("roi_y");
	mRoi.width = mSubject->getDouble("roi_width");
	mRoi.height = mSubject->getDouble("roi_height");
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

std::list<TrackEntry>& FrameHandler::calcBBoxes() {
	// find boundig boxes of newly detected objects, store them in mBBoxes and return them
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	mBBoxes.clear();

	cv::findContours(mFgrMask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	// calc bounding boxes of all detections and store in mBBoxes
	for (unsigned int i = 0; i < contours.size(); i++) { 
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > mBlobArea.min) && (bbox.area() < mBlobArea.max)) {
				mBBoxes.push_back(TrackEntry(bbox, i));
		}
	}
	
	return mBBoxes;
}

void FrameHandler::drawVehicles(list<Vehicle>& vehicles) {
	
	// draw ROI
	//cv::rectangle(mFrame, mRoi, blue);
}


bool FrameHandler::openCapSource(bool fromFile) {
	if (mCapture.isOpened())
		mCapture.release();
	if (fromFile) {
		mCapture.open(mCapSource.path + mCapSource.fileName);
		if (!mCapture.isOpened()) {
			cerr << "Cannot open file: " << mCapSource.path << mCapSource.fileName << endl;
			return false;
		}
	} 
	else {
		mCapture.open(mCapSource.deviceName);
		if (!mCapture.isOpened()) {
			cerr << "Cannot open device: " << mCapSource.deviceName << endl;
			return false;
		}
	}
	
	mFrameCounter = 0;
	
	mFramesize.x = (int)mCapture.get(CV_CAP_PROP_FRAME_WIDTH);
	mFramesize.y = (int)mCapture.get(CV_CAP_PROP_FRAME_HEIGHT); 

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
	mSubject->setParam("framesize_x", to_string((long long)mFramesize.x));
	mSubject->setParam("framesize_y", to_string((long long)mFramesize.y));
	mSubject->setParam("roi_x", to_string((long long)mRoi.x));
	mSubject->setParam("roi_y", to_string((long long)mRoi.y));
	mSubject->setParam("roi_width", to_string((long long)mRoi.width));
	mSubject->setParam("roi_height", to_string((long long)mRoi.height));
	mSubject->setParam("blob_area_min", to_string((long long)mBlobArea.min));
	mSubject->setParam("blob_area_max", to_string((long long)mBlobArea.max));

	mSubject->notifyObservers();

	return true;
}


bool FrameHandler::openVideoOut(string fileName) {
	mVideoOut.open(mCapSource.path + fileName, CV_FOURCC('M','P','4','V'), 10, mFramesize);
	if (!mVideoOut.isOpened())
		return false;
	return true;
}


bool FrameHandler::segmentFrame() {
		bool isSuccess = mCapture.read(mFrame);
		if (!isSuccess) {
			cout << "Cannot read frame from capture" << endl;
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

void FrameHandler::showFrame(list<Track>& tracks, list<Vehicle>& vehicles) {
		// show frame counter, int font = cnt % 8;
		cv::putText(mFrame, to_string((long long)mFrameCounter), cv::Point(10,35), 0, 1.0, red, 2);
		cv::rectangle(mFrame, mRoi, blue);
		cv::Scalar boxColor = green;

		cv::Rect rec;
		list<Vehicle>::iterator iVehicle = vehicles.begin();
		while (iVehicle != vehicles.end()){
			rec = iVehicle->getBbox();
			rec.x += (int)mRoi.x;
			rec.y += (int)mRoi.y;
			//cv::rectangle(mFrame, rec, red, 2);
			++iVehicle;
		}
		list<Track>::iterator iTrack = tracks.begin();
		while (iTrack != tracks.end()){
			rec = iTrack->getActualEntry().mBbox;
			rec.x += (int)mRoi.x;
			rec.y += (int)mRoi.y;
			boxColor = green;
			if (iTrack->getConfidence() > 3)
				boxColor = orange;
			if (iTrack->isCounted())
				boxColor = red;
			cv::rectangle(mFrame, rec, boxColor, 1);
			++iTrack;
		}


		cv::imshow(mFrameWndName, mFrame);
}


void FrameHandler::writeFrame() {
	mVideoOut.write(mFrame);
}

int FrameHandler::getCounter() {
	return mFrameCounter;
}