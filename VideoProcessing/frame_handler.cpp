#include "stdafx.h"
#include "../include/frame_handler.h"

//#include <exception>
#include <sstream>





FrameHandler::FrameHandler(Config* pConfig) : 
	Observer(pConfig), 
	mMog2(100, 25, false),
	m_isCaptureInitialized(false),
	m_captureWinCam(nullptr) {

	// instantiate cam input
	#if defined (_WIN32)
	m_captureWinCam = std::unique_ptr<CamInput>(new CamInput);
	#endif

	// retrieve config data
	update();
}


void FrameHandler::adjustFrameSizeDependentParams(int new_size_x, int new_size_y) {
	using namespace std;
	int old_size_x = stoi(mSubject->getParam("frame_size_x"));
	int old_size_y = stoi(mSubject->getParam("frame_size_y"));

	try {
		// roi
		long long roi_x = stoi(mSubject->getParam("roi_x")) * new_size_x / old_size_x;
		long long roi_width =  stoi(mSubject->getParam("roi_x")) * new_size_x / old_size_x;
		long long roi_y = stoi(mSubject->getParam("roi_y")) * new_size_y / old_size_y;
		long long roi_height = stoi(mSubject->getParam("roi_y")) * new_size_y / old_size_y;	
		mSubject->setParam("roi_x", to_string(roi_x));
		mSubject->setParam("roi_width", to_string(roi_width));
		mSubject->setParam("roi_y", to_string(roi_y));
		mSubject->setParam("roi_height", to_string(roi_height));
		
		// inset
		long long inset_height = stoi(mSubject->getParam("inset_height")) * new_size_y / old_size_y;
		mSubject->setParam("inset_height", to_string(inset_height));

		// blob assignment
		long long blob_area_min =  stoi(mSubject->getParam("blob_area_min")) * new_size_x * new_size_y 
			/ old_size_x / old_size_y;
		long long blob_area_max =  stoi(mSubject->getParam("blob_area_max")) * new_size_x * new_size_y 
			/ old_size_x / old_size_y;
		mSubject->setParam("blob_area_min", to_string(blob_area_min));
		mSubject->setParam("blob_area_max", to_string(blob_area_max));

		// track assignment
		long long track_max_dist =  stoi(mSubject->getParam("track_max_distance")) * (new_size_x / old_size_x
			+ new_size_y / old_size_y) / 2;
		mSubject->setParam("track_max_distance", to_string(track_max_dist));

		// count pos, count track length
		long long count_pos_x =  stoi(mSubject->getParam("count_pos_x")) * new_size_x / old_size_x;
		mSubject->setParam("count_pos_x", to_string(count_pos_x));
		long long count_track_length =  stoi(mSubject->getParam("count_track_length")) * new_size_x / old_size_x;
		mSubject->setParam("count_track_length", to_string(count_track_length));

		// truck_size
		long long truck_width_min =  stoi(mSubject->getParam("truck_width_min")) * new_size_x / old_size_x;
		mSubject->setParam("truck_width_min", to_string(truck_width_min));

		// TODO next line "invalid argument exception"
		long long truck_height_min =  stoi(mSubject->getParam("truck_height_min")) * new_size_y / old_size_y;
		mSubject->setParam("truck_height_min", to_string(truck_height_min));

		// save new frame size in config
		mSubject->setParam("frame_size_x", to_string((long long)new_size_x));
		mSubject->setParam("frame_size_y", to_string((long long)new_size_y));

		mSubject->notifyObservers();
	}
	catch (exception& e) {
		cerr << "invalid parameter in config: " << e.what() << endl;
	}
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


cv::Size2d FrameHandler::getFrameSize() {
	using namespace std;
	// check, if capture source is already initialized
	if (!m_isCaptureInitialized) {
		cerr << "capture must be initialized" << endl;
		return cv::Size2d(0,0);
	} else {
		return m_frameSize;
	}
}

// open cam device, save resolution to m_frameSize
bool FrameHandler::initCam(int camID, int camResolutionID) {
	using namespace std;

	// check, if capture source is already initialized
	if (m_isCaptureInitialized) {
		cerr << "initCam: capture already initialized" << endl;
		return false;
	}

	// enumerate available camera devices
	#if defined (_WIN32)
	m_captureWinCam = std::unique_ptr<CamInput>(new CamInput);
	int iDevices = m_captureWinCam->enumerateDevices();
	
	// camID must be in range
	if (camID < 0 || camID > iDevices) {
		cerr << "initCam: camID: "<< camID << " out of device range" << endl;
		return false;
	} 

	// open cam device with implicit cam resolution selection list
	if (!m_captureWinCam->open(camID, camResolutionID)) {
		cerr << "initCam: cannot open cam" << endl;
		return false;
	}

	#elif defined (__linux__)
	// use cv::Capture
	m_capture.open(camID);
	if (!m_capture.isOpened()) {
		cerr << "initCam: cannot open cam: " << camID << endl;
		return false;
	}
	m_frameSize.width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
	m_frameSize.height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (m_frameSize.height == 0 || m_frameSize.width == 0) {
		cerr << "initCam: wrong cam resolution" << endl;
		return false;
	}
	#else
		throw "unsupported OS";
	#endif

	m_isCaptureInitialized = true;
	m_isFileCapture = false;
	return true;
}


// open video file, save resolution to m_frameSize
bool FrameHandler::initFileReader(std::string videoFilePath) {
	using namespace std;

	// check, if capture source is already initialized
	if (m_isCaptureInitialized) {
		cerr << "initFileReader: capture already initialized" << endl;
		return false;
	}

	// try open video input file
	m_capture.open(videoFilePath);
	if (!m_capture.isOpened()) {
		cerr << "initFileReader: cannot open file: " << videoFilePath << endl;
		return false;
	}

	// retrieve frame size
	m_frameSize.width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
	m_frameSize.height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (m_frameSize.height == 0 || m_frameSize.width == 0) {
		cerr << "initFileReader: wrong cam resolution" << endl;
		return false;
	}

	m_isCaptureInitialized = true;
	m_isFileCapture = true;
	return true;
}

// locate video file, save full path to m_inVideoFilePath
// return empty string, if video file was not found in the search directories
// search order: 1. current directory, 2. /home/work_dir
std::string FrameHandler::locateFilePath(std::string fileName) {
	using namespace std;
	string path;

	// 1. check current_dir
	string currentPath;
	#if defined (_WIN32)
		char bufPath[_MAX_PATH];
		_getcwd(bufPath, _MAX_PATH);
		if (bufPath) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (GetLastError() == ERANGE)
				cerr << "locateVideoFile: path length > _MAX_PATH" << endl;
			return path;
		}
	#elif defined (__linux__)
		char bufPath[PATH_MAX];
		getcwd(bufPath, PATH_MAX);
		if (bufPath) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (errno == ENAMETOOLONG)
				cerr << "locateVideoFile: path length > PATH_MAX" << endl;
			return false;
		}
	#else
		throw "unsupported OS";
	#endif
	path = currentPath;
	appendDirToPath(path, fileName);
	if (isFileExist(path)) {
		m_inVideoFilePath = path;
		return path;
	}

	// 2. check /home/app_dir
	path = mSubject->getParam("application_path");
	appendDirToPath(path, fileName);
	if (isFileExist(path)) {
		m_inVideoFilePath = path;
		return path;
	}

	// file not found
	cerr << "locateVideoFile: " << fileName << " was not found" << endl;
	return string("");
}

