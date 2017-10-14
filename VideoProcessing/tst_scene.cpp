#include "stdafx.h"
#include "../include/Tracking.h"

using namespace std;
using namespace cv;

void prnTrack(Track& track);

SCENARIO("Get and return trackIDs", "[Scene]")
{
	GIVEN("scene with no tracks")
	{
		Scene scene;
		WHEN("trackID is pulled 9 times")
		{
			int id[9];
			for (int i = 0; i < 9; ++i)
				id[i] = scene.NextTrackID();
			THEN("next trackid is 0")
				REQUIRE(scene.NextTrackID() == 0);
			AND_THEN("trackID is returned 9 times")
			{
				for (int i = 1; i < 10; ++i)
				{
					REQUIRE(scene.ReturnTrackID(i));
					//cout << "returned id[" << i << "]: " << i << endl;
				}
				REQUIRE(scene.ReturnTrackID(10) == false);
				REQUIRE(scene.ReturnTrackID(0) == false);
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
TwoTracks multiUpdateScene(Scene& scene, Point2i origin1, Point2i origin2, Point2d velocity1, Point2d velocity2)
{
	scene.tracks.clear();
	int height = 100, width = 100;
	std::list<TrackEntry> blobs;
	for(size_t i = 0; i < 7; ++i)
	{
		blobs.push_back(TrackEntry(origin1.x + (int)(i*velocity1.x), origin1.y + (int)(i*velocity1.y),  width, height));
		blobs.push_back(TrackEntry(origin2.x + (int)(i*velocity2.x), origin2.y + (int)(i*velocity2.y),  width, height));
		scene.UpdateTracks(blobs);
	}
	TwoTracks twoTracks(scene.tracks.front(), scene.tracks.back());
	return twoTracks;
}

SCENARIO("Test tracks, if they have similar velocity", "[Scene]")
{
	GIVEN("scene with 2 Tracks with 6 updated history elements")
	{
		Scene scene;
		Point2i org1 = Point2i(100, 100);
		Point2i org2 = Point2i(500, 500);
		Point2d vel1 = Point2d(10,1);
		Point2d vel2 = Point2d(10,-1);

		WHEN("velocity vectors have same positive direction, abs cos_phi is larger than 0.9 and projected length is the same")
		{
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2)); // cos phi = 0.995
			THEN("they have similar velocity")
				REQUIRE(scene.HaveSimilarVelocityVector(tracks.track1, tracks.track2) == true);
		}

		WHEN("velocity vectors have same negative direction, abs cos_phi is larger than 0.9 and projected length differs less than 50%")
		{
			Point2d vel1 = Point2d(-10,1);
			Point2d vel2 = Point2d(-7,-1);
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2)); // cos phi = -0.919
			THEN("they have similar velocity")
				REQUIRE(scene.HaveSimilarVelocityVector(tracks.track1, tracks.track2) == true);
		}

		WHEN("velocity vectors have different direction")
		{
			vel2 = Point2d(-5,-0.5);
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(scene.HaveSimilarVelocityVector(tracks.track1, tracks.track2) == false);
		}

		WHEN("direction of velocity vector deviates from direction of traffic flow -> cos phi smaller than 0.9")
		{
			vel2 = Point2d(10, 5); // cos phi = 0,89
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(scene.HaveSimilarVelocityVector(tracks.track1, tracks.track2) == false);
		}

		WHEN("same positive direction, cos phi larger than 0.9, but projected length differs more than 50%")
		{
			vel2 = Point2d(21, 1); // vel2 > vel1 * 2
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			THEN("they don't have similar velocity")
				REQUIRE(scene.HaveSimilarVelocityVector(tracks.track1, tracks.track2) == false);
		}
	}
}


