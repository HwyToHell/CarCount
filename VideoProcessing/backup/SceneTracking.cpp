#include "stdafx.h"
#include "Tracking.h"


class Vehicle {
public:
	Vehicle(cv::Rect _bbox, cv::Point2d _velocity);
	void Update(std::list<Track>& sceneTracks);
	cv::Rect GetBbox();
	cv::Point2i GetCentroid();
	cv::Point2d GetVelocity();
	cv::Rect bbox;
	cv::Point2i centroid;
	cv::Point2d velocity;

private:
	const int confAssign;	// confidence above this level assigns unassigned trac to vehicle
	const int confVisible;	// confidence above this level makes vehicle visible 
	int distSubTrack;	// ToDo: change to const after testing
	double diffVelocitySubTrack; 

	std::list<Track*> pTracks;

};

// support function
double EuclideanDist(cv::Point& pt1, cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}

// collection of all tracks
Tracks::Tracks()
{
	// set criteria for blob assignment to track
	maxDist = 30; // max 30 pixel distance
	deviation = 80; // max 80% deviation in length and height
	maxConfidence = 5; // number of consecutive frames allowed to use substitute value for
	for (int i = 9; i > 0; --i) // fill lstIDs with 9 ints
		lstIDs.push_back(i);
}

bool Tracks::SimilarShape(TrackEntry& assigned, TrackEntry& toCompare)
{
	// x% deviation in width and height allowed
	if ( abs(assigned.bbox.width - toCompare.bbox.width) > (assigned.bbox.width * deviation / 100 ) )
		return false;
	if ( abs(assigned.bbox.height - toCompare.bbox.height) > (assigned.bbox.height * deviation /100 ) )
		return false;
	return true;
}

void Tracks::UpdateTracks(std::list<TrackEntry>& blobs)
{

	// assign blobs to existing tracks
	std::list<Track>::iterator it = lstTracks.begin();
	while ( it != lstTracks.end() ) // iterate through tracks
	{
		double minDist = maxDist;

		// iterate through detected blobs
		std::list<TrackEntry>::iterator ns = blobs.begin(), nsMinDist = blobs.end();
		while (ns != blobs.end()) 
		{
			// shape similar 
			if (SimilarShape(it->trHistory.back(), *ns) && EuclideanDist(ns->centroid, it->trHistory.back().centroid) < maxDist)
			{
				// centroid distance close
				if ( maxDist > (ns->distance = EuclideanDist(ns->centroid, it->trHistory.back().centroid)) )
				{
					// save smallest distance of all new blobs
					if (ns->distance < minDist)
					{
						minDist = ns->distance;
						nsMinDist = ns;
					}
				}
			}
			++ns;
		}// end iterate through all new blobs

		// assign closest shape to track and delete from list of blobs
		if (nsMinDist != blobs.end()) // shape to assign available
		{
			it->AddTrackEntry(*nsMinDist);
			ns = blobs.erase(nsMinDist);
			it->confidence <  maxConfidence ? it->confidence++ : it->confidence =  maxConfidence;
			it->UpdateVelocity();
			++it;
		}
		else // no shape to assign availabe, calculate substitute
		{
			it->confidence--;
			if (it->confidence < 0) // delete track, if confidence drops below zero
			{
				UnAssignTrackID(it->GetId()); // Un-Assign ID
				it = lstTracks.erase(it); // it points to next element, so ++it must be avoided
			}
			else // calculate subsitute
			{
				it->AddSubstitute();
				it->UpdateVelocity();
				++it;
			}
		}
	}// end for all tracks
	
	// create new tracks for remaining blobs
	SortTrackIDs();
	std::list<TrackEntry>::iterator nsRemain = blobs.begin();
	while (nsRemain != blobs.end())
	{
		int id = AssignTrackID(); // Assign ID
		if (id > 0)
			lstTracks.push_back(Track(*nsRemain, id));
		++nsRemain;
	}
	blobs.clear();


	return;
}

