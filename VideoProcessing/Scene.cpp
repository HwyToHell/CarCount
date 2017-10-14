#include "stdafx.h"
#include "../include/Tracking.h"

Scene::Scene() : confCreate(3), maxNoIDs(9), trafficFlowUnitVec(1,0)
{
	for (int i = maxNoIDs; i > 0; --i) // fill trackIDs with 9 ints
		trackIDs.push_back(i);
	distSubTrack = 30;	//--> maximum distance for grouping between bounding boxes of tracks
	minL2NormVelocity = 0.5;

	blobArea.min = 200; //320x240=100 640x480=500
	blobArea.max = 20000; //320x240=10000 640x480=60000
}

// update tracks from blobs and erase, if marked for deletion
// create new tracks from unassigned blobs
void Scene::UpdateTracks(std::list<TrackEntry>& blobs)
{
	/* debug_begin
	{
		int no = 1;
		std::list<Track>::iterator tr = tracks.begin();
		std::cout << "Tracks BEFORE update" << std::endl;
		while (tr != tracks.end())
		{
			std::cout << "track no " << no << ": " << tr->GetActualEntry().bbox << std::endl;
			++tr;
			++no;
		}

		no = 1;
		std::cout << "Blobs BEFORE update" << std::endl;
		std::list<TrackEntry>::iterator bl = blobs.begin();
		while (bl != blobs.end())
		{
			std::cout << "blob no " << no << ": " << bl->bbox << std::endl;
			++bl;
			++no;
		}
		std::cout << std::endl;
	}
	// debug_end */

	std::list<Track>::iterator tr = tracks.begin();
	while (tr != tracks.end())
	{
		tr->SetAssigned(false);
		tr->Update(blobs);
		// ToDo: remove pointers to MarkedForDeletion tracks in vehicles, before deleting tracks in scene 
		if (tr->IsMarkedForDelete())
		{
			ReturnTrackID(tr->GetId());
			tr = tracks.erase(tr);
		}
		else
			++tr;
	}
	
	// create new tracks for unassigned blobs
	trackIDs.sort(std::greater<int>());
	std::list<TrackEntry>::iterator bl = blobs.begin();
	while (bl != blobs.end())
	{
		int trackID = NextTrackID();
		if (trackID > 0)
			tracks.push_back(Track(*bl, trackID));
		++bl;
	}
	blobs.clear();
	
	/* debug_begin
	{
		int no = 1;
		std::list<Track>::iterator tr = tracks.begin();
		std::cout << "Tracks AFTER update" << std::endl;
		while (tr != tracks.end())
		{
			std::cout << "track no " << no << ": " << tr->GetActualEntry().bbox << std::endl;
			++tr;
			++no;
		}
		no = 1;
		std::cout << "Blobs AFTER update" << std::endl;
		std::list<TrackEntry>::iterator bl = blobs.begin();
		while (bl != blobs.end())
		{
			std::cout << "blob no " << no << ": " << bl->bbox << std::endl;
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
void Scene::UpdateTracksFromContours(const std::vector<std::vector<cv::Point>>& contours, std::vector<std::vector<cv::Point>>& movingContours)
{
	// clean up too small and too big contours
	std::list<TrackEntry> blobs;

	for (unsigned int i = 0; i < contours.size(); i++) // calc bounding boxes of new detection
	{
		//cv::drawContours(frame, contours, i, blue, CV_FILLED, 8, hierarchy, 0, cv::Point() );
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > blobArea.min) && (bbox.area() < blobArea.max))
		{
			blobs.push_back(TrackEntry(bbox, i));
		}
	}

	// ToDo: call update tracks and return indices of moving tracks for composing moving mask 
	UpdateTracks(blobs);
}


bool IsIdxZero(Track& track)
{
	if (track.GetIdxCombine() == 0)
		return true;
	else
		return false;
}

bool cmp_idx(Track& smaller, Track& larger)
{
	return (smaller.GetIdxCombine() < larger.GetIdxCombine());
}

bool sort_idx(Track& first, Track& second)
{
	return (first.GetIdxCombine() < second.GetIdxCombine());
}


void prnVehicle(Vehicle& vehicle)
{
	std::cout << "Vehicle - Rect: " << vehicle.GetBbox() << "; Velocity: " << vehicle.GetVelocity() << std::endl;
}



bool Scene::HaveSimilarVelocityVector(Track& track1, Track& track2)
{
	cv::Point2d vel1 = track1.GetVelocity();
	cv::Point2d vel2 = track2.GetVelocity();

	double vel1_DotTraffic = trafficFlowUnitVec.ddot(vel1);
	double cosPhiVel1 = vel1_DotTraffic / cv::norm(vel1) / cv::norm(trafficFlowUnitVec);

	double vel2_DotTraffic = trafficFlowUnitVec.ddot(vel2);
	double cosPhiVel2 = vel2_DotTraffic / cv::norm(vel2) / cv::norm(trafficFlowUnitVec);

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


// Check proximity of bbox of two tracks to compare	
bool Scene::AreClose(Track& track1, Track& track2)
{
	int dist_x = distSubTrack; // default: 30
	int dist_y = distSubTrack; // default: 30
	bool x_close, y_close;
	
	if (track1.GetActualEntry().bbox.x < track2.GetActualEntry().bbox.x)
		x_close = track2.GetActualEntry().bbox.x - (track1.GetActualEntry().bbox.x + track1.GetActualEntry().bbox.width) < dist_x ? true : false;
	else
		x_close = track1.GetActualEntry().bbox.x - (track2.GetActualEntry().bbox.x + track2.GetActualEntry().bbox.width) < dist_x ? true : false;

	if (track1.GetActualEntry().bbox.y < track2.GetActualEntry().bbox.y)
		y_close = track2.GetActualEntry().bbox.y - (track1.GetActualEntry().bbox.y + track1.GetActualEntry().bbox.height) < dist_y ? true : false;
	else
		y_close = track1.GetActualEntry().bbox.y - (track2.GetActualEntry().bbox.y + track2.GetActualEntry().bbox.height) < dist_y ? true : false;

	return (x_close & y_close);
}

void Scene::CombineTracks()
{
	vehicles.clear();
	
	std::list<Track*> pTracks; // list with pointers to tracks with high confidence
	std::vector<std::list<Track*>> vCombTracks; // vector with list of pointers to tracks
	std::list<Track>::iterator tr = tracks.begin();
	//std::for_each(tracks.begin(), tracks.end(), prnTrackId);

	// copy pointers to tracks 
	//  that move and
	//  have high confidence into new list
	while (tr != tracks.end())
	{
		int conf = tr->GetConfidence();
		bool isMoving = (norm(tr->GetVelocity()) > minL2NormVelocity);
		// ToDo: creating a fgMask from contours fails, if substitute values for blobs were calculated (conf < maxConf)
		// in this case track entry must not assigned to idxContour 
		if ((conf > 5) && isMoving)
			pTracks.push_back(tr->GetThis());
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
		cv::Point2d sumVelocity = (*pTr)->GetVelocity();
		cv::Rect sumBbox = (*pTr)->GetActualEntry().bbox;
		std::vector<int> contourIndices;
		contourIndices.push_back((*pTr)->GetActualEntry().idxContour);
		int nSum = 1;
		++pTr;
		while (pTr != vCombTracks[i].end())
		{
			sumBbox |= (*pTr)->GetActualEntry().bbox;
			sumVelocity += (*pTr)->GetVelocity();
			contourIndices.push_back((*pTr)->GetActualEntry().idxContour);
			++nSum;
			++pTr;
		} 
		// create vehicle
		cv::Point2d avgVelocity = sumVelocity * (1.0 / nSum);
		vehicles.push_back(Vehicle(sumBbox, avgVelocity, contourIndices));
	}

}

std::vector<int> Scene::getAllContourIndices()
{
	std::vector<int> allContourIndices;
	std::list<Vehicle>::iterator ve = vehicles.begin();
	while (ve != vehicles.end())
	{
		allContourIndices.insert(allContourIndices.end(), ve->contourIndices.begin(), ve->contourIndices.end());
		++ve;
	}
	return allContourIndices;
}

/*void Scene::UpdateVehicles()
{
	std::list<Vehicle>::iterator ve = vehicles.begin();
	while (ve != vehicles.end())
	{
		ve->Update(tracks);
		++ve;
	}
	
}
*/


int Scene::NextTrackID()
{
	if (trackIDs.empty()) return 0;
	else 
	{
		int id = trackIDs.back();
		trackIDs.pop_back();
		return id;
	}
}

bool Scene::ReturnTrackID(int id)
{
	if (id > 0 )
	{
		if (trackIDs.size() < maxNoIDs)
		{
			trackIDs.push_back(id);
			return true;
		}
		else
			return false;

	}
	else
		return false;
}



void Scene::ShowTracks(cv::Mat& frame)
{
	int line_slim = 1;
	int line_fat = 2;

	const cv::Scalar red = cv::Scalar(0,0,255);
	const cv::Scalar orange = cv::Scalar(0,128,255);
	const cv::Scalar yellow = cv::Scalar(0,255,255);
	const cv::Scalar ye_gr = cv::Scalar(0,255,180);
	const cv::Scalar green = cv::Scalar(0,255,0);

	std::list<Track>::iterator it = tracks.begin();
	while (it != tracks.end())
	{
		cv::rectangle(frame, it->GetActualEntry().bbox, green, line_slim);
		cv::Point textPos(it->GetActualEntry().bbox.x, it->GetActualEntry().bbox.y);
		// show avg velocity
		cv::Point textPos_x(it->GetActualEntry().centroid.x+10, it->GetActualEntry().centroid.y);
		cv::Point textPos_y(it->GetActualEntry().centroid.x, it->GetActualEntry().centroid.y+10);

		char buf[10];
		double x_vel = it->GetVelocity().x;
		std::sprintf(buf, "%2.1f", x_vel);
		std::string x_vel_str(buf);
		
		double y_vel = it->GetVelocity().y;
		std::sprintf(buf, "%2.1f", y_vel);
		std::string y_vel_str(buf);

		cv::putText(frame, x_vel_str, textPos_x, 0, 0.5, green, line_slim);
		cv::putText(frame, y_vel_str, textPos_y, 0, 0.5, green, line_slim);
		++it;
	}
}


void Scene::ShowVehicles(cv::Mat& frame)
{
	int line_slim = 1;
	int line_fat = 2;
	const cv::Scalar red = cv::Scalar(0,0,255);
	std::list<Vehicle>::iterator ve = vehicles.begin();
	while (ve != vehicles.end())
	{
		cv::rectangle(frame, ve->GetBbox(), red, line_fat);
		++ve;
	}

}

void Scene::PrintVehicles()
{
	for_each(vehicles.begin(), vehicles.end(), prnVehicle);
}

