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
	
	// command line arguments
	//  i(nput):		cam ID (single digit number) or file name,
	//					if empty take standard cam
	//  q(uiet)			quiet mode (take standard arguments from config file
	//  r(ate):			cam fps
	//  v(ideo size):	cam resolution ID (single digit number)

	ProgramOptions cmdLineOpts(argc, argv, "i:qr:v:");
	Config config;
	Config* pConfig = &config;

	bool succ = config.readCmdLine(cmdLineOpts);
	if (!succ)
		return (EXIT_FAILURE);

	FrameHandler frameHandler(pConfig); // frame reading, segmentation, drawing fcn
	FrameHandler* pFrameHandler = &frameHandler;
	config.attach(pFrameHandler);

	// capSource must be open in order to set frame size
	succ = frameHandler.openCapSource(false);
	
	// recalcFrameSizeDependentParams, if different frame size
	int widthNew = static_cast<int>(frameHandler.getFrameSize().width);
	int heightNew = static_cast<int>(frameHandler.getFrameSize().height);
	int widthActual = stoi(config.getParam("frame_size_x"));
	int heightActual = stoi(config.getParam("frame_size_y"));
	if ( widthNew != widthActual || heightNew != heightActual ) 
		 config.adjustFrameSizeDependentParams(widthNew, heightNew);


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

	config.locateVideoFile("traffic.avi");
	

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



