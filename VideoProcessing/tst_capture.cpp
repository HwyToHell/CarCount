#include "stdafx.h"

#include "../include/cam_cap_dshow.h"
#include "../include/config.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"
#include "../include/recorder.h"
#include "../../../cpp/inc/program_options.h"


int main(int argc, char* argv[]) {
	using namespace std;

	// set device
	CamInput* pCamInput = new CamInput;
	int iDevices = pCamInput->enumerateDevices();
	bool success = pCamInput->setDevice(0);

	
	// print capabilities of capture device in graph
	int nCaps = pCamInput->enumerateStreamCaps();
	for (int iCap = 0; iCap < nCaps; ++iCap) {
		cv::Size frameSize = pCamInput->getResolution(iCap);
		cout << iCap << " resolution: " << frameSize.width << "x" << frameSize.height << endl;
	}

	// print actual resolution
	if (pCamInput->setResolution(6)) {
		double width = pCamInput->get(CV_CAP_PROP_FRAME_WIDTH);
		double height = pCamInput->get(CV_CAP_PROP_FRAME_HEIGHT);
		cout << "actual resolution: " << width << "x" << height << endl;
	} else
		cout << "resolution not set" << endl;
		
	// start graph
	success = pCamInput->runGraph();
	
	// check, if graph is running
	if (pCamInput->isGraphRunning()) {
		cout << "graph running" << endl;
	} else {
		cout << "graph not running yet" << endl;
	}

	// display grabbed image
	cv::Mat image;
	while (pCamInput->read(image)) {
		cv::imshow("grabbed image", image);

		if (cv::waitKey(10) == 27) 	{
			std::cout << "ESC pressed -> end video processing" << std::endl;
			break;
		}
	}

	//success = pCamInput->stopGraph(); // not necessary as stopGraph() called in D'tor
	delete pCamInput;

	cout << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	return 0;
}