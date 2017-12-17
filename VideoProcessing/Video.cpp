#include "stdafx.h"
#include "keypress.h"

#include "../include/config.h"
#include "../include/tracking.h"


using namespace std;

list<TrackEntry> findShapes(const cv::Mat& mask);
void drawShapes(list<TrackEntry> shapeList, cv::Mat& frame);

list<TrackEntry> findShapes2(const cv::Mat& mask, cv::Mat& frame);

void FindBlobs(const cv::Mat& mask, cv::Mat& frame);

	
typedef cv::Rect_ <double> Rect2d; 

int main(int argc, char* argv[])
{
	Config config("video.sqlite");
	Config* pConfig = &config;

	Scene scene(pConfig); // collection of tracks and vehicles with MOG2
	Scene* pScene = &scene;
	config.attachScene(pScene);

	cv::BackgroundSubtractorMOG2 mog2(100, 15, 0);
	list<TrackEntry> newBlobs; // stores new identified objects, that have not been assigned to tracks yet, for MOG2 algorithm

	cv::VideoCapture cap("D:/Holger/AppDev/OpenCV/VideoProcessing/DataSet/320x240_10fps/_Bus_320x240_10fps.avi");
	if (!cap.isOpened())
	{
		cout << "Cannot open video file" << endl;
		return -1;
	}
	cv::Point2d framesize(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT)); 
	config.changeParam("framesize_x", to_string((long double)framesize.x));
	config.changeParam("framesize_y", to_string((long double)framesize.y));

	// define region of interest --> TODO: select graphical -or- detect by optical flow
	cv::Rect roi(80, 80, 180, 80);

	Rect2d roiNorm(roi);
	roiNorm.x /= framesize.x; 
	roiNorm.width /= framesize.x;
	roiNorm.y /= framesize.y;
	roiNorm.height /= framesize.y;


	config.changeParam("roi_x", to_string((long double)roi.x));
	config.changeParam("roi_y", to_string((long double)roi.y));
	config.changeParam("roi_width", to_string((long double)roi.width));
	config.changeParam("roi_height", to_string((long double)roi.height));

	cv::Mat frame, frame_roi, fgMaskMOG2, fgMask, fBackgrnd, fPreProcessed, fPostProcessed, fMedBlur;
	//cv::BackgroundSubtractorMOG mog = cv::BackgroundSubtractorMOG(100, 3, 0.8);
	cv::Mat krnl = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(-1,-1));

	cv::namedWindow("Origin");
	cv::namedWindow("MOG2+Dilate");


	int cnt = 0;

	while(true)
	{
		bool isSuccess = cap.read(frame);
		if (!isSuccess)
		{
			cout << "Cannot read frame from video file" << endl;
			break;
		}
		cnt++;

		/*********************************************************************/
		/* Pre-Processing ****************************************************/
		/*********************************************************************/
		frame_roi = frame(roi);
		cv::GaussianBlur(frame_roi, fPreProcessed, cv::Size(9,9), 2);

		// show frame counter, int font = cnt % 8;
		cv::putText(frame, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(0, 0,255), 2);
		cv::imshow("Origin", frame);

		/*********************************************************************/
		/* Background Segmentation *******************************************/
		/*********************************************************************/
		// MOG 
		mog2(fPreProcessed, fgMaskMOG2); // std: 0.01
		mog2.getBackgroundImage(fBackgrnd);
		

		/*********************************************************************/
		/* Post-Processing ***************************************************/
		/*********************************************************************/
		// MOG
		cv::threshold(fgMaskMOG2, fgMask, 0, 255, cv::THRESH_BINARY);	
		cv::medianBlur(fgMask, fMedBlur, 5);
		//cv::imshow("MOG2", fgMask);
		cv::dilate(fMedBlur, fPostProcessed, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),1);
		cv::imshow("MOG2+Dilate", fPostProcessed);
		cv::putText(fgMask, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(127, 127, 127), 2);
		
		newBlobs = findShapes2(fPostProcessed, frame);	

		/// TODO
		/// findShapes + 
		/// drawShapes(shapeList, frame) 

		scene.updateTracks(newBlobs);
		scene.showTracks(frame);
		scene.combineTracks();
		scene.showVehicles(frame);
		imshow("Detect", frame);
		

	
		
		StopStartNextFrame();
		if (cv::waitKey(10) == 27)
		{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}


	cv::waitKey(0);
	return 0;
}



list<TrackEntry> findShapes2(const cv::Mat& mask, cv::Mat& frame)
{
	/// @todo
	/// implement ROI as parameter in scene
	const cv::Rect sceneROI(80, 80, 180, 80);

	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	list<TrackEntry> newShapes; // stores new identified objects, that have not been assigned to tracks yet
	const cv::Scalar blue = cv::Scalar(255,0,0);

	// draw ROI
	cv::rectangle(frame, sceneROI, blue);


	// detect new moving objects, store in list newShapes
	cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	cout << endl << "contours: " << contours.size() << endl;
	for (unsigned int i = 0; i < contours.size(); i++) // calc bounding boxes of new detection
	{
		cv::drawContours(frame, contours, i, blue, CV_FILLED, 8, hierarchy, 0, cv::Point() );
		cv::Rect bbox = boundingRect(contours[i]);
		if (sceneROI.contains(bbox.tl()) && sceneROI.contains(bbox.br()))
			if ((bbox.area() > 100) && (bbox.area() < 10000))
			{
				newShapes.push_back(TrackEntry(bbox, i));
			
			}
	}
	

/*
	const int thickness = 1;
	list<TrackEntry>::iterator it = newShapes.begin();
	while (it != newShapes.end()) 
	{

		cv::Rect boxPos(it->mBbox.x-2, it->mBbox.y-2, it->mBbox.width+4, it->mBbox.height+4);
		cv::rectangle(frame, boxPos, blue, thickness);
		++it;
	}
*/
	return newShapes;
}

list<TrackEntry> findShapes(const cv::Mat& mask) {
	
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	list<TrackEntry> shapes; // stores new identified objects, that have not been assigned to tracks yet
	
	// detect new moving objects, store in list newShapes
	cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));

	for (unsigned int i = 0; i < contours.size(); i++) }// calc bounding boxes of new detection
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > 100) && (bbox.area() < 10000)) {
			newShapes.push_back(TrackEntry(bbox, i));
		}
	}
	
	return newShapes;
}



void FindBlobs(const cv::Mat& mask, cv::Mat& frame, cv::Mat& maskMovingObj)
{
	const cv::Scalar blue = cv::Scalar(255,0,0);
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;

	// detect new moving objects, store in list newShapes
	cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));



	cout << endl << "contours: " << contours.size() << endl;


}