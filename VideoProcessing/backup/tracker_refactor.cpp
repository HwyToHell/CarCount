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

//////////////////////////////////////////////////////////////////////////////
// Test Cases ////////////////////////////////////////////////////////////////

SCENARIO("Get and return trackIDs", "[SceneTracker]")
{
	GIVEN("scene with no tracks")
	{
		Config config;
		Config* pConfig = &config;
		SceneTracker scene(pConfig);
		WHEN("trackID is pulled 9 times")
		{
			int id[9];
			for (int i = 0; i < 9; ++i)
				id[i] = scene.nextTrackID();
			THEN("next trackid is 0")
				REQUIRE(scene.nextTrackID() == 0);
			AND_THEN("trackID is returned 9 times")
			{
				for (int i = 1; i < 10; ++i)
				{
					REQUIRE(scene.returnTrackID(i));
					//cout << "returned id[" << i << "]: " << i << endl;
				}
				REQUIRE(scene.returnTrackID(10) == false);
				REQUIRE(scene.returnTrackID(0) == false);
			}
		}
	}
}

struct TwoTracks
{
	Track& track1;
	Track& track2;
	TwoTracks(Track& tr1, Track& tr2) : track1(tr1), track2(tr2) {};
	TwoTracks& operator= (const TwoTracks& tSource)
	{
		track1 = tSource.track1;
		track2 = tSource.track2;
		return *this;
	}
};

// Generate two tracks combined in a struct to compare with
//  origin:   start (upper left corner) of inital rectangle
//  velocity: velocity vector the rectangle will be moved at 
TwoTracks multiUpdateScene(SceneTracker& scene, Point2i origin1, Point2i origin2, Point2d velocity1, Point2d velocity2)
{
	scene.mTracks.clear();
	int height = 100, width = 100;
	std::list<TrackEntry> blobs;
	for(size_t i = 0; i < 7; ++i)
	{
		blobs.push_back(TrackEntry(origin1.x + (int)(i*velocity1.x), origin1.y + (int)(i*velocity1.y),  width, height));
		blobs.push_back(TrackEntry(origin2.x + (int)(i*velocity2.x), origin2.y + (int)(i*velocity2.y),  width, height));
		scene.updateTracks(blobs);
	}
	TwoTracks twoTracks(scene.mTracks.front(), scene.mTracks.back());
	return twoTracks;
}

SCENARIO("Test tracks, if they have similar velocity", "[SceneTracker]")
{
	GIVEN("SceneTracker with 2 Tracks with 6 updated history elements")
	{
		Config config;
		Config* pConfig = &config;
		SceneTracker scene(pConfig);
		Point2i org1 = Point2i(100, 100);
		Point2i org2 = Point2i(500, 500);
		Point2d vel1 = Point2d(10,1);
		Point2d vel2 = Point2d(10,-1);
		double directionDiff = 0.1;
		double normDiff = 0.5;

		WHEN("velocity vectors have same positive direction, abs cos_phi is larger than 0.9 and projected length is the same")
		{
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2)); // cos phi = 0.995
			THEN("they have similar velocity")
				REQUIRE(tracks.track1.getActualEntry().(tracks.track2, directionDiff, normDiff) == true);
		}

		WHEN("velocity vectors have same negative direction, abs cos_phi is larger than 0.9 and projected length differs less than 50%")
		{
			Point2d vel1 = Point2d(-10,1);
			Point2d vel2 = Point2d(-7,-1);
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2)); // cos phi = -0.919
			THEN("they have similar velocity")
				REQUIRE(tracks.track1.hasSimilarVelocityVector(tracks.track2, directionDiff, normDiff) == true);
		}

		WHEN("velocity vectors have different direction")
		{
			vel2 = Point2d(-5,-0.5);
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(tracks.track1.hasSimilarVelocityVector(tracks.track2, directionDiff, normDiff) == false);
		}

		WHEN("direction of velocity vector deviates from direction of traffic flow -> cos phi smaller than 0.9")
		{
			vel2 = Point2d(10, 5); // cos phi = 0,89
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(tracks.track1.hasSimilarVelocityVector(tracks.track2, directionDiff, normDiff) == false);
		}

		WHEN("same positive direction, cos phi larger than 0.9, but projected length differs more than 50%")
		{
			vel2 = Point2d(21, 1); // vel2 > vel1 * 2
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(tracks.track1.hasSimilarVelocityVector(tracks.track2, directionDiff, normDiff) == false);
		}
	}
}





*/