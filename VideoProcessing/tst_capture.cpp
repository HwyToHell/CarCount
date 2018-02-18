#include "stdafx.h"
#include "keypress.h"

#include "../include/config.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"
#include "../include/recorder.h"
#include "../../../cpp/inc/program_options.h"

using namespace std;


int main(int argc, char* argv[]) {

	// cv::VideoCapture cap("D:/Users/Holger/count_traffic/traffic.avi");
	cv::VideoCapture cap;
	
	cap.open(0);
		if (!cap.isOpened()) 	{
		cout << "Cannot open camera" << endl;
	}
	cout << "open - video size: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << "x" 
		<< cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;

	int width = 320;
	int height = 240;
	if ( cap.set(CV_CAP_PROP_FRAME_WIDTH, width)
		&& cap.set(CV_CAP_PROP_FRAME_WIDTH, height) )
		cout << "resized to: " << width << "x" << height << endl;

	cout << "resized - video size: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << "x" 
		<< cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;

	cv::Mat frame;
	int cnt = 0;


	while(cap.isOpened()) {
		bool isSuccess = cap.read(frame);
		if (!isSuccess) 		{
			cout << "Cannot read frame from video file" << endl;
			break;
		}
		cnt++;

		cv::imshow("Origin", frame);

		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			break;
		}
	}

	cout << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	return 0;
}