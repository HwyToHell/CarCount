#include "stdafx.h"
#include "../include/Tracking.h"
#include "../include/Parameters.h"


double EuclideanDist(cv::Point& pt1, cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}


// detected blobs
TrackEntry::TrackEntry(int x, int y, int width, int height, int idx) : idxContour(idx), velocity(0,0)
{
	bbox.x = abs(x);
	bbox.y = abs(y);
	bbox.width = abs(width);
	bbox.height = abs(height);
	centroid.x = bbox.x + (bbox.width / 2);
	centroid.y = bbox.y + (bbox.height / 2);
}

TrackEntry::TrackEntry(cv::Rect _bbox, int _idxContour) : bbox(_bbox), idxContour(_idxContour), velocity(0,0)
{
	centroid.x = bbox.x + (bbox.width / 2);
	centroid.y = bbox.y + (bbox.height / 2);
}


// track vector
Track::Track(TrackEntry& blob, int id) : maxDist(30), maxDeviation(80), maxConfidence(6), 
	id(id), confidence(0), idxCombine(0), markedForDelete(false), avgVelocity(0,0)
{
	// const criteria for blob assignment
	// maxDist = 30 --> maximum of 30 pixel distance between centroids
	// maxDeviation = 80 --> maximum 80% deviation in length and height
	// maxConfidence = 6 --> maximum number of consecutive frames allowed for substitute value calculation
	history.push_back(blob);
}

Track& Track::operator= (const Track& source)
{
	history = source.history;
	return *this;
}

Track* Track::GetThis()
{
	return this;
}

void Track::AddTrackEntry(const TrackEntry& blob)
{
	history.push_back(blob);
	int lengthHistory = history.size();
	if (lengthHistory > 1) // update velocity from n-1 positions
	{
		history[lengthHistory-1].velocity = history[lengthHistory-1].centroid - history[lengthHistory-2].centroid;
	}
	UpdateAvgVelocity();
	return;
}

void Track::AddSubstitute()
{
	// take average velocity and compose bbox from centroid of previous element
	cv::Point2i velocity((int)round(avgVelocity.x), (int)round(avgVelocity.y));
	TrackEntry substitute = history.back();
	substitute.centroid += velocity;
	substitute.bbox.x = substitute.centroid.x - substitute.bbox.width / 2;
	substitute.bbox.x < 0 ? 0 : substitute.bbox.x;
	substitute.bbox.y = substitute.centroid.y - substitute.bbox.height / 2;
	substitute.bbox.y < 0 ? 0 : substitute.bbox.y;
	// ToDo: check upper bounds of video window
	AddTrackEntry(substitute);
	return;
}

// iterate through detected blobs, check if size is similar, distance close, save closest blob to history and delete from blob input list 
void Track::Update(std::list<TrackEntry>& blobs)
{
	double minDist = maxDist;
	std::list<TrackEntry>::iterator ns = blobs.begin(), nsMinDist = blobs.end();
	while (ns != blobs.end()) 	
	{
		if (HasSimilarSize(*ns))
		{
			ns->distance = EuclideanDist(ns->centroid, GetActualEntry().centroid);
			if (ns->distance < minDist) 
			{
				minDist = ns->distance;
				nsMinDist = ns;
			}
		}
		++ns;
	}// end iterate through all input blobs

	// assign closest shape to track and delete from list of blobs
	if (nsMinDist != blobs.end()) // shape to assign available
	{
		AddTrackEntry(*nsMinDist);
		ns = blobs.erase(nsMinDist);
		confidence <  maxConfidence ? ++confidence : confidence =  maxConfidence;
	}
	else // no shape to assign availabe, calculate substitute
	{
		--confidence;
		if (confidence <= 0) 
		{
			markedForDelete = true;
		}
		else
			AddSubstitute();
	}
	assigned = false; // set to true in Vehicle::Update
}


TrackEntry& Track::GetActualEntry()
{
	return history.back();
}

int Track::GetConfidence()
{
	return confidence;
}

void Track::SetId(int trkID) { id = trkID;}
	
int Track::GetId() {return id;}

int Track::GetIdxCombine() {return idxCombine;}

void Track::SetIdxCombine(int idx) {idxCombine = idx;}

cv::Point2d Track::GetVelocity() {return avgVelocity;}

bool Track::IsMarkedForDelete() {return markedForDelete;}

bool Track::IsAssigned() 
{
	bool temp = assigned;
	return assigned;
}

void Track::SetAssigned(bool state) {assigned = state;}

// average velocity with recursive formula
void Track::UpdateAvgVelocity()
{
	const int window = 5;
	int lengthHistory = history.size();
	cv::Point2d avg = cv::Point2d(0,0);
	int n = lengthHistory - 1; 	
	if (n > 0) // more than 1 TrackEntries needed in order to calculate velocity
	{
		if (n < window+1) // window not fully populated yet
		{
			avgVelocity.x = (avgVelocity.x * (n - 1) + history[n].velocity.x) / n;
			avgVelocity.y = (avgVelocity.y * (n - 1) + history[n].velocity.y) / n;
		}
		else // moving average over <window> values
		{
			avgVelocity.x += ((history[n].velocity.x - history[n-(int)window].velocity.x) / (double)window);
			avgVelocity.y += ((history[n].velocity.y - history[n-(int)window].velocity.y) / (double)window);
		}
	}
	return;
}

// maxDeviation% in width and height allowed
bool Track::HasSimilarSize(TrackEntry& blob)
{
	if ( abs(GetActualEntry().bbox.width - blob.bbox.width) > (GetActualEntry().bbox.width * maxDeviation / 100 ) )
		return false;
	if ( abs(GetActualEntry().bbox.height - blob.bbox.height) > (GetActualEntry().bbox.height * maxDeviation /100 ) )
		return false;
	return true;
}




// testing only
void Track::ClearHistory()
{
	history.clear();
}