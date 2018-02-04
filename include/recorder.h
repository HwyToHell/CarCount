#pragma once


struct CountResults {
	int carLeft;
	int carRight;
	int truckLeft;
	int truckRight;
	CountResults() : carLeft(0), carRight(0), truckLeft(0), truckRight(0) {}
	CountResults operator+=(const CountResults& rhs);
};


struct ClassifyVehicle {
	int countConfidence;
	int countPos;
	int trackLength;
	cv::Size2i truckSize;
};


class CountRecorder {
	int mCarCntLeft;
	int mCarCntRight;
	int mTruckCntLeft;
	int mTruckCntRight;
public:
	CountRecorder() : mCarCntLeft(0), mCarCntRight(0), 
		mTruckCntLeft(0), mTruckCntRight(0) {}
	CountResults getStatus();
	void updateCnt(bool movesLeft, bool isTruck = 0);
	void printResults();
};