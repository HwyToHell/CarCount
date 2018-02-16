#include "stdafx.h"
#include "keypress.h"

#include "../include/config.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"
#include "../include/recorder.h"
#include "../../../cpp/inc/program_options.h"

using namespace std;

int main(int argc, char* argv[]) {
	// TODO define region of interest --> TODO: select graphical -or- detect by optical flow
	
	ProgramOptions cmdLineOpts(argc, argv, "i:r:v:w:");
	// Config config(ProgramOptions(argc, argv, "i:r:v:w:"), "video.sqlite")
	Config config("video.sqlite");
	Config* pConfig = &config;

	FrameHandler frameHandler(pConfig); // frame reading, segmentation, drawing fcn
	FrameHandler* pFrameHandler = &frameHandler;
	config.attach(pFrameHandler);

	SceneTracker scene(pConfig); // collection of tracks and vehicles with MOG2
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	CountRecorder recorder; // counting vehicles and save results in db
	CountRecorder* pRecorder = &recorder;
	scene.attachCountRecorder(pRecorder);

	CountResults cr;


	list<TrackEntry> bboxList; // debug only, delete after
	list<Track> trackList;


	/* TODO un-comment after testing camera
	 * if (!frameHandler.openCapSource(true)) 
	 *	return -1;
	 */

	bool succ = frameHandler.openCapSource(false);
	

	while(true)
	{
		
		if (!frameHandler.segmentFrame())
			break;

		int type = frameHandler.getFrameInfo();
		
		bboxList = frameHandler.calcBBoxes();

		trackList = scene.updateTracks(bboxList);

		//vehicleList = scene.combineTracks();

		scene.inspect(frameHandler.getFrameCount());

		cr += scene.countVehicles(frameHandler.getFrameCount());
		
		//CountResults cr = recorder.getStatus();

		frameHandler.showFrame(trackList, cr);

		//frameHandler.writeFrame();


	
		
		StopStartNextFrame();
		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}

	recorder.printResults();

	cv::waitKey(0);
	return 0;
}



