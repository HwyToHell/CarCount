#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cmath>
#include <opencv2/opencv.hpp>
#include "KeyPress.h"
#include "Parameters.h"

using namespace std;


void HitOrMiss(cv::Mat& src, cv::Mat& dst, cv::Mat& kernel)
{
	CV_Assert(src.type() == CV_8U && src.channels() ==1);
	cv::Mat kHit = (kernel == 1) / 255;
	cv::Mat kMiss = (kernel == -1) / 255;
	/*cout << "Kernel: " << endl << kernel << endl << endl;
	cout << "KHit: " << endl << kHit << endl << endl;
	cout << "KMiss: " << endl << kMiss << endl << endl;*/
	cv::Mat eHit, eMiss;
	cv::erode(src, eHit, kHit, cv::Point(-1,-1), 1);
	cv::erode(255-src, eMiss, kMiss, cv::Point(-1,-1), 1);
	/*cout << "inv src: " << endl << (255-src) << endl << endl;
	cout << "eHit: " << endl << eHit << endl << endl;
	cout << "eMiss: " << endl << eMiss << endl << endl;*/
	dst = eHit & eMiss;
}

	// test for hit-and-miss
	//top-left angle
	cv::Mat kTopLeft = (cv::Mat_<int>(3,3) << -1,-1,0,-1,1,1,0,1,0);
	//top-right angle
	cv::Mat kTopRight = (cv::Mat_<int>(3,3) << 0,-1,-1,1,1,-1,0,1,0);
	//bottom-left angle
	cv::Mat kBottomLeft = (cv::Mat_<int>(3,3) << 0,1,0,-1,1,1,-1,-1,0);
	//bottom-right angle
	cv::Mat kBottomRight = (cv::Mat_<int>(3,3) << 0,1,0,1,1,-1,0,-1,-1);
	//straight-top
	cv::Mat kStraightTop = (cv::Mat_<int>(3,3) << -1,-1,-1,1,1,1,0,0,0);
	//straight-right
	cv::Mat kStraightRight = (cv::Mat_<int>(3,3) << 0,1,-1,0,1,-1,0,1,-1);
	//straight-bottom
	cv::Mat kStraightBottom = (cv::Mat_<int>(3,3) << 0,0,0,1,1,1,-1,-1,-1);
	//straight-left
	cv::Mat kStraightLeft = (cv::Mat_<int>(3,3) << -1,1,0,-1,1,0,-1,1,0);
	//find single bg wholes in fg (xor result with original image
	cv::Mat kNoise = (cv::Mat_<int>(3,3) << 0,-1,0,-1,1,-1,0,-1,0);

void RemoveNoise(cv::Mat& src, cv::Mat& dst)
{
	cv::Mat tmp;
	HitOrMiss(src, tmp, kNoise);
	dst = src ^ tmp;
}

void Edges(cv::Mat& src, cv::Mat& dst)
{
	cv::Mat topLeft, topRight, bottomLeft, bottomRight;
	HitOrMiss(src, topLeft, kTopLeft);
	HitOrMiss(src, topRight, kTopRight);
	HitOrMiss(src, bottomLeft, kBottomLeft);
	HitOrMiss(src, bottomRight, kBottomRight);
	dst = topLeft | topRight | bottomLeft | bottomRight;
}

void StraightBorder(cv::Mat& src, cv::Mat& dst)
{
	cv::Mat straightTop, straightRight, straightBottom, straightLeft;
	HitOrMiss(src, straightTop, kStraightTop);
	HitOrMiss(src, straightRight, kStraightRight);
	HitOrMiss(src, straightBottom, kStraightBottom);
	HitOrMiss(src, straightLeft, kStraightLeft);
	dst = straightTop | straightRight | straightBottom | straightLeft;
}

void Outline(cv::Mat& src, cv::Mat& dst)
{
	cv::Mat edges;
	Edges(src, edges);
	cv::Mat straight;
	StraightBorder(src, straight);
	dst = edges | straight;
}

