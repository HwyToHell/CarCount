#pragma once

using namespace std;


class CountRecorder {
	int mCarCntLeft;
	int mCarCntRight;
	int mTruckCntLeft;
	int mTruckCntRight;
public:
	CountRecorder() : mCarCntLeft(0), mCarCntRight(0), 
		mTruckCntLeft(0), mTruckCntRight(0) {}
	void updateCnt(bool movesLeft, bool isTruck = 0);
	void printResults();
};