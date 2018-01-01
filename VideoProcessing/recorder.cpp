#include "stdafx.h"
#include "../include/recorder.h"


void CountRecorder::updateCnt(bool movesLeft, bool isTruck) {
	if (isTruck) {
		if (movesLeft) {
			++mTruckCntLeft;
		}
		else {
			++mTruckCntRight;
		}
	}
	else { // isCar
		if (movesLeft) {
			++mCarCntLeft;
		}
		else {
			++mCarCntRight;
		}
	}
}

CountResults CountRecorder::getStatus() {
	CountResults cr;
	cr.carLeft = mCarCntLeft;
	cr.carRight = mCarCntRight;
	cr.truckLeft = mTruckCntLeft;
	cr.truckRight = mTruckCntRight;
	return cr;
}

void CountRecorder::printResults() {
	cout << endl;
	cout << "---------- counting results ----------" << endl;
	cout << " <<< car <<< " << setw(3) << mCarCntLeft << " | " << setw(3) << mCarCntRight << " >>> car >>> " << endl;
	cout << " << truck << " << setw(3) << mTruckCntLeft << " | " << setw(3) << mTruckCntRight << " >> truck >> " << endl;
}