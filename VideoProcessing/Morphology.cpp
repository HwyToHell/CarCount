#include "stdafx.h"
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