void Tracks::ShowTracks(cv::Mat& frame, int thickness)
{
	//double thickness = 1;

	const cv::Scalar red = cv::Scalar(0,0,255);
	const cv::Scalar orange = cv::Scalar(0,128,255);
	const cv::Scalar yellow = cv::Scalar(0,255,255);
	const cv::Scalar ye_gr = cv::Scalar(0,255,180);
	const cv::Scalar green = cv::Scalar(0,255,0);

	cv::Scalar color[] ={red, orange, yellow, ye_gr, green, green};

	std::list<Track>::iterator iL = lstTracks.begin();
	while (iL != lstTracks.end())
	{
		cv::rectangle(frame, iL->trHistory.back().bbox, color[iL->confidence < 5 ? iL->confidence : 5], thickness);
		cv::Point textPos(iL->trHistory.back().bbox.x, iL->trHistory.back().bbox.y);
		//show ID
		cv::putText(frame, std::to_string((long long) iL->GetId()), textPos, CV_FONT_HERSHEY_DUPLEX, 0.5, color[iL->confidence < 5 ? iL->confidence : 5], 2);
		// show avg velocity
		cv::Point textPos_x(iL->trHistory.back().centroid.x+10, iL->trHistory.back().centroid.y);
		cv::Point textPos_y(iL->trHistory.back().centroid.x, iL->trHistory.back().centroid.y+10);
		int x_vel = (int)iL->GetVelocity().x;
		int y_vel = (int)iL->GetVelocity().y;
		cv::putText(frame, std::to_string((long long) x_vel), textPos_x, 0, 0.5, color[iL->confidence < 5 ? iL->confidence : 5], thickness);
		cv::putText(frame, std::to_string((long long) y_vel), textPos_y, 0, 0.5, color[iL->confidence < 5 ? iL->confidence : 5], thickness);

		++iL;
	}
}

void Tracks::Print()
{
	using namespace std; 
	// print bbox of tracks
	cout << endl << "List of tracks" << endl;
	list<Track>::iterator iL = lstTracks.begin();
	int i = 0;
	while (iL != lstTracks.end())
	{
		i++;
		cout << i << "  [" << setw(3) << iL->trHistory.back().bbox.x << ", " << setw(3) << iL->trHistory.back().bbox.y << ", "
			<< setw(3) << iL->trHistory.back().bbox.width << ", " << setw(3) << iL->trHistory.back().bbox.height << "]" 
			<< "  hist: " << iL->trHistory.size() << "  conf: " << iL->confidence << " vel: " << iL->trHistory.back().velocity.x 
			<< " avgVel: " << iL->GetVelocity().x << endl;
		++iL;
	}// end for all tracks
	return;
}

int Tracks::AssignTrackID()
{
	if (lstIDs.empty()) return 0;
	else 
	{
		int id = lstIDs.back();
		lstIDs.pop_back();
		return id;
	}
}

bool Tracks::UnAssignTrackID(int id)
{
	if (id > 0 )
	{
		lstIDs.push_back(id);
		return true;
	}
	else
		return false;
}

void Tracks::SortTrackIDs()
{
	lstIDs.sort(std::greater<int>());
}




void Tracks::UpdateVehicleFromTrack()
{
	// for each vehicle
			// existing vehicle: assign closest track with high confidence
			// if more close tracks available, combine them
			// if no close track available, calc substitute, reduce confidence
			// if confidence drops below zero, delete vehicle
			// checks: entire scene, obscurance

	std::list<Vehicle>::iterator iv = lstVehicles.begin();
	while (iv != lstVehicles.end()) // iterate through vehicles
	{
		// find associated tracks and check confidence
		// iterate through associated tracks
		std::list<std::list<Track>::iterator>::iterator iAssTrks = iv->GetFirstTrackIter();
		while (iAssTrks != iv-> lstItTracks.end())
		//iv->lstItTracks
			//std::find();

		// new tracks with high confidence and same velocity avaialble --> assign them
		
		
		double minDist = maxDistVehicle;
		std::list<Track>::iterator itr = lstTracks.begin(), itrMinDist = lstTracks.end();
		while (itr != lstTracks.end()) // iterate through tracks
		{
	
			++itr;
		}// end iterate through tracks
		++iv;
	}// end iterate through vehicles





	// remaining tracks with high confidence
			// create new vehicles

	std::list<Track>::iterator iL = lstTracks.begin();
	while (iL != lstTracks.end())
	{
		// existing vehicle: assign closest track with high confidence

		// not assigned tracks with high confidence will create new vehicles


		
		// ToDo: compare velocity of tracks; if same velocity and near = combine

		// if tracks assigned

		//if (iL->confidence > 3)
			//this->lstVehicles.push_back(iL->trHistory.back());
		++iL;
	}
}


