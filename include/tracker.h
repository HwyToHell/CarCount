#pragma once
#include "../../../cpp/inc/observer.h"

// helper functions
double euclideanDist(cv::Point& pt1, cv::Point& pt2);
double round(double number); // not necessary in C++11
inline bool signBit(double x) {return (x < 0) ? true : false;} // use std signbit in C++11

class Config;

// representation for a blob (detected geometric moving object) in the track vector 
class TrackEntry {
public://private:
	cv::Rect mBbox;
	cv::Point2i mCentroid;

public:
	TrackEntry(int x = 0, int y = 0, int width = 100, int height = 50);
	TrackEntry(cv::Rect rect);
	
	cv::Point2i centroid();
	bool hasSimilarSize(TrackEntry& teCompare, double maxDeviation);
	int height();
	int width();
};


// a time sequence of the same shape (track), with the history of the shape occurances
class Track {
private:
	const double mMaxDist;
	const double mMaxDeviation;
	const int mMaxConfidence;
	cv::Point2d mAvgVelocity;
	bool mAssigned;
	int mConfidence;
	bool mCounted;
	std::vector<TrackEntry> mHistory; // dimension: time
	int mId;
	//int mIdxCombine; //TODO delete
	bool mMarkedForDelete;
	cv::Point2d mTrafficFlow;

	cv::Point2d& updateAverageVelocity();

public:
	Track(TrackEntry& blob, int id = 0);
	Track& operator= (const Track& source);
	void addTrackEntry(const TrackEntry& blob);
	void addSubstitute();
	void clearHistory();
	TrackEntry& getActualEntry();
	int getConfidence();
	int getId();
	double getLength();
	Track* getThis();
	cv::Point2d getVelocity();
	bool hasSimilarVelocityVector(Track& track2, double directionDiff, double normDiff); 
	bool isAssigned(); // delete, if not needed
	bool isClose(Track& track2, int maxDist); // TODO use maxDist member of Track
	bool isCounted();
	bool isMarkedForDelete();
	void setAssigned(bool state);
	void setCounted(bool state);
	void setId(int trkID); // delete, if not needed
	void updateTrack(std::list<TrackEntry>& blobs);
};


class CountRecorder;

class SceneTracker : public Observer {
private:
	int mConfCreate;	// confidence above this level creates vehicle from unassigned track 
	int mDistSubTrack;	// ToDo: change to const after testing
	unsigned int mMaxNoIDs;
	double mMinVelocityL2Norm;
	CountRecorder* mRecorder; 
	cv::Rect mRoi; // region of interest = subwindow of frame
	cv::Point2d mTrafficFlow; // TODO delete after testing Track::hasSimilarVelocityVec
public:
	SceneTracker(Config* pConfig);
	void attachCountRecorder(CountRecorder* pRecorder);
	void SceneTracker::countCars();
	void SceneTracker::countCars2(int frameCnt);
	//std::list<Vehicle>& combineTracks(); // TODO delete
	int nextTrackID();
	void printVehicles();
	bool returnTrackID(int id);
	void update(); // updates observer with subject's parameters (Config)
	std::list<Track>& updateTracks(std::list<TrackEntry>& blobs);
	void updateTracksFromContours(const std::vector<std::vector<cv::Point>>& contours, 
		std::vector<std::vector<cv::Point>>& movingContours);

	//bool HaveSimilarVelocityVector(Track& track1, Track& track2); // TODO delete, after testing Track::hasSimilarVelocityVector
	//bool AreClose(Track& track1, Track& track2); // TODO delete, after testing Track::isClose()
	//void DeleteVehicle();
	// private:
	std::list<Track> mTracks;
	std::list<int> mTrackIDs;
	
	// DEBUG
	void inspect(int frameCnt);

};
