#include "stdafx.h"
#include "Tracking.h"
#include "Parameters.h"

// tracking.h
// backup of track that uses list of blobs in update function
class Track_bak {
public:
	Track(TrackEntry& blob, int id = 0);
	Track* GetThis();
	Track& operator= (const Track& source);
	void AddTrackEntry(const TrackEntry& blob);
	void AddSubstitute();
	void Update(std::list<TrackEntry>& blobs);
	TrackEntry& GetActualEntry();
	int GetConfidence();
	cv::Point2d GetVelocity();
	void SetId(int trkID);
	int GetId();
	int GetIdxCombine();
	void SetIdxCombine(int idx);
	bool IsMarkedForDelete();
	bool IsAssigned();
	void SetAssigned(bool state);
	bool IsClose(Track& track);
	bool HasSimilarVelocity(Track& track);

	void ClearHistory();
private:
	const double maxDist;
	int distSubTrack;	// ToDo: change to const after testing
	const double maxDeviation;
	const int maxConfidence;
	int id;
	int confidence;
	int idxCombine;
	bool markedForDelete;
	bool assigned;
	cv::Point2d avgVelocity;
	std::vector<TrackEntry> history; // dimension: time

	void UpdateAvgVelocity();
	bool HasSimilarSize(TrackEntry& blob);
};


// track vector
Track::Track(TrackEntry& blob, int id) : maxDist(30), maxDeviation(80), maxConfidence(6), 
	id(id), confidence(0), idxCombine(0), markedForDelete(false), avgVelocity(0,0)
{
	// const criteria for blob assignment
	// maxDist = 30 --> maximum of 30 pixel distance between centroids
	// maxDeviation = 80 --> maximum 80% deviation in length and height
	// maxConfidence = 6 --> maximum number of consecutive frames allowed for substitute value calculation
	distSubTrack = 30;	//--> maximum distance for grouping between bounding boxes of tracks
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

// Check proximity of bbox of track with track to compare	
bool Track::IsClose(Track& track)
{
	int dist_x = distSubTrack; // default: 30
	int dist_y = distSubTrack; // default: 30
	bool x_close, y_close;
	
	if (GetActualEntry().bbox.x < track.GetActualEntry().bbox.x)
		x_close = track.GetActualEntry().bbox.x - (GetActualEntry().bbox.x + GetActualEntry().bbox.width) < dist_x ? true : false;
	else
		x_close = GetActualEntry().bbox.x - (track.GetActualEntry().bbox.x + track.GetActualEntry().bbox.width) < dist_x ? true : false;

	if (GetActualEntry().bbox.y < track.GetActualEntry().bbox.y)
		y_close = track.GetActualEntry().bbox.y - (GetActualEntry().bbox.y + GetActualEntry().bbox.height) < dist_y ? true : false;
	else
		y_close = GetActualEntry().bbox.y - (track.GetActualEntry().bbox.y + track.GetActualEntry().bbox.height) < dist_y ? true : false;

	return (x_close & y_close);
}

// Check similarity of velocity of track with track to compare	
bool Track::HasSimilarVelocity(Track& track)
{
	bool xSignVelocityEqual = (signBit(avgVelocity.x) && signBit(track.GetVelocity().x));
	bool ySignVelocityEqual = (signBit(avgVelocity.y) && signBit(track.GetVelocity().y));
	if ( xSignVelocityEqual && ySignVelocityEqual )
	{ // velocity vector in same quadrant (direction)
		if ( abs(avgVelocity.x - track.GetVelocity().x) <= ((avgVelocity.x + track.GetVelocity().x) * 0.5 * 0.5) 
			&& ( abs(avgVelocity.y - track.GetVelocity().y) <= ((avgVelocity.y + track.GetVelocity().y) * 0.5 * 0.5) ) ) 
			return true; // velocity deviation of track < 50%
	}
	return false;
}


// testing only
void Track::ClearHistory()
{
	history.clear();
}