bool FrameHandler::loadInset(std::string insetFile) {
	using namespace std;
	// capture source must be initialized in order to obtain correct frame size
	if (!m_isCaptureInitialized) {
		cerr << "loadInset: capture source must be initialized" << endl;
		return false;
	}

	// load inset image to display counting results on
	string inset_path = mSubject->getParam("application_path");
	inset_path += insetFile;
	if (!isFileExist(inset_path)) {
		cerr << "loadInset: path to inset image does not exist: " << inset_path << endl;
	}

	cv::Mat inset_org = cv::imread(inset_path);
	if (inset_org.data) { 
		// use loaded image, fit to current frame size
		cv::Size insetSize = inset_org.size();
		double scaling = m_frameSize.width / (double)insetSize.width;
		cv::resize(inset_org, m_inset, cv::Size(), scaling, scaling);
	} else { // create black matrix with same dimensions
		cerr << "loadInset: could not open " << inset_path << ", use substitute inset" << endl;
		int width = (int)m_frameSize.width;
		int height = stoi(mSubject->getParam("inset_height"));
		cv::Mat insetSubst(height, width, CV_8UC3, black);
		m_inset = insetSubst;
	}

	return true;
}


bool FrameHandler::openCapSource(bool fromFile) {
	using namespace std;

	if (fromFile) {
		string videoFile = mSubject->getParam("video_file");
		string filePath = locateFilePath(videoFile);
		if (filePath == "") 
			return false;
		bool succ = initFileReader(filePath);
		return succ;
	} else {
		bool succ = initCam(m_camProps.deviceID, m_camProps.resolutionID);
		return succ;
	}

}