void Tracks::CreateVehicle(std::list<Track>::iterator itTrack)
{
	lstVehicles.push_back(Vehicle(itTrack));
}

void Tracks::DeleteVehicle(std::list<Vehicle>::iterator itVehicle)
{
	itVehicle = lstVehicles.erase(itVehicle);
}

void Tracks::ShowVehicles(cv::Mat& frame)
{
	int thickness = 1;

	const cv::Scalar red = cv::Scalar(0,0,255);
	const cv::Scalar orange = cv::Scalar(0,128,255);
	const cv::Scalar yellow = cv::Scalar(0,255,255);
	const cv::Scalar ye_gr = cv::Scalar(0,255,180);
	const cv::Scalar green = cv::Scalar(0,255,0);

	cv::Scalar color[] ={red, orange, yellow, ye_gr, green, green};

	std::list<Track>::iterator iL = lstTracks.begin();
	while (iL != lstTracks.end())
	{
		cv::rectangle(frame, iL->trHistory.back().bbox, color[iL->confidence < 5 ? iL->confidence : 5], thickness);
		cv::Point textPos(iL->trHistory.back().bbox.x, iL->trHistory.back().bbox.y);
		//if (thickness > 1)
			cv::putText(frame, std::to_string((long long) iL->GetId()), textPos, 0, 0.5, color[iL->confidence < 5 ? iL->confidence : 5], thickness);
		++iL;
	}
}


// vehicle representation
Vehicle::Vehicle(std::list<Track>::iterator itTrack)
{
	bbox = itTrack->trHistory.back().bbox;
	centroid = itTrack->trHistory.back().centroid;
	velocity = itTrack->GetVelocity();
	lstItTracks.push_back(itTrack);
}

std::list<std::list<Track>::iterator>::iterator Vehicle::GetFirstTrackIter()
{
	return lstItTracks.begin();
}


// assign tracks to existing vehicle, update outbound rect and velocity
void Vehicle::Update(std::list<Track>& sceneTracks)
{
	std::list<Track>::iterator sc = sceneTracks.begin();
	// ToDo: Flush pTracks in each update step
	pTracks.clear();
	while (sc != sceneTracks.end())
	{
		if ( !sc->IsAssigned() && (sc->GetConfidence() > confAssign) )
		{
			if ( HasSimilarVelocity(*sc) && IsClose(*sc) && IsEntering() )
			{
				sc->SetAssigned(true);
				pTracks.push_back(sc->GetThis());
			}
		}
		++sc;
	}

	// update outbound rectangle (vehicle::bbox) and velocity
	if (pTracks.size() > 0)
	{

		cv::Point2d sumVelocity(0, 0);
		int i = 0;
		std::list<Track*>::iterator trPtr = pTracks.begin();
		while (trPtr != pTracks.end())
		{
			bbox = ( bbox | (*trPtr)->GetActualEntry().bbox );
			sumVelocity += (*trPtr)->GetVelocity();
			++i;
			++trPtr;
		}
		avgVelocity = sumVelocity * (1.0 / i);
	}
}

void Vehicle::RemoveDeletedTracks()
{
		// for each vehicle
			// existing vehicle: assign closest track with high confidence
			// if more close tracks available, combine them
			// if no close track available, calc substitute, reduce confidence
			// if confidence drops below zero, delete vehicle
			// checks: entire scene, obscurance
	std::list<Track>::iterator aTr = assignedTracks.begin();
	while (aTr != assignedTracks.end()) // iterate through assignedTracks
	{
		if (aTr->confidence < 3)
			// make invisible;
		if (aTr->confidence < 1)
			aTr = lstItTracks.erase(aTr);
		else
			++aTr;
	}
	//std::list<Track>::iterator iTr = assignedTracks.begin();
	//while (iTr != assignedTracks.end()) // iterate through list of iterators into list of tracks
	

}


