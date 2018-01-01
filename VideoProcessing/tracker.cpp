#include "stdafx.h"
#include "../include/config.h"
#include "../include/tracker.h"
#include "../include/recorder.h"

double euclideanDist(cv::Point& pt1, cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}

double round(double number) // not necessary in C++11
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

// detected blobs
TrackEntry::TrackEntry(int x, int y, int width, int height, int idx) : mIdxContour(idx), mVelocity(0,0)
{
	mBbox.x = abs(x);
	mBbox.y = abs(y);
	mBbox.width = abs(width);
	mBbox.height = abs(height);
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

TrackEntry::TrackEntry(cv::Rect _bbox, int _idxContour) : mBbox(_bbox), mIdxContour(_idxContour), mVelocity(0,0)
{
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

// mMaxDeviation% in width and height allowed
bool TrackEntry::hasSimilarSize(TrackEntry& teCompare, double maxDeviation)
{
	if ( abs(mBbox.width - teCompare.mBbox.width) > (mBbox.width * maxDeviation / 100 ) )
		return false;
	if ( abs(mBbox.height - teCompare.mBbox.height) > (mBbox.height * maxDeviation / 100 ) )
		return false;
	return true;
}


// track vector
Track::Track(TrackEntry& blob, int id) : mMaxDist(30), mMaxDeviation(80), mMaxConfidence(6), 
	mTrafficFlow(1,0), mId(id), mConfidence(0), mIdxCombine(0), mCounted(false), mMarkedForDelete(false), 
	mAvgVelocity(0,0) {
	// const criteria for blob assignment
	// mMaxDist = 30 --> maximum of 30 pixel distance between centroids
	// mMaxDeviation = 80 --> maximum 80% deviation in length and height
	// maxConfidence = 6 --> maximum number of consecutive frames allowed for substitute value calculation
	mHistory.push_back(blob);
}

Track& Track::operator= (const Track& source) {
	mHistory = source.mHistory;
	return *this;
}

void Track::addTrackEntry(const TrackEntry& blob) {
	mHistory.push_back(blob);
	int lengthHistory = mHistory.size();
	if (lengthHistory > 1) { // update velocity from n-1 positions
		mHistory[lengthHistory-1].mVelocity = mHistory[lengthHistory-1].mCentroid - mHistory[lengthHistory-2].mCentroid;
	}
	updateAvgVelocity();
	return;
}

void Track::addSubstitute() {
	// take average velocity and compose bbox from centroid of previous element
	cv::Point2i velocity((int)round(mAvgVelocity.x), (int)round(mAvgVelocity.y));
	TrackEntry substitute = mHistory.back();
	substitute.mCentroid += velocity;
	substitute.mBbox.x = substitute.mCentroid.x - substitute.mBbox.width / 2;
	substitute.mBbox.x < 0 ? 0 : substitute.mBbox.x;
	substitute.mBbox.y = substitute.mCentroid.y - substitute.mBbox.height / 2;
	substitute.mBbox.y < 0 ? 0 : substitute.mBbox.y;
	// ToDo: check upper bounds of video window
	addTrackEntry(substitute);
	return;
}

// testing only, move eventually to test class
void Track::clearHistory() { mHistory.clear(); }

TrackEntry& Track::getActualEntry() { return mHistory.back(); }

int Track::getConfidence() { return mConfidence; }

int Track::getId() { return mId; }

int Track::getIdxCombine() { return mIdxCombine; }

double Track::getLength() {
	double length = euclideanDist(mHistory.front().mCentroid, mHistory.back().mCentroid);
	return length;
}

Track* Track::getThis() { return this; }

cv::Point2d Track::getVelocity() { return mAvgVelocity; }

bool Track::hasSimilarVelocityVector(Track& track2, double directionDiff, double normDiff) {
	// directionDiff:	max difference of direction 
	//						0 = must have same direction (cos phi = 1)
	//						1 = can differ by 100%
	// normDiff:		max deviation of L1 norm 
	//						0 = must have same length 
	//						1 = can differ by 100%

	// TODO move to constructor, remove as parameter in fcn call
	if (directionDiff < 0) 
		directionDiff = 0;
	if (directionDiff > 1)
		directionDiff = 1;
	if (normDiff < 0) 
		normDiff = 0;
	if (directionDiff > 0.9)
		normDiff = 0.9;

	double normDiffLower = 1 - normDiff;		// normDiffLower 0.1 ... 1
	double normDiffUpper = 1 / normDiffLower;	// normDiffUpper  10 ... 1

	cv::Point2d vel1 = getVelocity();
	cv::Point2d vel2 = track2.getVelocity();

	double vel1_DotTraffic = mTrafficFlow.ddot(vel1);
	double cosPhiVel1 = vel1_DotTraffic / cv::norm(vel1) / cv::norm(mTrafficFlow);

	double vel2_DotTraffic = mTrafficFlow.ddot(vel2);
	double cosPhiVel2 = vel2_DotTraffic / cv::norm(vel2) / cv::norm(mTrafficFlow);

	if (signBit(vel1_DotTraffic) != signBit(vel2_DotTraffic))
		return false; // opposite direction

	if ( (std::abs(cosPhiVel1) < (1 - directionDiff)) || (std::abs(cosPhiVel2) < (1 - directionDiff)) )
		return false; // direction deviation too large
	


	if ( (abs(vel2_DotTraffic) > abs(vel1_DotTraffic * normDiffLower)) && abs(vel2_DotTraffic) < abs(vel1_DotTraffic * normDiffUpper) )
		return true; // normDiff = 0.25: vel1 * 0.75 < vel2 < vel1 * 1.333 ---> max velocity difference 50%
	else
		return false;
}

bool Track::isAssigned() {return mAssigned;}

bool Track::isClose(Track& track2, int maxDist) {
	int dist_x = maxDist; // default: 30
	int dist_y = maxDist; // default: 30
	bool x_close, y_close;
	
	if (getActualEntry().mBbox.x < track2.getActualEntry().mBbox.x)
		x_close = track2.getActualEntry().mBbox.x - (getActualEntry().mBbox.x + getActualEntry().mBbox.width) < dist_x ? true : false;
	else
		x_close = getActualEntry().mBbox.x - (track2.getActualEntry().mBbox.x + track2.getActualEntry().mBbox.width) < dist_x ? true : false;

	if (getActualEntry().mBbox.y < track2.getActualEntry().mBbox.y)
		y_close = track2.getActualEntry().mBbox.y - (getActualEntry().mBbox.y + getActualEntry().mBbox.height) < dist_y ? true : false;
	else
		y_close = getActualEntry().mBbox.y - (track2.getActualEntry().mBbox.y + track2.getActualEntry().mBbox.height) < dist_y ? true : false;

	return (x_close & y_close);
}

bool Track::isCounted() { return mCounted; }

bool Track::isMarkedForDelete() {return mMarkedForDelete;}

void Track::setAssigned(bool state) {mAssigned = state;}

void Track::setCounted(bool state) { mCounted = state; }

void Track::setId(int trkID) { mId = trkID;}

void Track::setIdxCombine(int idx) {mIdxCombine = idx;}

/// average velocity with recursive formula
void Track::updateAvgVelocity()
{
	const int window = 5;
	int lengthHistory = mHistory.size();
	cv::Point2d avg = cv::Point2d(0,0);
	int n = lengthHistory - 1; 	
	if (n > 0) // more than 1 TrackEntries needed in order to calculate velocity
	{
		if (n < window+1) // window not fully populated yet
		{
			mAvgVelocity.x = (mAvgVelocity.x * (n - 1) + mHistory[n].mVelocity.x) / n;
			mAvgVelocity.y = (mAvgVelocity.y * (n - 1) + mHistory[n].mVelocity.y) / n;
		}
		else // moving average over <window> values
		{
			mAvgVelocity.x += ((mHistory[n].mVelocity.x - mHistory[n-(int)window].mVelocity.x) / (double)window);
			mAvgVelocity.y += ((mHistory[n].mVelocity.y - mHistory[n-(int)window].mVelocity.y) / (double)window);
		}
	}
	return;
}


/// iterate through detected blobs, check if size is similar, distance close, save closest blob to history and delete from blob input list 
void Track::updateTrack(std::list<TrackEntry>& blobs) {
	double minDist = mMaxDist; // minDist needed to determine closest track
	std::list<TrackEntry>::iterator ns = blobs.begin(), nsMinDist = blobs.end();
	while (ns != blobs.end()) {
		if ( getActualEntry().hasSimilarSize(*ns, mMaxDeviation) ) {
			ns->mDistance = euclideanDist(ns->mCentroid, getActualEntry().mCentroid);
			if (ns->mDistance < minDist) {
				minDist = ns->mDistance;
				nsMinDist = ns;
			}
		}
		++ns;
	}// end iterate through all input blobs

	// assign closest shape to track and delete from list of blobs
	if (nsMinDist != blobs.end()) // shape to assign available
	{
		addTrackEntry(*nsMinDist);
		ns = blobs.erase(nsMinDist);
		mConfidence <  mMaxConfidence ? ++mConfidence : mConfidence =  mMaxConfidence;
	}
	else // no shape to assign availabe, calculate substitute
	{
		--mConfidence;
		if (mConfidence <= 0) 
		{
			mMarkedForDelete = true;
		}
		else
			addSubstitute();
	}
	mAssigned = false; // set to true in Vehicle::Update
}



/******************************************************************************
/**************************** SceneTracker ***********************************/

SceneTracker::SceneTracker(Config* pConfig) : Observer(pConfig) {
	update();
}

void SceneTracker::attachCountRecorder(CountRecorder* pRecorder) {
	mRecorder = pRecorder;
}


void SceneTracker::update() {
	mConfCreate = (int)mSubject->getDouble("group_min_confidence");
	mDistSubTrack = (int)mSubject->getDouble("group_distance");
	mMaxNoIDs = (int)mSubject->getDouble("max_n_of_tracks");
	mMinVelocityL2Norm = mSubject->getDouble("group_min_velocity");
	mTrafficFlow.x = mSubject->getDouble("traffic_flow_x");
	mTrafficFlow.y = mSubject->getDouble("traffic_flow_y");

	// mConfCreate(3)			-> confidence above this level creates vehicles from unassigned tracks
	// mDistSubTrack(30)		-> max distance for grouping between bounding boxes of tracks
	// mMaxNoIDs(9)				-> max number of tracks
	// mMinVelocityL2Norm(0.5)	-> slower tracks won't be grouped
	// mTrafficFlow.x(1)		-> horizontal component of traffic flow (normalized)
	// mTrafficFlow.y(0)		-> vertical component of traffic flow (normalized)
	for (int i = mMaxNoIDs; i > 0; --i) // fill trackIDs with 9 ints
	mTrackIDs.push_back(i);
}

/// assign blobs to existing tracks
/// create new tracks from unassigned blobs
/// erase, if marked for deletion
list<Track>& SceneTracker::updateTracks(list<TrackEntry>& blobs) {
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
		tr->updateTrack(blobs);
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
	return mTracks;
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



bool SceneTracker::HaveSimilarVelocityVector(Track& track1, Track& track2)
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
bool SceneTracker::AreClose(Track& track1, Track& track2)
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


struct Trk {
	int id;
	int confidence;
	double velocity;
	bool counted;
	Trk(int id_, int confidence_, double velocity_, bool counted_) : id(id_), confidence(confidence_), velocity(velocity_), counted(counted_) {}
};


void SceneTracker::inspect(int frmCnt) {
	vector<Trk> tracks;
	list<Track>::iterator iTrack = mTracks.begin();
	while (iTrack != mTracks.end()) {
		tracks.push_back(Trk(iTrack->getId(), iTrack->getConfidence(), iTrack->getActualEntry().mVelocity.x, iTrack->isCounted()));
		++iTrack;
	}
}

enum direction { left=0, right};


void printCount(int frmCnt, double length, direction dir) {
	char* sDir[] = { " <<<left<<<  ", " >>>right>>> "};
	cout.precision(1);
	cout << "frame: " << setw(4) << fixed << frmCnt << sDir[dir];
	cout << "length: " << setw(4) <<  length << endl;
};

void printCount2(int frmCnt, int width, int height) {
	cout << "frame: " << setw(4) << frmCnt;
	cout << " width: " << setw(4) << width;
	cout << " height: " << setw(4) <<  height << endl << endl;
};


class CountAndClassify {
	int mFrameCnt;
	CountRecorder* mRecorder;

public:
	CountAndClassify(int frmCnt, CountRecorder* pRec) : mFrameCnt(frmCnt), mRecorder(pRec) {}
	void operator()(Track& track) {
		int countPos = 90; // roi.width/2 = "count_pos_x"
		double trackLength = track.getLength();
		if (track.getConfidence() > 3) {
			cv::Point2d velocity(track.getVelocity());
			if (signBit(velocity.x)) { // moving to left
				if ((track.getActualEntry().mCentroid.x < countPos) && !track.isCounted()) {
					if (trackLength > 20) { // "count_track_length"
						track.setCounted(true);
						bool movesLeft = true;
						bool isTruck = false;
						int width = track.getActualEntry().mBbox.width;
						int height = track.getActualEntry().mBbox.height;
						if ( (width > 60) && (height > 28) ) // "truck_width_min" "truck_height_min"
							isTruck = true;
						mRecorder->updateCnt(movesLeft, isTruck);
						//printCount(mFrameCnt, track.getLength(), direction::left);
						//printCount2(mFrameCnt, track.getActualEntry().mBbox.width, track.getActualEntry().mBbox.height);
					}
				}
			}
			else {// moving to right
				if ((track.getActualEntry().mCentroid.x > countPos) && !track.isCounted()) {
					if (trackLength > 20) {
						track.setCounted(true);
						bool movesLeft = false;
						bool isTruck = false;
						int width = track.getActualEntry().mBbox.width;
						int height = track.getActualEntry().mBbox.height;
						if ( (width > 60) && (height > 28) ) // "truck_width_min" "truck_height_min"
							isTruck = true;
						mRecorder->updateCnt(movesLeft, isTruck);
						//printCount(mFrameCnt, track.getLength(), direction::right);
						//printCount2(mFrameCnt, track.getActualEntry().mBbox.width, track.getActualEntry().mBbox.height);
					}
				}
			}
		}
	}
};

/// counting conditions for vehicle track:
/// - above confidence level
/// - beyond counting position
/// - completed track length
/// classifying conditions for trucks:
/// - average width and height
void checkPosAndDir(Track& track) {
	//int countPos = mFramesize / 2;
	int countPos = 90; // roi.width/2
	double trackLength;
	if (track.getConfidence() > 4) {
		cv::Point2d velocity(track.getVelocity());
		if (signBit(velocity.x)) { // moving to left
			if (track.getActualEntry().mCentroid.x < countPos) {
				track.setCounted(true);
			}
		}
		else {// moving to right
			if (track.getActualEntry().mCentroid.x > countPos)
				track.setCounted(true);
		}
	}
}

/// count vehicles: count only tracks above defined confidence level
void SceneTracker::countCars() {
	// track confidence > x
	// moving left && position < yy
	// moving right && position > yy
	for_each(mTracks.begin(), mTracks.end(), checkPosAndDir);
}

void SceneTracker::countCars2(int frmCnt) {
	// track confidence > x
	// moving left && position < yy
	// moving right && position > yy
	for_each(mTracks.begin(), mTracks.end(), CountAndClassify(frmCnt, mRecorder));
}

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

list<Vehicle>& SceneTracker::getVehicles() {
	return mVehicles;
}


int SceneTracker::nextTrackID()
{
	if (mTrackIDs.empty()) return 0;
	else 
	{
		int id = mTrackIDs.back();
		mTrackIDs.pop_back();
		return id;
	}
}

bool SceneTracker::returnTrackID(int id)
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



void SceneTracker::showTracks(cv::Mat& frame)
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