bool FrameHandler::openVideoOut(std::string fileName) {
	std::string path = mSubject->getParam("application_path");
	mVideoOut.open(path + fileName, CV_FOURCC('M','P','4','V'), 10, m_frameSize);
	if (!mVideoOut.isOpened())
		return false;
	return true;
}


bool FrameHandler::segmentFrame() {
	bool isSuccess = false;

	// capture source must be initialized in order to do segementation
	if (!m_isCaptureInitialized) {
		std::cerr << "segmentFrame: capture source must be initialized" << std::endl;
		return false;
	}

	// split between file and cam capture (Win32)
	if (m_isFileCapture) {
		isSuccess = m_capture.read(mFrame);
		if (!isSuccess) {
			std::cout << "segmentFrame: annot read frame from file" << std::endl;
			return false;
		}

	} else { // isCamCapture
		#if defined (_WIN32)
		isSuccess = m_captureWinCam->read(mFrame);
		if (!isSuccess) {
			std::cout << "segmentFrame: cannot read frame from cam" << std::endl;
			return false;
		}

		#elif defined (__linux__)
		isSuccess = m_capture.read(mFrame);
		if (!isSuccess) {
			std::cout << "segmentFrame: cannot read frame from cam" << std::endl;
			return false;
		}

		#else
			throw "unsupported OS";
		#endif
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
	if (m_inset.data)
		// TODO adjust copy position depending on frame size
		m_inset.copyTo(mFrame(cv::Rect(0,177, m_inset.cols, m_inset.rows)));
		
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
	m_camProps.deviceID = stoi(mSubject->getParam("cam_device_ID"));
	m_camProps.resolutionID = stoi(mSubject->getParam("cam_resolution_ID"));

	m_frameSize.width = stoi(mSubject->getParam("frame_size_x"));
	m_frameSize.height = stoi(mSubject->getParam("frame_size_y"));
	// region of interest
	mRoi.x = stod(mSubject->getParam("roi_x"));
	mRoi.y = stod(mSubject->getParam("roi_y"));
	mRoi.width = stod(mSubject->getParam("roi_width"));
	mRoi.height = stod(mSubject->getParam("roi_height"));
	
	// mBlobArea.min(200)		-> smaller blobs will be discarded 320x240=100   640x480=500
	// mBlobArea.max(20000)		-> larger blobs will be discarded  320x240=10000 640x480=60000
	mBlobArea.min = stoi(mSubject->getParam("blob_area_min"));
	mBlobArea.max = stoi(mSubject->getParam("blob_area_max"));
}


void FrameHandler::writeFrame() {
	mVideoOut.write(mFrame);
}

int FrameHandler::getFrameCount() {
	return mFrameCounter;
}