#include "stdafx.h"
#include "../include/tracking.h"


double EuclideanDist(cv::Point& pt1, cv::Point& pt2)
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
Track::Track(TrackEntry& blob, int id) : mMaxDist(30), mMaxDeviation(80), mMaxConfidence(6), mTrafficFlow(1,0),
	mId(id), mConfidence(0), mIdxCombine(0), mMarkedForDelete(false), mAvgVelocity(0,0)
{
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

Track* Track::getThis() {
	return this;
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

// testing only
void Track::clearHistory() {
	mHistory.clear();
}

TrackEntry& Track::getActualEntry() {
	return mHistory.back();
}

int Track::getConfidence() {
	return mConfidence;
}

int Track::getId() {return mId;}

int Track::getIdxCombine() {return mIdxCombine;}

cv::Point2d Track::getVelocity() {return mAvgVelocity;}

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

bool Track::isMarkedForDelete() {return mMarkedForDelete;}

void Track::setAssigned(bool state) {mAssigned = state;}

void Track::setId(int trkID) { mId = trkID;}

void Track::setIdxCombine(int idx) {mIdxCombine = idx;}

// average velocity with recursive formula
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

// mMaxDeviation% in width and height allowed
bool Track::HasSimilarSize(TrackEntry& blob)
{
	if ( abs(getActualEntry().mBbox.width - blob.mBbox.width) > (getActualEntry().mBbox.width * mMaxDeviation / 100 ) )
		return false;
	if ( abs(getActualEntry().mBbox.height - blob.mBbox.height) > (getActualEntry().mBbox.height * mMaxDeviation /100 ) )
		return false;
	return true;
}






// iterate through detected blobs, check if size is similar, distance close, save closest blob to history and delete from blob input list 
void Track::update(std::list<TrackEntry>& blobs) {
	double minDist = mMaxDist; // minDist needed to determine closest track
	std::list<TrackEntry>::iterator ns = blobs.begin(), nsMinDist = blobs.end();
	while (ns != blobs.end()) {
		if ( getActualEntry().hasSimilarSize(*ns, mMaxDeviation) ) {
		//if (HasSimilarSize(*ns)) { // TODO delete if getActualEntry works
			ns->mDistance = EuclideanDist(ns->mCentroid, getActualEntry().mCentroid);
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