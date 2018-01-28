#include "stdafx.h"
#include "../include/config.h"
#include "../include/recorder.h"

using namespace std;





/*
/// iterate through detected blobs, check if size is similar and distance close
/// save closest blob to history and delete from blob input list 
void Track::updateTrack_org(std::list<TrackEntry>& blobs) {
	typedef std::list<TrackEntry>::iterator TiterBlobs;
	double minDist = mMaxDist; // minDist needed to determine closest track
	TiterBlobs iBlobs = blobs.begin();
	TiterBlobs iBlobsMinDist = blobs.end(); // points to blob with closest distance to track
	
	while (iBlobs != blobs.end()) {
		if ( getActualEntry().hasSimilarSize(*iBlobs, mMaxDeviation) ) {
			iBlobs->mDistance = euclideanDist(iBlobs->mCentroid, getActualEntry().mCentroid);
			if (iBlobs->mDistance < minDist) {
				minDist = iBlobs->mDistance;
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
	mAssigned = false; // set to true in Vehicle::Update
}
*/