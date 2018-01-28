#include "stdafx.h"
#include "../include/config.h"
#include "../include/recorder.h"

using namespace std;

double euclideanDist(cv::Point& pt1, cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}

double round(double number) // not necessary in C++11
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

/******************************************************************************
/*** TrackEntry ***************************************************************
/******************************************************************************/

TrackEntry::TrackEntry(int x, int y, int width, int height) {
	mBbox.x = abs(x);
	mBbox.y = abs(y);
	mBbox.width = abs(width);
	mBbox.height = abs(height);
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

TrackEntry::TrackEntry(cv::Rect _bbox) : mBbox(_bbox) {
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

cv::Point2i TrackEntry::centroid() {
	return mCentroid;
}

// mMaxDeviation% in width and height allowed
bool TrackEntry::hasSimilarSize(TrackEntry& teCompare, double maxDeviation) {
	if ( abs(mBbox.width - teCompare.mBbox.width) > (mBbox.width * maxDeviation / 100 ) )
		return false;
	if ( abs(mBbox.height - teCompare.mBbox.height) > (mBbox.height * maxDeviation / 100 ) )
		return false;
	return true;
}

int TrackEntry::height() {
	return mBbox.height;
}

int TrackEntry::width() {
	return mBbox.width;
}


/******************************************************************************
/*** Track ********************************************************************
/******************************************************************************/

Track::Track(TrackEntry& blob, int id) : mMaxDist(30), mMaxDeviation(80), mMaxConfidence(6), 
	mTrafficFlow(1,0), mId(id), mConfidence(0), mCounted(false), mMarkedForDelete(false), 
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

	updateAverageVelocity();

	return;
}

void Track::addSubstitute() {
	// take average velocity and compose bbox from centroid of previous element
	cv::Point2i velocity((int)round(mAvgVelocity.x), (int)round(mAvgVelocity.y));

	cv::Point2i centroid = mHistory.back().centroid();
	centroid += velocity;
	int height = mHistory.back().height();
	int width = mHistory.back().width();
	int x = centroid.x - width / 2;
	int y = centroid.y - height / 2;
	// TODO: use Track::avgHeight, avgWidth
	// TODO: check lower and upper bounds of video window

	addTrackEntry(TrackEntry(x, y, width, height));
	return;
}

// testing only, move eventually to test class
void Track::clearHistory() { mHistory.clear(); }

TrackEntry& Track::getActualEntry() { return mHistory.back(); }

int Track::getConfidence() { return mConfidence; }

int Track::getId() { return mId; }

double Track::getLength() {
	double length = euclideanDist(mHistory.front().centroid(), mHistory.back().centroid());
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


//TODO isClose --> TrackEntry
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



/// average velocity with recursive formula
cv::Point2d& Track::updateAverageVelocity() {
	const int window = 5;
	int lengthHistory = mHistory.size();

	if (lengthHistory > 1) {
		int idxMax = lengthHistory - 1;
		cv::Point2d actVelocity = mHistory[idxMax].centroid() - mHistory[idxMax-1].centroid();

		if (idxMax <= window) { // window not fully populated yet
			mAvgVelocity = (mAvgVelocity * (double)(idxMax - 1) + actVelocity) * (1.0 / (double)(idxMax));
		}
		else { // window fully populated, remove window-1 velocity value
			cv::Point2d oldVelocity = mHistory[idxMax-window].centroid() - mHistory[idxMax-window-1].centroid();
			mAvgVelocity += (actVelocity - oldVelocity) * (1.0 / (double)window);
		}
	}

	return mAvgVelocity;
}

/// iterate through detected blobs, check if size is similar and distance close
/// save closest blob to history and delete from blob input list 
void Track::updateTrack(std::list<TrackEntry>& blobs) {
	typedef std::list<TrackEntry>::iterator TiterBlobs;
	double minDist = mMaxDist; // minDist needed to determine closest track
	TiterBlobs iBlobs = blobs.begin();
	TiterBlobs iBlobsMinDist = blobs.end(); // points to blob with closest distance to track
	
	while (iBlobs != blobs.end()) {
		if ( getActualEntry().hasSimilarSize(*iBlobs, mMaxDeviation) ) {
			double distance = euclideanDist(iBlobs->centroid(), getActualEntry().centroid());
			if (distance < minDist) {
				minDist = distance;
				iBlobsMinDist = iBlobs;
			}
		}
		++iBlobs;
	}// end iterate through all input blobs

	// assign closest shape to track and delete from list of blobs
	if (iBlobsMinDist != blobs.end()) // shape to assign available
	{
		addTrackEntry(*iBlobsMinDist);
		iBlobs = blobs.erase(iBlobsMinDist);
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
	mAssigned = false; // set to true in SceneTracker::updateTracks
}



/******************************************************************************
/*** SceneTracker *************************************************************
/******************************************************************************/

SceneTracker::SceneTracker(Config* pConfig) : Observer(pConfig) {
	update();
}

void SceneTracker::attachCountRecorder(CountRecorder* pRecorder) {
	mRecorder = pRecorder;
}

/// update parameters from config subject
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
///  create new tracks from unassigned blobs
///  erase, if marked for deletion
std::list<Track>& SceneTracker::updateTracks(list<TrackEntry>& blobs) {

	// assign blobs to existing tracks
	// delete orphaned tracks and free associated Track-ID
	//  for_each track
	std::list<Track>::iterator iTrack = mTracks.begin();
	while (iTrack != mTracks.end())
	{
		iTrack->setAssigned(false);
		iTrack->updateTrack(blobs); // assign new blobs to existing track

		if (iTrack->isMarkedForDelete())
		{
			returnTrackID(iTrack->getId());
			iTrack = mTracks.erase(iTrack);
		}
		else
			++iTrack;
	}
	
	// create new tracks for unassigned blobs
	mTrackIDs.sort(std::greater<int>());
	std::list<TrackEntry>::iterator iBlob = blobs.begin();
	while (iBlob != blobs.end())
	{
		int trackID = nextTrackID();
		if (trackID > 0)
			mTracks.push_back(Track(*iBlob, trackID));
		++iBlob;
	}
	blobs.clear();
	
	return mTracks;
}






struct Trk {
	int id;
	int confidence;
	double velocity;
	bool counted;
	Trk(int id_, int confidence_, double velocity_, bool counted_) : id(id_), confidence(confidence_), velocity(velocity_), counted(counted_) {}
};


void SceneTracker::inspect(int frameCnt) {
	vector<Trk> tracks;
	list<Track>::iterator iTrack = mTracks.begin();
	while (iTrack != mTracks.end()) {
		tracks.push_back(Trk(iTrack->getId(), iTrack->getConfidence(), iTrack->getVelocity().x, iTrack->isCounted()));
		++iTrack;
	}
}

enum direction { left=0, right};


void printCount(int frameCnt, double length, direction dir) {
	char* sDir[] = { " <<<left<<<  ", " >>>right>>> "};
	cout.precision(1);
	cout << "frame: " << setw(4) << fixed << frameCnt << sDir[dir];
	cout << "length: " << setw(4) <<  length << endl;
};

void printCount2(int frameCnt, int width, int height) {
	cout << "frame: " << setw(4) << frameCnt;
	cout << " width: " << setw(4) << width;
	cout << " height: " << setw(4) <<  height << endl << endl;
};


/******************************************************************************
/*** CountAndClassify *********************************************************
/******************************************************************************/


class CountAndClassify {
	int mFrameCnt;
	CountRecorder* mRecorder;

public:
	CountAndClassify(int frameCnt, CountRecorder* pRec) : mFrameCnt(frameCnt), mRecorder(pRec) {}
	void operator()(Track& track) {
		int countPos = 90; // roi.width/2 = "count_pos_x"
		double trackLength = track.getLength();
		if (track.getConfidence() > 3) {
			cv::Point2d velocity(track.getVelocity());
			if (signBit(velocity.x)) { // moving to left
				if ((track.getActualEntry().centroid().x < countPos) && !track.isCounted()) {
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
				if ((track.getActualEntry().centroid().x > countPos) && !track.isCounted()) {
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
			if (track.getActualEntry().centroid().x < countPos) {
				track.setCounted(true);
			}
		}
		else {// moving to right
			if (track.getActualEntry().centroid().x > countPos)
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

void SceneTracker::countCars2(int frameCnt) {
	// track confidence > x
	// moving left && position < yy
	// moving right && position > yy
	for_each(mTracks.begin(), mTracks.end(), CountAndClassify(frameCnt, mRecorder));
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