#include "stdafx.h"
#include "tracking.h"

using namespace std;
using namespace cv;

ostream& operator<<(ostream& out, vector<vector<double>> mat);
ostream& operator<<(ostream& out, cv::Rect r);




TEST_CASE( "Centroid, positive bbox", "[TrackEntry]" ) {
    TrackEntry te(100, 100, 100, 100);
	REQUIRE(te.centroid.x == 150);
	REQUIRE(te.centroid.y == 150);
}

TEST_CASE( "Centroid, negative bbox", "[TrackEntry]" ) 
{
    TrackEntry te(-100, -100, -100, -100);
	REQUIRE(te.centroid.x == 150);
	REQUIRE(te.centroid.y == 150);
}

TEST_CASE( "using Rect contstructor + idxContour required", "[TrackEntry]" ) 
{
    TrackEntry te(Rect(100, 100, 100, 100), 1);
	REQUIRE(te.centroid.x == 150);
	REQUIRE(te.centroid.y == 150);
	REQUIRE(te.idxContour == 1);
}

TEST_CASE( "ID", "[Track]" ) 
{
	Track track(TrackEntry(0, 0, 100, 100), 1);
	REQUIRE(track.GetId() == 1);
	track.SetId(2);
	REQUIRE(track.GetId() == 2);
}

TEST_CASE( "velocity", "[Track]" ) 
{
	Track track(TrackEntry(0, 0, 100, 100), 1);
	REQUIRE(track.GetActualEntry().bbox == cv::Rect(0, 0, 100, 100));
	REQUIRE(track.GetActualEntry().velocity == cv::Point2i(0, 0));
	REQUIRE(track.GetVelocity() == cv::Point2d(0, 0));

	track.AddTrackEntry(TrackEntry(50, 0, 100, 100));
	REQUIRE(track.GetActualEntry().bbox == cv::Rect(50, 0, 100, 100));
	REQUIRE(track.GetActualEntry().velocity == cv::Point2i(50, 0));
	REQUIRE(track.GetVelocity() == cv::Point2d(50, 0));
	
	track.AddTrackEntry(TrackEntry(150, 0, 100, 100));
	REQUIRE(track.GetActualEntry().bbox == cv::Rect(150, 0, 100, 100));
	REQUIRE(track.GetActualEntry().velocity == cv::Point2i(100, 0));
	REQUIRE(track.GetVelocity() == cv::Point2d(75, 0));

	SECTION("substitute value")
	{
		track.AddSubstitute();
		REQUIRE(track.GetActualEntry().bbox == cv::Rect(150+75, 0, 100, 100));	
		REQUIRE(track.GetVelocity() == cv::Point2d(75, 0)); 		// average velocity remains unchanged
	}

}

TEST_CASE( "confidence increase", "[Track]" ) 
{
	Track track(TrackEntry(0, 0, 100, 100), 1);
	track.AddTrackEntry(TrackEntry(50, 0, 100, 100));
	track.AddTrackEntry(TrackEntry(150, 0, 100, 100));
	std::list<TrackEntry> blobs;
	SECTION ("take closest")
	{
		blobs.push_back(TrackEntry(150+10, 0, 100, 100)); // closest, maxDist = 30
		blobs.push_back(TrackEntry(150+20, 0, 100, 100));
		REQUIRE(blobs.size() == 2);
		track.Update(blobs);
		REQUIRE(track.GetActualEntry().bbox == cv::Rect(150+10, 0, 100, 100));
		REQUIRE(blobs.size() == 1);
		REQUIRE(track.GetConfidence() == 1);
	}
	SECTION ("take similar")
	{
		blobs.push_back(TrackEntry(150, 0, 150, 100)); // shape similar, maxDeviation = 80%
		blobs.push_back(TrackEntry(150, 0, 200, 100)); 
		REQUIRE(blobs.size() == 2);
		track.Update(blobs);
		REQUIRE(track.GetActualEntry().bbox == cv::Rect(150, 0, 150, 100));
		REQUIRE(blobs.size() == 1);
		REQUIRE(track.GetConfidence() == 1);
	}
}

SCENARIO("Track will be deleted, if confidence drops below zero", "[Track]")
{
	GIVEN("Track with 3 updated TrackEntries")
	{
		Track track(TrackEntry(0, 0, 100, 100), 1);		
		std::list<TrackEntry> blobs;
		blobs.push_back(TrackEntry(10, 0, 100, 100)); // closest, maxDist = 30
		blobs.push_back(TrackEntry(30, 0, 100, 100));
		blobs.push_back(TrackEntry(45, 0, 100, 100));
		track.Update(blobs);
		track.Update(blobs);
		track.Update(blobs);
		REQUIRE(blobs.size() == 0);
		REQUIRE(track.GetConfidence() == 3);
		REQUIRE(track.GetActualEntry().bbox == cv::Rect(45, 0, 100, 100));
		REQUIRE(track.GetVelocity() == cv::Point2d(15, 0));
		WHEN("Substitute calculated 3 times")
		{
			blobs.push_back(TrackEntry(130, 0, 100, 100)); // too far >> 45(bbox.x) + 3(updates) * 15(actVelocity) + 30(maxDist) = 120
			REQUIRE(track.IsMarkedForDelete() == false);
			track.Update(blobs);
			track.Update(blobs);
			track.Update(blobs);
			REQUIRE(track.GetConfidence() == 0);	
			REQUIRE(track.IsMarkedForDelete() == true);
		}
	}
}




ostream& operator<<(ostream& out, cv::Rect r)
{
	out << "Rect x=" << r.x << " y=" << r.y << " width=" << r.width << " height=" << r.height << endl;
	return out;
}

ostream& operator<<(ostream& out, vector<vector<double>> mat)
{
	for (unsigned int i = 0; i < mat.size(); i++)
	{
		out << "row " << i << " "; 
		for (unsigned int n = 0; n < mat[i].size(); n++)
			out << mat[i][n] << ", ";
		out << endl;
	}
	return out; 
}

void PrintShapes(list<TrackEntry>& Shapes)
{
	// print bbox of new shapes
	cout << endl << "List of new shapes" << endl;
	list<TrackEntry>::iterator ns = Shapes.begin();
	int i = 0;
	while (ns != Shapes.end()) 
	{
		i++;
		cout << i << "  [" << ns->bbox.x << ", " << ns->bbox.y << ", " << ns->bbox.width << ", " << ns->bbox.height << "]" << endl;
		++ns;
	}// end for all new shapes
	return;
}