void refineSegments(const cv::Mat& img, cv::Mat& mask, cv::Mat& dst)
{
    int niters = 3;

    vector<vector<cv::Point> > contours;
    vector<cv::Vec4i> hierarchy;

    cv::Mat temp;

    cv::dilate(mask, temp, cv::Mat(), cv::Point(-1,-1), niters);
    cv::erode(temp, temp, cv::Mat(), cv::Point(-1,-1), niters*2);
    cv::dilate(temp, temp, cv::Mat(), cv::Point(-1,-1), niters);
	//imshow("temp", temp);

    findContours( temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );

    dst = cv::Mat::zeros(img.size(), CV_8UC3);

    if( contours.size() == 0 )
        return;

    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    int idx = 0, largestComp = 0;
    double maxArea = 0;

    for( ; idx >= 0; idx = hierarchy[idx][0] )
    {
        const vector<cv::Point>& c = contours[idx];
        double area = fabs(cv::contourArea(cv::Mat(c)));
        if( area > maxArea )
        {
            maxArea = area;
            largestComp = idx;
        }
    }
    cv::Scalar color( 0, 0, 255 );
    cv::drawContours( dst, contours, largestComp, color, CV_FILLED, 8, hierarchy );

}

struct DistToTrack {
	int idxTrack;
	double distance;
};

struct TrackSample {
	cv::Rect bbox;
	cv::Point2i centroid;
	vector<DistToTrack> distToTracks;
	double velocity;
};


struct LocalTrack {
	vector<TrackSample> trHistory; // dimension: time
	double confidence;
	bool visible;
};

double EuclideanDist(cv::Point& pt1, cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}

void DrawBoxes(cv::Mat& frame, cv::Mat& mask, deque<LocalTrack>& tracks)
{
	const cv::Scalar green = cv::Scalar(0,255,0);
	const cv::Scalar red = cv::Scalar(0,0,255);
	const int thickness = 1;
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	vector<TrackSample> detectedObj; // stores new identified objects, that have not been assigned to tracks yet

	cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	for (unsigned int i = 0; i < contours.size(); i++) // calc bounding boxes of new detection
	{
		cv::drawContours(frame, contours, i, red, 2, 8, hierarchy, 0, cv::Point() );
		cv::Rect bbox = boundingRect(contours[i]);
		if (bbox.area() > 100)
		{
			TrackSample newObj;
			newObj.bbox = bbox;
			newObj.centroid = cv::Point(bbox.x + bbox.width/2, bbox.y + bbox.height/2);
			newObj.velocity = 0;
			detectedObj.push_back(newObj);
		}
	}
	for (unsigned int i = 0; i < detectedObj.size(); i++) // show shapes
	{
		cv::Point textPos(detectedObj[i].bbox.x, detectedObj[i].bbox.y);
		cv::rectangle(frame, detectedObj[i].bbox, green, thickness);
		cv::putText(frame, to_string((long long) detectedObj[i].bbox.area()), textPos, 0, 0.5, green, thickness);
	}
	// calculate distances to associate tracks
	double distThreshold = 100;
	for (unsigned int i = 0; i < detectedObj.size(); i++) // calculate distance from each detected Object to existing tracks
	{
//		if (tracks.size() > 0)
//		{
			for (unsigned int n = 0; n < tracks.size(); n++)  
			{
				DistToTrack dstToTrack;
				dstToTrack.distance	= EuclideanDist(detectedObj[i].centroid, tracks[n].trHistory.back().centroid);
				dstToTrack.idxTrack = n;
				if (dstToTrack.distance < distThreshold)
					detectedObj[i].distToTracks.push_back(dstToTrack);
			}
	}
	for (unsigned int i = 0; i < detectedObj.size(); i++) 
	{
		for (unsigned int n = 0; n < detectedObj[i].distToTracks.size(); n++)  // find minimum distance
		{

		}



	}
}


