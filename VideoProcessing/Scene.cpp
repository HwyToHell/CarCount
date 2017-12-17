#include "stdafx.h"
#include "../include/config.h"
#include "../include/tracking.h"

Scene::Scene(Config* pConfig) : mConfig(pConfig)
{
	pullConfig();
}

void Scene::pullConfig()
{
	mBlobArea.min = (int)mConfig->getDouble("blob_area_min");
	mBlobArea.max = (int)mConfig->getDouble("blob_area_max");
	mConfCreate = (int)mConfig->getDouble("group_min_confidence");
	mDistSubTrack = (int)mConfig->getDouble("group_distance");
	mMaxNoIDs = (int)mConfig->getDouble("max_n_of_tracks");
	mMinVelocityL2Norm = mConfig->getDouble("group_min_velocity");
	mTrafficFlow.x = mConfig->getDouble("traffic_flow_x");
	mTrafficFlow.y = mConfig->getDouble("traffic_flow_y");
	// mBlobArea.min(200)		-> smaller blobs will be discarded 320x240=100   640x480=500
	// mBlobArea.max(20000)		-> larger blobs will be discarded  320x240=10000 640x480=60000
	// mConfCreate(3)			-> confidence above this level creates vehicles from unassigned tracks
	// mDistSubTrack(30)		-> max distance for grouping between bounding boxes of tracks
	// mMaxNoIDs(9)				-> max number of tracks
	// mMinVelocityL2Norm(0.5)	-> slower tracks won't be grouped
	// mTrafficFlow.x(1)		-> horizontal component of traffic flow (normalized)
	// mTrafficFlow.y(0)		-> vertical component of traffic flow (normalized)
	for (int i = mMaxNoIDs; i > 0; --i) // fill trackIDs with 9 ints
	mTrackIDs.push_back(i);
}

// update tracks from blobs and erase, if marked for deletion
// create new tracks from unassigned blobs
void Scene::updateTracks(std::list<TrackEntry>& blobs)
{
	/* debug_begin
	{
		int no = 1;
		std::list<Track>::iterator tr = mTracks.begin();
		std::cout << "Tracks BEFORE update" << std::endl;
		while (tr != mTracks.end())
		{
			std::cout << "track no " << no << ": " << tr->GetActualEntry().mBbox << std::endl;
			++tr;
			++no;
		}

		no = 1;
		std::cout << "Blobs BEFORE update" << std::endl;
		std::list<TrackEntry>::iterator bl = blobs.begin();
		while (bl != blobs.end())
		{
			std::cout << "blob no " << no << ": " << bl->mBbox << std::endl;
			++bl;
			++no;
		}
		std::cout << std::endl;
	}
	// debug_end */

	std::list<Track>::iterator tr = mTracks.begin();
	while (tr != mTracks.end())
	{
		tr->setAssigned(false);
		tr->update(blobs);
		// ToDo: remove pointers to MarkedForDeletion tracks in vehicles, before deleting tracks in scene 
		if (tr->isMarkedForDelete())
		{
			returnTrackID(tr->getId());
			tr = mTracks.erase(tr);
		}
		else
			++tr;
	}
	
	// create new tracks for unassigned blobs
	mTrackIDs.sort(std::greater<int>());
	std::list<TrackEntry>::iterator bl = blobs.begin();
	while (bl != blobs.end())
	{
		int trackID = nextTrackID();
		if (trackID > 0)
			mTracks.push_back(Track(*bl, trackID));
		++bl;
	}
	blobs.clear();
	
	/* debug_begin
	{
		int no = 1;
		std::list<Track>::iterator tr = mTracks.begin();
		std::cout << "Tracks AFTER update" << std::endl;
		while (tr != mTracks.end())
		{
			std::cout << "track no " << no << ": " << tr->GetActualEntry().mBbox << std::endl;
			++tr;
			++no;
		}
		no = 1;
		std::cout << "Blobs AFTER update" << std::endl;
		std::list<TrackEntry>::iterator bl = blobs.begin();
		while (bl != blobs.end())
		{
			std::cout << "blob no " << no << ": " << bl->mBbox << std::endl;
			++bl;
			++no;
		}
		std::cout << std::endl;
	}
	// debug_end */

}

/// update tracks from contours of blobs
/// convert vector of contours to list of bboxes for further processing
/// erase bbox, if marked for deletion
/// create new tracks from unassigned blobs
void Scene::updateTracksFromContours(const std::vector<std::vector<cv::Point>>& contours, std::vector<std::vector<cv::Point>>& movingContours)
{
	// clean up too small and too big contours
	std::list<TrackEntry> blobs;

	for (unsigned int i = 0; i < contours.size(); i++) // calc bounding boxes of new detection
	{
		//cv::drawContours(frame, contours, i, blue, CV_FILLED, 8, hierarchy, 0, cv::Point() );
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > mBlobArea.min) && (bbox.area() < mBlobArea.max))
		{
			blobs.push_back(TrackEntry(bbox, i));
		}
	}

	// ToDo: call update tracks and return indices of moving tracks for composing moving mask 
	updateTracks(blobs);
}


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


void prnVehicle(Vehicle& vehicle)
{
	std::cout << "Vehicle - Rect: " << vehicle.getBbox() << "; Velocity: " << vehicle.getVelocity() << std::endl;
}