void Scene::CreateVehicles()
{
	std::list<Track>::iterator tr = tracks.begin();
	while (tr != tracks.end())
	{
		if (!tr->IsAssigned())
		{
			if (tr->GetConfidence() > confCreate)
			{
				vehicles.push_back(Vehicle(*tr));
				tr->SetAssigned(true);
			}
		}

		++tr;
	}
}


// TESTS
SCENARIO("Check HasSameVelocity", "[Vehicle]")
{
	GIVEN("Vehicle based on one track with [10, 0] velocity vector")
	{
		Track track1(TrackEntry(0, 0, 100, 50), 1);
		std::list<TrackEntry> blobs;
		blobs.push_back(TrackEntry(10, 0, 100, 50)); // velocity [10,0]
		track1.Update(blobs);

		std::list<Track> scene_tracks;
		scene_tracks.push_back(track1);
		Vehicle vehicle(track1);
		vehicle.Update(scene_tracks);
		cout << "vehicle velocity [10, 0]: " << vehicle.GetVelocity() << endl;

		WHEN("The vehicle is compared with a track of 10% different velocity [11, 0]")
		{

			Track track2(TrackEntry(110, 0, 100, 50), 2);
			blobs.clear();
			blobs.push_back(TrackEntry(121, 0, 100, 50)); 
			track2.Update(blobs);
			cout << "velocity track 2 [11, 0]: " << track2.GetVelocity() << endl;
			THEN("it has similar velocity")
				REQUIRE(vehicle.HasSimilarVelocity(track2) == true);
		}

		WHEN("Vehicle is compared with a track of opposite sign [-10, 0]")
		{
			Track track3(TrackEntry(10, 0, 100, 50), 3);
			blobs.clear();
			blobs.push_back(TrackEntry(0, 0, 100, 50)); 
			track3.Update(blobs);
			cout << "velocity track 3: " << track3.GetVelocity() << endl; 
			THEN("it has NOT similar velocity")
				REQUIRE(vehicle.HasSimilarVelocity(track3) == false);
		}

		WHEN("Vehicle is compared with a track of 60% different velocity [16, 0]")
		{
			Track track4(TrackEntry(0, 0, 100, 50), 2);
			blobs.clear();
			blobs.push_back(TrackEntry(16, 0, 100, 50)); 
			track4.Update(blobs);
			cout << "velocity track 4 [16, 0]: " << track4.GetVelocity() << endl;
			THEN("it has NOT similar velocity")
				REQUIRE(vehicle.HasSimilarVelocity(track4) == false);
		}
	}
	cout << endl;
}

SCENARIO("Check IsClose", "[Vehicle]")
{
	GIVEN("Vehicle based on one track")
	{
		Track track(TrackEntry(0, 0, 100, 50), 1);
		Vehicle vehicle(track);

		std::list<Track> sceneTracks;
		sceneTracks.push_back(track);
		vehicle.Update(sceneTracks);
		//cout << "vehicle bbox [0, 0, 100, 50]: " << vehicle.GetBbox() << endl;
		
		WHEN("Track to compare is closer than 30 pixel") // distance_x = 20
		{
			Track track_IsClose(TrackEntry(120, 0, 100, 20)); 
			REQUIRE(vehicle.IsClose(track_IsClose) == true);
		}

		WHEN("Track to compare has a longer distance than 30 pixel") // distance_x = 50
		{
			Track track_NotClose(TrackEntry(150, 150, 100, 100));
			REQUIRE(vehicle.IsClose(track_NotClose) == false);
		}
	}
}


