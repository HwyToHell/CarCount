#include "stdafx.h"
#include "KeyPress.h"
#include "../include/Parameters.h"
#include "vibe-background-sequential.h"
#include "FrameDiff.h"

using namespace std;

list<TrackEntry> FindShapes(const cv::Mat& mask, cv::Mat& frame);

void FindBlobs(const cv::Mat& mask, cv::Mat& frame);

void SegmentFrameVibe(vibeModel_Sequential_t* model, const cv::Mat& frame, cv::Mat& segmentationMap);
	


int _main(int argc, char* argv[])
{

	//cv::VideoCapture cap("D:/Holger/AppDev/OpenCV/BGS_old/Debug/dataset/traffic.wmv");
	cv::VideoCapture cap("D:/Holger/AppDev/OpenCV/VideoProcessing/DataSet/320x240_10fps/_Bus_320x240_10fps.avi");
	if (!cap.isOpened())
	{
		cout << "Cannot open video file" << endl;
		return -1;
	}

	cv::Mat frame, frame_old, fDiff, out_frame, fgMaskMOG2, fgMask_old, fgMaskAND, fgMask, fgMask_1,
		fBackgrnd, fBlur, fDilate, fErode, fBlurBig, fMorph, fMorph_1, fResize,
		maskMOG, MedBlur, bgrMOG, bgrVibe,
		maskDiff, bgrDiff;
	cv::Mat im_gray, frame_grey; 

	//cv::BackgroundSubtractorMOG mog = cv::BackgroundSubtractorMOG(100, 3, 0.8);
	cv::Mat krnl = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(-1,-1));

	//-------------------------------------------------------------------------
	// initialize MOG2 and load parameters
	//-------------------------------------------------------------------------
	string paramFile = "params.xml";
	MOG2 mog2;
	mog2.initPar();

	/*if (mog2.fromFile(paramFile))
		cout << "Parameters sucessfully loaded" << endl;
	else
		cout << "Not able to load parameters" << endl;
	*/
	
	mog2.updateAll();

	cv::Ptr<cv::BackgroundSubtractorMOG2> pMOG2_1;	
	pMOG2_1 = new cv::BackgroundSubtractorMOG2(100,15,0);

	
	//-------------------------------------------------------------------------
	// initialize Vibe and load parameters
	//-------------------------------------------------------------------------
	vibeModel_Sequential_t* vibeModel = (vibeModel_Sequential_t*)libvibeModel_Sequential_New();

	//-------------------------------------------------------------------------
	// initialize FrameDiff and load parameters
	//-------------------------------------------------------------------------
	FrameDiff bgsFrameDiff;
	
	int threshold = 20; // std 30
	/*cv::namedWindow("Threshold", cv::WINDOW_AUTOSIZE);
	cv::createTrackbar("Shadow", "Threshold", &threshold, 255);
	*/
	//cv::waitKey(0);

	int cnt = 0;

	list<TrackEntry> newBlobs; // stores new identified objects, that have not been assigned to tracks yet, for MOG2 algorithm
	list<TrackEntry> newBlobsVibe;
	list<TrackEntry> newBlobsFrameDiff;

	Scene sceneMOG; // collection of tracks and vehicles with MOG2
	Scene sceneVibe; 
	Scene sceneDiff; 

	cv::namedWindow("Origin");
	cv::namedWindow("MOG2+Dilate");
	//cv::namedWindow("FrameDiff");
	//cv::namedWindow("FrameDiff Background");
	//cv::namedWindow("FrameDiff Dilate");
	//cv::namedWindow("FrameDiff Org");
	//cv::namedWindow("OrgVibe");
	//cv::namedWindow("Vibe");
	//cv::namedWindow("Vibe+Dilate");

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

		// To Do: Smooting frame (downsize frame, temporal moving average or blending
		// To Do: Background Subtraction "Moving Mean" over 10 samples

		cv::GaussianBlur(frame, fBlurBig, cv::Size(9,9), 2);
		cv::Mat frameVibe = fBlurBig.clone();
		cv::Mat frameDiff = frame.clone();
		// show frame counter, int font = cnt % 8;
		cv::putText(frame, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(0, 0,255), 2);
		cv::imshow("Origin", frame);

		/*********************************************************************/
		/* Background Segmentation *******************************************/
		/*********************************************************************/
		// MOG 
		mog2.bs(fBlurBig, fgMaskMOG2); // std: 0.01
		mog2.bs.getBackgroundImage(fBackgrnd);
		
		/*// Vibe
		SegmentFrameVibe(vibeModel, frameVibe, bgrVibe); 

		// Frame Diff
		bgsFrameDiff.apply(frameDiff, maskDiff);*/

		/*********************************************************************/
		/* Post-Processing ***************************************************/
		/*********************************************************************/
		// MOG
		cv::threshold(fgMaskMOG2, fgMask, 0, 255, cv::THRESH_BINARY);	
		cv::medianBlur(fgMask, MedBlur, 5);
		//cv::imshow("MOG2", fgMask);
		cv::dilate(MedBlur, fDilate, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),1);
		cv::imshow("MOG2+Dilate", fDilate);
		cv::putText(fgMask, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(127, 127, 127), 2);
		newBlobs = FindShapes(fDilate, frame);	
		sceneMOG.UpdateTracks(newBlobs);
		sceneMOG.ShowTracks(frame);
		sceneMOG.CombineTracks();
		sceneMOG.ShowVehicles(frame);
		imshow("Detect", frame);

		/*// Vibe
		cv::imshow("OrgVibe", frameVibe);
		cv::medianBlur(bgrVibe, bgrVibe, 5);
		cv::imshow("Vibe", bgrVibe);
		cv::dilate(bgrVibe, bgrVibe, cv::Mat(11,11,CV_8UC1,1), cvPoint(-1,-1),1);
		cv::imshow("Vibe+Dilate", bgrVibe);
		newBlobsVibe = FindShapes(bgrVibe);
		sceneVibe.UpdateTracks(newBlobsVibe);
		sceneVibe.ShowTracks(frameVibe);
		sceneVibe.CombineTracks();
		sceneVibe.ShowVehicles(frameVibe);
		
		
		// FrameDiff
		cv::imshow("FrameDiff", maskDiff);
		cv::medianBlur(maskDiff, maskDiff, 5);
		//cv::erode(maskDiff, maskDiff, cv::Mat(3,3,CV_8UC1,1));
		//cv::dilate(maskDiff, maskDiff, cv::Mat(7,7,CV_8UC1,1));
		cv::imshow("FrameDiff Dilate", maskDiff);
		//newBlobsFrameDiff = FindShapes(maskDiff, frame);
		

		// update tracks from contours, return moving contours
		vector<vector<cv::Point> > contours, movingContours; vector<cv::Vec4i> hierarchy;
		cv::findContours(maskDiff, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
		sceneDiff.UpdateTracksFromContours(contours, movingContours);
		sceneDiff.ShowTracks(frame);
		sceneDiff.CombineTracks();
		bgsFrameDiff.updateBg(sceneDiff, contours, bgrDiff);
		cv::imshow("FrameDiff Background", bgrDiff); // bgrDiff can only be shown after update background
		sceneDiff.ShowVehicles(frame);
		cv::imshow("FrameDiff Org", frame);
		*/
		
		
		StopStartNextFrame();
		if (cv::waitKey(10) == 27)
		{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}

	libvibeModel_Sequential_Free(vibeModel);

	if (mog2.toFile(paramFile)) // save params for next test loop
		cout << "Parameters sucessfully saved" << endl;
	else
		cout << "Not able to save parameters" << endl;
	// save params to directory of frame samples
	stringstream stream;
