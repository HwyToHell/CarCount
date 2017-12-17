#include <opencv2/opencv.hpp>
#include <iostream>


using namespace std;
using namespace cv;

void HitOrMiss(cv::Mat& src, cv::Mat& dst, cv::Mat& kernel); // defined in video.cpp


void _RemoveNoise(cv::Mat& src, cv::Mat& dst, cv::Mat& kernel)
{
	CV_Assert(src.type() == CV_8U && src.channels() ==1);
	cv::Mat kHit = (kernel == 1) / 255;
	cv::Mat kMiss = (kernel == -1) / 255;

	cout << "KHit: " << endl << kHit << endl << endl;
	cv::Mat eHit, eMiss;
	cv::erode(src, eHit, kHit, cv::Point(-1,-1), 1);
	cout << "dilate eHit: " << endl << eHit << endl << endl;
	dst = eHit;
}




int mat_main(int,char**)
{
	// performance measurement
	double tickStart = (double) getTickCount();
   
	Mat L;
	cout << "L empty: " << L.empty() << endl;
    cout << "L = " << endl << " " << L << endl << endl;	
	
	// create by using the constructor
    Mat M(4,4, CV_8UC1, 100);
	cout << "Rows: " << M.rows << endl;
	cout << "M empty: " << M.empty() << endl;
    cout << "M = " << endl << " " << M << endl << endl;


    // summarizing columns, returning vector
	Mat X(1, 4, CV_32SC1);
    cout << "X = "<< endl << " "  << X << endl << endl;
	reduce(M, X, 0, CV_REDUCE_SUM, CV_32SC1);
    cout << "M reduced to row = "<< endl << " "  << X << endl << endl;
	reduce(M, X, 1, CV_REDUCE_SUM, CV_32SC1);
    cout << "M reduced to col = "<< endl << " "  << X << endl << endl;

	Mat Y;
	Y = Mat::zeros(6, 6, CV_8UC1); // zeros = static function
	cout << "Size: " << Y.rows << endl;
	Y.at<uchar>(1,2) = 255;
	Y.at<uchar>(1,3) = 255;
	Y.at<uchar>(1,4) = 255;
	Y.at<uchar>(2,2) = 255; 	Y.at<uchar>(2,4) = 255; 
	Y.at<uchar>(3,2) = 255; 	
	Y.at<uchar>(4,2) = 255;		Y.at<uchar>(4,4) = 255; 

	cout << "Y=  " << endl << Y << endl << endl;

	// test for hit-and-miss
	//top-left angle
	Mat kTopLeft = (Mat_<int>(3,3) << -1,-1,0,-1,1,1,0,1,0);
	//top-right angle
	Mat kTopRight = (Mat_<int>(3,3) << 0,-1,-1,1,1,-1,0,1,0);
	//bottom-left angle
	Mat kBottomLeft = (Mat_<int>(3,3) << 0,1,0,-1,1,1,-1,-1,0);
	//bottom-right angle
	Mat kBottomRight = (Mat_<int>(3,3) << 0,1,0,1,1,-1,0,-1,-1);
	//find single bg wholes in fg (xor result with original image
	Mat kNoise = (Mat_<int>(3,3) << 0,-1,0,-1,1,-1,0,-1,0);

	cout << "k top left =  " << endl << kTopLeft << endl << endl;
	cout << "k top right =  " << endl << kTopRight << endl << endl;
	cout << "k bottom left =  " << endl << kBottomLeft << endl << endl;
	cout << "k bottom right = " << endl << kBottomRight << endl << endl;
	cout << "k noise =  " << endl << kNoise << endl << endl;

	// ToDo: lower angles, side contour 

	Mat Z, Z1, Z2;
	HitOrMiss(Y, Z1, kTopRight);
	HitOrMiss(Y, Z2, kTopLeft);
	Z = Z1 | Z2;
	cout << "Z= " << endl << Z << endl << endl;
	HitOrMiss(Y, Z, kNoise);
	cout << "Z= " << endl << Z << endl << endl;
	//RemoveNoise(Y, Z, kNoise);
	cout << "Y xor Z: " << endl << (Y^Z) << endl << endl; 


	// performance measurement
	double tickElapsed = (double) getTickCount() - tickStart;
	double tElapsed = tickElapsed / getTickFrequency();
	cout << "Tick frequency: " << getTickFrequency() << endl;
	cout << "Elapsed time: " << tElapsed * 1000 << " ms" << endl;


	cv::imshow("Mat", M);; // highgui window necessary for waitKey fcn
	cv::waitKey(0); // at least one highgui window has to be created for this function to work


    return 0;
}