SCENARIO("Check on combining vehicles together", "[Scene]")
{
	GIVEN("5 Tracks with 6 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 4 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			blobs.push_back(TrackEntry(i*10, 0, 100, 50));		// [1] initial track
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20));	// [1] combined (close and similar velocity )
			blobs.push_back(TrackEntry(300+i*11, 0, 100, 20));	//  [2] NOT close but similar velocity (+50 pixel difference)
			blobs.push_back(TrackEntry(50+i*10, 40, 80, 30));	// [1] close and similar velocity
			blobs.push_back(TrackEntry(80+i*22, 50, 100, 20));	//  [3] close but velocity NOT similar (+100% difference)
			scene.UpdateTracks(blobs);
		}
		for_each(scene.tracks.begin(), scene.tracks.end(), prnTrack);
		
		WHEN("3 tracks are close and have similar velocity")
		{
			THEN("3 tracks are combined, 2 stay separate")
			{
				scene.CombineTracks();
				REQUIRE(scene.vehicles.size() == 3);
				list<Vehicle>::iterator ve = scene.vehicles.begin();
				REQUIRE(ve->GetBbox() == cv::Rect(60, 0, 226, 70));		// [1]
				++ve;
				REQUIRE(ve->GetBbox() == cv::Rect(366, 0, 100, 20));	// [2]
				++ve;
				REQUIRE(ve->GetBbox() == cv::Rect(212, 50, 100, 20));	// [3]
			}
		}
		// scene.PrintVehicles();
	}
}


SCENARIO("Only moving tracks will be combined, not very slow moving ones", "[Scene]")
{
	GIVEN("scene with 2 Tracks with 6 updated history elements")
	{
		// 	minL2NormVelocity = 0.5;
		Scene scene;
		Point2i org1 = Point2i(100, 100);
		Point2i org2 = Point2i(200, 100);
		Point2d vel1 = Point2d(1, 0.2);

		WHEN("L2 norm of velocity vector < 0.5")
		{
			Point2d vel2 = Point2d(0.3, 0.1); // L2 norm = 0.2; cos(phi) = 1
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			scene.CombineTracks();
			THEN("slow moving track is not considered for combination to vehicle")
				REQUIRE(scene.vehicles.front().GetBbox().width == 100);
		}

		WHEN("L2 norm of velocity vector > 0.5")
		{
			Point2d vel2 = Point2d(1, 0.2); // L2 norm = 1.01; cos(phi) = 0.98
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			scene.CombineTracks();
			THEN("track moves fast enough and is combined")
				REQUIRE(scene.vehicles.front().GetBbox().width > 100);
		}

	}
}


SCENARIO("Check on returning idxContour for each vehicle", "[Scene]")
{
	GIVEN("5 Tracks with 6 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 4 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			blobs.push_back(TrackEntry(i*10, 0, 100, 50, 1));		// vehicle: 1, idxContour: 1
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20, 3));	// vehicle: 1, idxContour: 3
			blobs.push_back(TrackEntry(300+i*11, 0, 100, 20, 4));	// vehicle: 2, idxContour: 4
			blobs.push_back(TrackEntry(50+i*10, 40, 80, 30, 7));	// vehicle: 1, idxContour: 7
			blobs.push_back(TrackEntry(80+i*22, 50, 100, 20, 9));	// vehicle: 3, idxContour: 9
			scene.UpdateTracks(blobs);
		}
		
		WHEN("tracks are combined to vehicles")
		{
			THEN("the appropriate vector for the contour indices are returned")
			{
				scene.CombineTracks();
				list<Vehicle>::iterator ve = scene.vehicles.begin();
				// vehicle 1: [1, 3, 7]
				REQUIRE(ve->contourIndices.at(0) == 1);
				REQUIRE(ve->contourIndices.at(1) == 3);			
				REQUIRE(ve->contourIndices.at(2) == 7);
				++ve;
				// vehicle 2: [4]
				REQUIRE(ve->contourIndices.at(0) == 4);
				++ve;
				// vehicle 3: [9]
				REQUIRE(ve->contourIndices.at(0) == 9);
			}
		}
		// scene.PrintVehicles();
	}
}



int wherex()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(
		GetStdHandle( STD_OUTPUT_HANDLE ),
		&csbi))
		return -1;
	return csbi.dwCursorPosition.X;
}

int wherey()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(
		GetStdHandle( STD_OUTPUT_HANDLE ),
		&csbi))
		return -1;
	return csbi.dwCursorPosition.Y;
}

void prnTrack(Track& track)
{
	cout << "ID:" << track.GetId() << " bbox: " << track.GetActualEntry().bbox;
	int padding = 38 - wherex();
	if (padding > 0)
		for (int i = 0; i < padding; ++i)
			cout << " ";
	cout << " velocity: " << track.GetVelocity() << endl;
}