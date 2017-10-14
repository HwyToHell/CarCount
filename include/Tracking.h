

// helper functions
double EuclideanDist(cv::Point& pt1, cv::Point& pt2);
inline bool signBit(double x) {return (x < 0) ? true : false;} // use std signbit in C++11


// representation for a blob (detected geometric moving object) in the track vector 
class TrackEntry {
public:
	TrackEntry(int x = 0, int y = 0, int width = 100, int height = 50, int idx = 0);
	TrackEntry(cv::Rect rect, int idx);
	cv::Rect bbox;
	cv::Point2i centroid;
	int idxContour;
	double distance;
	bool assigned;
	cv::Point2i velocity;

//private:
};


// a time sequence of the same shape (track), with the history of the shape occurances
class Track {
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

	void ClearHistory();
private:
	const double maxDist;
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



// vehicle representation
class Vehicle {
public:
	Vehicle(cv::Rect _bbox, cv::Point2d _velocity, std::vector<int> _contourIndices);
	cv::Rect GetBbox();
	cv::Point2i GetCentroid();
	cv::Point2d GetVelocity();
	cv::Rect bbox;
	cv::Point2i centroid;
	cv::Point2d velocity;
	std::vector<int> contourIndices;

private:
	const int confAssign;	// confidence above this level assigns unassigned trac to vehicle
	const int confVisible;	// confidence above this level makes vehicle visible 

};

class Scene {
public:
	Scene();
	void UpdateTracks(std::list<TrackEntry>& blobs);
	void UpdateTracksFromContours(const std::vector<std::vector<cv::Point>>& contours, 
		std::vector<std::vector<cv::Point>>& movingContours);
	void CombineTracks();
	void _CombineTracks();
	std::vector<int> getAllContourIndices();
	void UpdateVehicles();
	void ShowTracks(cv::Mat& frame);
	void ShowVehicles(cv::Mat& frame);
	void PrintVehicles();
	bool HaveSimilarVelocityVector(Track& track1, Track& track2);
	bool AreClose(Track& track1, Track& track2);
	// private:
	std::list<Track> tracks;
	std::list<Vehicle> vehicles;
	cv::Rect trackingWindow;
	std::list<int> trackIDs;
	const int confCreate;	// confidence above this level creates vehicle from unassigned track 
	int distSubTrack;	// ToDo: change to const after testing
	double minL2NormVelocity; 



	//void DeleteVehicle();
	int NextTrackID();
	bool ReturnTrackID(int id);
private:
	const unsigned int maxNoIDs;
	cv::Point2d trafficFlowUnitVec;
	struct AreaLimits{
		int min;
		int max;
	} blobArea;

};