int main(int argc, char* argv[])
{

	cv::VideoCapture cap("D:/Holger/AppDev/OpenCV/BGS/Debug/dataset/traffic.wmv");
	if (!cap.isOpened())
	{
		cout << "Cannot open video file" << endl;
		return -1;
	}

	cv::Mat frame, frame_old, fDiff, out_frame, fgMaskMOG2, fgMaskMOG2_1, fgMask, fgMask_1,
		fBackgrnd, fBlur, fDilate, fErode, fBlurBig, fMorph, fMorph_1, fResize,
		maskMOG, MedBlur, bgrMOG;
	cv::Mat im_gray, frame_grey; 

	//cv::BackgroundSubtractorMOG mog = cv::BackgroundSubtractorMOG(100, 3, 0.8);
	cv::Mat krnl = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(-1,-1));
	// initialize MOG2 and load parameters
	string paramFile = "params.xml";
	MOG2 mog2;
	mog2.initPar();

	/*if (mog2.fromFile(paramFile))
		cout << "Parameters sucessfully loaded" << endl;
	else
		cout << "Not able to load parameters" << endl;
	*/
	
	mog2.updateAll();
	/*mog2.createTrackbars();
	
	SaveSamples ss(&fDilate, &fgMaskMOG2);
	string sampleDir = ss.CreateDir();
	cout << "Saving frames to: " << sampleDir << endl;
	*/
	cv::Ptr<cv::BackgroundSubtractorMOG2> pMOG2_1;	
	pMOG2_1 = new cv::BackgroundSubtractorMOG2(100,15,0);

	// threshold ( original - background ) 
	int threshold = 20; // std 30
	/*cv::namedWindow("Threshold", cv::WINDOW_AUTOSIZE);
	cv::createTrackbar("Shadow", "Threshold", &threshold, 255);
	*/
	//cv::waitKey(0);

	int cnt = 0;

	deque<LocalTrack> locTracks;

	while(!KeyPressed(27))
	{
		double tickStart = (double) cv::getTickCount();

		bool isSuccess = cap.read(frame);
		if (!isSuccess)
		{
			cout << "Cannot read frame from video file" << endl;
			break;
		}
		cnt++;
		//cout << "Loop count: " << cnt << endl << endl;
		//StopStartEnter();
/*	
		//mog.operator()(frame, maskMOG, 0.01);
		cv::cvtColor(frame, frame_grey, CV_RGB2GRAY);
				cv::imshow("greyOrg", frame_grey);
		mog(frame_grey, maskMOG);
		cv::imshow("MOG", maskMOG);
*/
				//cv::blur(frame, fBlurBig, cv::Size(7,7));
		cv::GaussianBlur(frame, fBlurBig, cv::Size(7,7), 2);
		mog2.bs(fBlurBig, fgMaskMOG2); // std: 0.01
		cv::threshold(fgMaskMOG2, fgMask, 0, 255, cv::THRESH_BINARY);	
		cv::medianBlur(fgMask, MedBlur, 5);
			//cv::imshow("MedBlur", MedBlur);
			
		cv::dilate(MedBlur, fDilate, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),1);
				cv::imshow("MedBlur+Dilate", fDilate);
	/*/ find blobs
    std::vector<std::vector<cv::Point> > v;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours( mask, v, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    mask = cv::Scalar(0, 0, 0);
    for ( size_t i=0; i < v.size(); ++i )
    {
        // drop smaller blobs
        if (cv::contourArea(v[i]) < 20)
            continue;
        // draw filled blob
        cv::drawContours(mask, v, i, cv::Scalar(255,0,0), CV_FILLED, 8, hierarchy, 0, cv::Point() ); 
    }
	*/

		// dilate original frame
			//cv::dilate(frame, fMorph, cv::Mat(), cv::Point(-1,-1),1);
			//cv::imshow("Open", fMorph);
		/*
		// blur + resize original frame
		cv::resize(frame, fResize, cv::Size(frame.size().width/2, frame.size().height/2));
		cv::blur(fResize, fBlur, cv::Size(4,4));
		cv::imshow("Resize-Blur", fBlur);
		pMOG2_1->operator()(fBlur, fgMaskMOG2_1, 0.005);
		cv::threshold(fgMaskMOG2_1, fgMask_1, 0, 255, cv::THRESH_BINARY);

		cv::morphologyEx(fgMask, fMorph, cv::MORPH_ERODE, krnl);		
		cv::morphologyEx(fgMask_1, fMorph_1, cv::MORPH_ERODE, krnl);
		cv::dilate(fMorph_1, fDilate, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),1);

		cv::imshow("morph1", fMorph_1);
		*/
		

		mog2.bs.getBackgroundImage(fBackgrnd);
	//			cv::imshow("bgrMOG2", fBackgrnd);

		cv::absdiff(fBlurBig, fBackgrnd, fDiff);
		cv::cvtColor(fDiff, im_gray, CV_RGB2GRAY);
		cv::threshold(im_gray, fDiff, (double)threshold, 255, cv::THRESH_BINARY);
		//cv::imshow("Threshold", fDiff);

		//cv::imshow("Diff Gray", im_gray);
		//double dThreshold = threshold;
		

		

		// Morph mask output
			//cv::dilate(fgMask, fDilate, cv::Mat(3,3,CV_8UC1,1), cvPoint(-1,-1),1);
			//RemoveNoise(fgMask, fMorph);
		
		double tickElapsed = (double) cv::getTickCount() - tickStart;
		double tElapsed = tickElapsed / cv::getTickFrequency();
		//cout << "Tick frequency: " << cv::getTickFrequency() << endl;
		//cout << "Elapsed time: " << tElapsed * 1000 << " ms" << endl;
