#include "stdafx.h"
#include "../include/Tracking.h"

// TODO delete vehicle representation
Vehicle::Vehicle(cv::Rect _bbox, cv::Point2d _velocity, std::vector<int> _contourIndices) 
	: mBbox(_bbox), mVelocity(_velocity), mContourIndices(_contourIndices), mConfAssign(3), mConfVisible(4)
{
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}





cv::Rect Vehicle::getBbox()
{
	return mBbox;
}

cv::Point2i Vehicle::getCentroid()
{
	return mCentroid;
}

cv::Point2d Vehicle::getVelocity()
{
	return mVelocity;
}

void Vehicle::update() {}