#include "stdafx.h"
#include "../include/config.h"
#include "../include/recorder.h"

using namespace std;

// TODO delete vehicle representation
// vehicle representation
class Vehicle {
private:
	const int mConfAssign;	// confidence above this level assigns unassigned trac to vehicle
	const int mConfVisible;	// confidence above this level makes vehicle visible 
	cv::Rect mBbox;
	cv::Point2i mCentroid;
	cv::Point2d mVelocity;
	int mId;
public:
	Vehicle(cv::Rect _bbox, cv::Point2d _velocity, std::vector<int> _contourIndices);
	cv::Rect getBbox();
	cv::Point2i getCentroid();
	cv::Point2d getVelocity();
	void update();
//private:
	std::vector<int> mContourIndices;
};


/// MOVE
list<Vehicle>& SceneTracker::combineTracks()
{
	mVehicles.clear();
	
	std::list<Track*> pTracks; // list with pointers to tracks with high confidence
	std::vector<std::list<Track*>> vCombTracks; // vector with list of pointers to tracks
	std::list<Track>::iterator tr = mTracks.begin();
	//std::for_each(mTracks.begin(), mTracks.end(), prnTrackId);

	// copy pointers to tracks 
	//  that move and
	//  have high confidence into new list
	while (tr != mTracks.end())
	{
		int conf = tr->getConfidence();
		bool isMoving = (norm(tr->getVelocity()) > mMinVelocityL2Norm);
		// ToDo: creating a fgMask from contours fails, if substitute values for blobs were calculated (conf < maxConf)
		// in this case track entry must not assigned to idxContour 
		if ((conf > mConfCreate) && isMoving)
			pTracks.push_back(tr->getThis());
		++tr;
	}

	// create vector of lists with pointers to tracks that need to be combined
	int idx = 0;
	std::list<Track*>::iterator pTr = pTracks.begin();
	while (pTr != pTracks.end())
	{
		std::list<Track*> tempList;
		// create new row with empty list
		vCombTracks.push_back(tempList);
		// move actual track entry to end of list
		vCombTracks[idx].push_back(*pTr);
		pTr = pTracks.erase(pTr);
		while (pTr != pTracks.end())
		{
			bool hasSimilarVelocity = HaveSimilarVelocityVector(*vCombTracks[idx].back(), **pTr);
			bool isClose = AreClose(*vCombTracks[idx].back(), (**pTr));
			/// TODO test alternative
			double direction = 0.1;
			double norm = 0.5;
			hasSimilarVelocity = (**pTr).hasSimilarVelocityVector(*vCombTracks[idx].back(), direction, norm);
			isClose = (**pTr).isClose(*vCombTracks[idx].back(), mDistSubTrack);
			/// end test alternative


			if (hasSimilarVelocity && isClose)
			{
				vCombTracks[idx].push_back(*pTr);
				pTr = pTracks.erase(pTr);
			}
			else
				++pTr;
		}
		++idx;
		pTr = pTracks.begin();
	}

	// combine bounding boxes, average velocity and create vehicle
	//  for each vectorelement
	//  combine list entries
	//  build vector of contour indices
	//  and create vehicle
	for (unsigned int i = 0; i < vCombTracks.size(); ++i)
	{
		pTr = vCombTracks[i].begin();
		cv::Point2d sumVelocity = (*pTr)->getVelocity();
		cv::Rect sumBbox = (*pTr)->getActualEntry().mBbox;
		std::vector<int> contourIndices;
		contourIndices.push_back((*pTr)->getActualEntry().mIdxContour);
		int nSum = 1;
		++pTr;
		while (pTr != vCombTracks[i].end())
		{
			sumBbox |= (*pTr)->getActualEntry().mBbox;
			sumVelocity += (*pTr)->getVelocity();
			contourIndices.push_back((*pTr)->getActualEntry().mIdxContour);
			++nSum;
			++pTr;
		} 
		// create vehicle
		cv::Point2d avgVelocity = sumVelocity * (1.0 / nSum);
		mVehicles.push_back(Vehicle(sumBbox, avgVelocity, contourIndices));
	}
	return mVehicles;
}

/* TODO delete, if not needed std::vector<int> SceneTracker::getAllContourIndices()
{
	std::vector<int> allContourIndices;
	std::list<Vehicle>::iterator ve = mVehicles.begin();
	while (ve != mVehicles.end())
	{
		allContourIndices.insert(allContourIndices.end(), ve->contourIndices.begin(), ve->contourIndices.end());
		++ve;
	}
	return allContourIndices;
}
*/

bool IsIdxZero(Track& track)
{
	if (track.getIdxCombine() == 0)
		return true;
	else
		return false;
}

bool cmp_idx(Track& smaller, Track& larger)
{
	return (smaller.getIdxCombine() < larger.getIdxCombine());
}

bool sort_idx(Track& first, Track& second)
{
	return (first.getIdxCombine() < second.getIdxCombine());
}




list<Vehicle>& SceneTracker::getVehicles() {
	return mVehicles;
}



void SceneTracker::showVehicles(cv::Mat& frame)
{
	int line_slim = 1;
	int line_fat = 2;
	const cv::Scalar red = cv::Scalar(0,0,255);
	std::list<Vehicle>::iterator ve = mVehicles.begin();
	while (ve != mVehicles.end())
	{
		cv::rectangle(frame, ve->getBbox(), red, line_fat);
		++ve;
	}

}

void SceneTracker::printVehicles()
{
	for_each(mVehicles.begin(), mVehicles.end(), prnVehicle);
}


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