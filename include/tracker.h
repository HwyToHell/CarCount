#pragma once
#include "../../../cpp/inc/observer.h"
#include "../include/recorder.h"

// helper functions
double euclideanDist(const cv::Point& pt1, const cv::Point& pt2);
double round(double number); // not necessary in C++11
inline bool signBit(double x) {return (x < 0) ? true : false;} // use std signbit in C++11


/// config.h
class Config;

/// representation for a blob (detected geometric moving object) in the track vector 
class TrackEntry {
private:
	cv::Rect mBbox;
	cv::Point2i mCentroid;

public:
	TrackEntry(int x = 0, int y = 0, int width = 100, int height = 50);
	TrackEntry(cv::Rect rect);
	
	cv::Point2i& centroid();
	int height();
	cv::Rect& rect();
	int width();

	bool isClose(TrackEntry& teCompare, int maxDist);
	bool isSizeSimilar(TrackEntry& teCompare, double maxDeviation);
};


/// a time sequence of the same shape (track), with the history of the shape occurances
class Track {
private:
	cv::Point2d mAvgVelocity;
	int mConfidence;
	bool mCounted;
	std::vector<TrackEntry> mHistory; // dimension: time
	int mId;
	bool mMarkedForDelete;

	cv::Point2d& updateAverageVelocity();

public:
	Track(TrackEntry& blob, int id = 0);
	Track& operator= (const Track& source);
	
	void addTrackEntry(const TrackEntry& blob);
	void addSubstitute();
	void checkPosAndDir(); // TODO delete

	TrackEntry& getActualEntry();
	int getConfidence();
	int getId();
	double getLength();
	cv::Point2d getVelocity();
	bool isCounted();
	bool isMarkedForDelete();
	void setCounted(bool state);
	void updateTrack(std::list<TrackEntry>& blobs, int maxConf, 
		double maxDeviation, double maxDist);
};

/// recorder.h
//class CountRecorder;


/// 
class SceneTracker : public Observer {
private:
	// adjustable parameters
	ClassifyVehicle mClassify;
	int mMaxConfidence;
	double mMaxDeviation;
	double mMaxDist;
	int mMaxNoIDs;

	// variables
	CountRecorder* mRecorder; 
	std::list<Track> mTracks;
	std::list<int> mTrackIDs;

public:
	SceneTracker(Config* pConfig);

	void attachCountRecorder(CountRecorder* pRecorder);
	CountResults countVehicles(int frameCnt = 0);
	int nextTrackID();
	void printVehicles();
	bool returnTrackID(int id);
	void update(); // updates observer with subject's parameters (Config)
	std::list<Track>& updateTracks(std::list<TrackEntry>& blobs);
	
	// DEBUG
	void inspect(int frameCnt);
	// END DEBUG
};
