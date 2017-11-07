#include "stdafx.h"
#include "../include/Tracking.h"

// TODO delete vehicle representation
Vehicle::Vehicle(cv::Rect _bbox, cv::Point2d _velocity, std::vector<int> _contourIndices) 
	: bbox(_bbox), velocity(_velocity), contourIndices(_contourIndices), confAssign(3), confVisible(4)
{
	centroid.x = bbox.x + (bbox.width / 2);
	centroid.y = bbox.y + (bbox.height / 2);
}





cv::Rect Vehicle::GetBbox()
{
	return bbox;
}

cv::Point2i Vehicle::GetCentroid()
{
	return centroid;
}

cv::Point2d Vehicle::GetVelocity()
{
	return velocity;
}