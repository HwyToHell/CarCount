#include "stdafx.h"
#include "keypress.h"

#include "../include/config.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"

using namespace std;

int main(int argc, char* argv[]) {
	/// TODO define region of interest --> TODO: select graphical -or- detect by optical flow
	
	Config config("video.sqlite");
	Config* pConfig = &config;

	FrameHandler frameHandler(pConfig); // frame reading, segmentation, drawing fcn
	FrameHandler* pFrameHandler = &frameHandler;
	config.attach(pFrameHandler);
	
	SceneTracker scene(pConfig); // collection of tracks and vehicles with MOG2
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	list<TrackEntry> bboxList; // debug only, delete after
	list<Track> trackList;
	list<Vehicle> vehicleList;

	if (!frameHandler.openCapSource(true)) 
		return -1;

	while(true)
	{
		
		if (!frameHandler.segmentFrame())
			return -1;
		
		bboxList = frameHandler.calcBBoxes();

		trackList = scene.updateTracks(bboxList);
		//scene.showTracks(frame);

		vehicleList = scene.combineTracks();
		
		frameHandler.showFrame(trackList, vehicleList);


	
		
		StopStartNextFrame();
		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}


	cv::waitKey(0);
	return 0;
}