//	stream << sampleDir << "\\" << paramFile;
	//mog2.toFile(stream.str());
	
	cv::waitKey(0);
	return 0;
}

void SegmentFrameVibe(vibeModel_Sequential_t* model, const cv::Mat& frame, cv::Mat& segmentationMap)
{
	static int frameNumber = 1;
	cv::Mat frameGray;
	
	// Segmentation with grayscale version 
	cv::cvtColor(frame, frameGray, CV_BGR2GRAY);
	//frameGray = frame;
	if (frameNumber == 1) 
	{
		segmentationMap = cv::Mat(frameGray.rows, frameGray.cols, CV_8UC1);
		libvibeModel_Sequential_AllocInit_8u_C1R(model, frameGray.data, frameGray.cols, frameGray.rows);
		//libvibeModel_Sequential_AllocInit_8u_C3R(model, frame.data, frame.cols, frame.rows);
		libvibeModel_Sequential_SetNumberOfSamples(model, 20); // std: 20
		libvibeModel_Sequential_SetMatchingThreshold(model, 16); // std: 20
		libvibeModel_Sequential_SetMatchingNumber(model, 2); // std: 2
		libvibeModel_Sequential_SetUpdateFactor(model, 4); // std: 16

		libvibeModel_Sequential_PrintParameters(model);
	}
	/* ViBe: Segmentation and updating. */
	/* Grayscale */
	libvibeModel_Sequential_Segmentation_8u_C1R(model, frameGray.data, segmentationMap.data);
	libvibeModel_Sequential_Update_8u_C1R(model, frameGray.data, segmentationMap.data);
	/* Color */
	//libvibeModel_Sequential_Segmentation_8u_C3R(model, frame.data, segmentationMap.data);
	//libvibeModel_Sequential_Update_8u_C3R(model, frame.data, segmentationMap.data);
	++frameNumber;
}


list<TrackEntry> FindShapes(const cv::Mat& mask, cv::Mat& frame)
{
	/// @todo
	/// implement ROI as parameter in scene
	const cv::Rect sceneROI(80, 80, 180, 80);

	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	list<TrackEntry> newShapes; // stores new identified objects, that have not been assigned to tracks yet
	const cv::Scalar blue = cv::Scalar(255,0,0);

	// draw ROI
	cv:rectangle(frame, sceneROI, blue);


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

		cv::Rect boxPos(it->bbox.x-2, it->bbox.y-2, it->bbox.width+4, it->bbox.height+4);
		cv::rectangle(frame, boxPos, blue, thickness);
		++it;
	}
*/
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