/*
		//cv::imshow("Dilate 3x3", fDilate);
		//cv::imshow("Noise", fMorph);
		cv::dilate(fMorph, fDilate, cv::Mat(3,3,CV_8UC1,1), cvPoint(-1,-1),1);
		//cv::imshow("Noise + Dilate 3x3", fDilate);
		Edges(fDilate, fEdges);
		//cv::imshow("Edges", fEdges);
		Outline(fDilate, fOutline);
		//cv::imshow("Outline", fOutline);
		cv::dilate(fDilate, fDilate, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),1);
		//cv::imshow("Dilate 5x5", fDilate);

		cv::dilate(fgMask, fDilate, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),1);
		//cv::imshow("Dilate 5x5", fDilate);
		cv::erode(fDilate, fDilate, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),1);
		//cv::imshow("Dilate 5x5 + Erode 7x7", fDilate);

		cv::dilate(fgMask, fDilate, cv::Mat(3,3,CV_8UC1,1), cvPoint(-1,-1),2);
		cv::erode(fDilate, fDilate, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),1);
		cv::dilate(fDilate, fDilate, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),2);
		cv::imshow("Dilate + Erode", fDilate);
		
	
		cv::erode(fgMask, fErode, cv::Mat(3,3,CV_8UC1,1), cvPoint(-1,-1),1);
		cv::dilate(fErode, fErode, cv::Mat(5,5,CV_8UC1,1), cvPoint(-1,-1),2);
		cv::imshow("Erode + Dilate", fErode);

		//cv::morphologyEx( fgMask, fMorph, cv::MORPH_CLOSE, cv::Mat() );
		//cv::imshow("Open", fMorph);
*/


		//int font = cnt % 8;
		cv::putText(frame, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(0, 0,255), 2);
		cv::putText(fgMask, to_string((long long)cnt), cv::Point(10,35), 0, 1.0, cv::Scalar(127, 127, 127), 2);

		DrawBoxes(frame, fDilate, locTracks);
		//cv::imshow("Test",  fTest);
		cv::imshow("Origin", frame);
		cv::imshow("MOG2_Org", fgMask);
//		cv::imshow("Blur_MOG2", fgMask_1);
		//cv::imshow("w/ shadow", fgMask);

		// ToDo: write image samples
	/*	if (ss.CheckForSave())
			cout << "Frame " << ss.count << endl;
	*/		
		//WriteFrames(frame, perfDir, cnt);
		//cv::imshow("Background", fBackgrnd);



		if (cv::waitKey(10) == 27)
		{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}
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