bool Scene::HaveSimilarVelocityVector(Track& track1, Track& track2)
{
	cv::Point2d vel1 = track1.getVelocity();
	cv::Point2d vel2 = track2.getVelocity();

	double vel1_DotTraffic = mTrafficFlow.ddot(vel1);
	double cosPhiVel1 = vel1_DotTraffic / cv::norm(vel1) / cv::norm(mTrafficFlow);

	double vel2_DotTraffic = mTrafficFlow.ddot(vel2);
	double cosPhiVel2 = vel2_DotTraffic / cv::norm(vel2) / cv::norm(mTrafficFlow);

	if (signBit(vel1_DotTraffic) != signBit(vel2_DotTraffic))
		return false; // opposite direction

	if ((std::abs(cosPhiVel1) < 0.9) || (std::abs(cosPhiVel2) < 0.9))
		return false; // direction deviation too large
	
	if (abs(vel2_DotTraffic) > abs(vel1_DotTraffic * 0.5) && abs(vel2_DotTraffic) < abs(vel1_DotTraffic * 2) )
		return true; // vel1 * 0.5 < vel2 < vel1 * 2 ---> max velocity difference 50%
	else
		return false;
/*
	double sumVelDotTraffic = abs(vel1_DotTraffic) + abs(vel2_DotTraffic);
	double diffVelDotTraffic = abs( abs(vel1_DotTraffic) - abs(vel2_DotTraffic) );
	
	if ((diffVelDotTraffic/sumVelDotTraffic) < 0.3334)
		return true;
	else return false;
	*/
}

/* TODO Delete after testing of Track::isClose() */
// Check proximity of bbox of two tracks to compare	
bool Scene::AreClose(Track& track1, Track& track2)
{
	int dist_x = mDistSubTrack; // default: 30
	int dist_y = mDistSubTrack; // default: 30
	bool x_close, y_close;
	
	if (track1.getActualEntry().mBbox.x < track2.getActualEntry().mBbox.x)
		x_close = track2.getActualEntry().mBbox.x - (track1.getActualEntry().mBbox.x + track1.getActualEntry().mBbox.width) < dist_x ? true : false;
	else
		x_close = track1.getActualEntry().mBbox.x - (track2.getActualEntry().mBbox.x + track2.getActualEntry().mBbox.width) < dist_x ? true : false;

	if (track1.getActualEntry().mBbox.y < track2.getActualEntry().mBbox.y)
		y_close = track2.getActualEntry().mBbox.y - (track1.getActualEntry().mBbox.y + track1.getActualEntry().mBbox.height) < dist_y ? true : false;
	else
		y_close = track1.getActualEntry().mBbox.y - (track2.getActualEntry().mBbox.y + track2.getActualEntry().mBbox.height) < dist_y ? true : false;

	return (x_close & y_close);
}

void Scene::combineTracks()
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
		if ((conf > 5) && isMoving)
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

}

/* TODO delete, if not needed std::vector<int> Scene::getAllContourIndices()
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
/* assign vehicles 

/*void Scene::updateVehicles()
{
	std::list<Vehicle>::iterator ve = mVehicles.begin();
	while (ve != mVehicles.end())
	{
		ve->update(mTracks);
		++ve;
	}
	
}
*/


int Scene::nextTrackID()
{
	if (mTrackIDs.empty()) return 0;
	else 
	{
		int id = mTrackIDs.back();
		mTrackIDs.pop_back();
		return id;
	}
}

bool Scene::returnTrackID(int id)
{
	if (id > 0 )
	{
		if (mTrackIDs.size() < mMaxNoIDs)
		{
			mTrackIDs.push_back(id);
			return true;
		}
		else
			return false;

	}
	else
		return false;
}



void Scene::showTracks(cv::Mat& frame)
{
	int line_slim = 1;
	int line_fat = 2;

	const cv::Scalar red = cv::Scalar(0,0,255);
	const cv::Scalar orange = cv::Scalar(0,128,255);
	const cv::Scalar yellow = cv::Scalar(0,255,255);
	const cv::Scalar ye_gr = cv::Scalar(0,255,180);
	const cv::Scalar green = cv::Scalar(0,255,0);

	std::list<Track>::iterator it = mTracks.begin();
	while (it != mTracks.end())
	{
		cv::rectangle(frame, it->getActualEntry().mBbox, green, line_slim);
		cv::Point textPos(it->getActualEntry().mBbox.x, it->getActualEntry().mBbox.y);
		// show avg velocity
		cv::Point textPos_x(it->getActualEntry().mCentroid.x+10, it->getActualEntry().mCentroid.y);
		cv::Point textPos_y(it->getActualEntry().mCentroid.x, it->getActualEntry().mCentroid.y+10);

		char buf[10];
		double x_vel = it->getVelocity().x;
		std::sprintf(buf, "%2.1f", x_vel);
		std::string x_vel_str(buf);
		
		double y_vel = it->getVelocity().y;
		std::sprintf(buf, "%2.1f", y_vel);
		std::string y_vel_str(buf);

		cv::putText(frame, x_vel_str, textPos_x, 0, 0.5, green, line_slim);
		cv::putText(frame, y_vel_str, textPos_y, 0, 0.5, green, line_slim);
		++it;
	}
}


void Scene::showVehicles(cv::Mat& frame)
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

void Scene::printVehicles()
{
	for_each(mVehicles.begin(), mVehicles.end(), prnVehicle);
}