SCENARIO("Check CreateVehicles() and UpdateVehicle(), creating new vehicle from tracks", "[Scene]")
{
	GIVEN("2 tracks with 7 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 2 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			// initial track
			blobs.push_back(TrackEntry(i*10, 0, 100, 50)); // track close and similar velocity
			// tracks to combine
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20)); // track close and similar velocity
			scene.UpdateTracks(blobs);
		}
		cv::Rect bbox(scene.tracks.front().GetActualEntry().bbox);
		bbox = (bbox | scene.tracks.back().GetActualEntry().bbox);

		WHEN("new scene with no tracks assigned")
		{
			THEN("no vehicle in scene")
				REQUIRE(scene.vehicles.size() == 0);
		}
		WHEN("createVehicle() is called in this new scene")
		{
			scene.CreateVehicles();
			THEN("2 new vehicles are created")
				REQUIRE(scene.vehicles.size() == 2);
			cout << "vehicle 1 bbox [60, 0, 100, 50]: " << scene.vehicles.front().GetBbox(); 
			cout << "vehicle 2 bbox [186, 0, 100, 20]: " << scene.vehicles.back().GetBbox() << endl; 
		}
		WHEN("tracks are updated and vehicle is updated")
		{
			blobs.push_back(TrackEntry(70, 0, 110, 55));
			blobs.push_back(TrackEntry(77+120, 0, 90, 20));
			bool assigned = true;
			scene.UpdateVehicles();
				THEN("2 tracksfor all assigned tracks assign bit is set to true")
				REQUIRE(assigned == true);
		}
		WHEN("2 tracks are close to each other and have similar velocity")
		{
			scene.CombineTracks();
			list<Track>::iterator tr = scene.tracks.begin(); int i = 1;
			THEN("both tracks get the same index (idxCombine)")
				while (tr != scene.tracks.end())
				{
					cout << "track " << i << " - idxCombine: "  << tr->GetIdxCombine() << endl;
					REQUIRE(tr->GetIdxCombine() == 1);
					++tr; ++i;
				}
		}
	}
}

SCENARIO("Check Update() routine, assigning tracks to existing vehicle", "[Vehicle]")
{
	GIVEN("Tracks with 7 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 4 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			// initial track
			blobs.push_back(TrackEntry(i*10, 0, 100, 50)); // track close and similar velocity
			// tracks to combine
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20)); // track close and similar velocity
			blobs.push_back(TrackEntry(150+i*11, 0, 100, 20)); // track NOT close but similar velocity (+50 pixel difference)
			blobs.push_back(TrackEntry(80+i*17, 50, 100, 20)); // track close but velocity NOT similar (+70% difference)
			scene.UpdateTracks(blobs);
		}

		WHEN("Vehicle is close to one track and has similar velocity")
		{
			// Vehicle with bbox[70,0,100,50] and velocity[10,0]
			Track track1(TrackEntry(60, 0, 100, 50), 1);
			blobs.push_back(TrackEntry(70, 0, 100, 50)); 
			track1.Update(blobs);
			std::list<Track> scene_tracks;
			scene_tracks.push_back(track1);
			Vehicle vehicle(track1);
			vehicle.Update(scene_tracks);
			cout << "vehicle bbox [70, 0, 100, 50]: " << vehicle.GetBbox();
			cout << "vehicle velocity [10, 0]: " << vehicle.GetVelocity() << endl << endl;

			cv::Rect bbox(vehicle.GetBbox());
			list<Track>::iterator tr = scene.tracks.begin(); int n = 1; 
			// track close and similar velocity
			cout << "track " << n << " - bbox [60,0,100,50]: " << tr->GetActualEntry().bbox; 
			bbox = (bbox | tr->GetActualEntry().bbox);
			++tr; ++n;
			// track close and similar velocity
			cout << "track " << n << " - bbox [186,0,100,20]: " << tr->GetActualEntry().bbox; 
			bbox = (bbox | tr->GetActualEntry().bbox);
			++tr; ++n;
			cout << "track " << n << " - bbox [216,0,100,20]: " << tr->GetActualEntry().bbox; // track NOT close but similar velocity (+46 pixel difference)
			++tr; ++n; 
			cout << "track " << n << " - bbox [176,50,100,20]: " << tr->GetActualEntry().bbox; // track close but velocity NOT similar (+60% difference)

			vehicle.Update(scene.tracks);
			cout << "vehicle bbox [60, 0, 226, 50]: " << vehicle.GetBbox(); 
			THEN("it is combined with the track and the bounding box is updated")
				REQUIRE(vehicle.GetBbox() == bbox);

		}
	}
}