#include "stdafx.h"

int tst_main(int argc, char* argv[]) {
    using namespace std;
    std::string videoFile("/home/holger/counter/traffic320x240.avi");
    cv::VideoCapture cap(videoFile);
    //cap.open("/home/holger/counter/traffic320x240.avi")
    cv::Mat frame;
    cv::Size frameSize(0,0);
    if (cap.isOpened()) {
        if (!cap.read(frame)) {
            cerr << "cannot read first frame" << endl;
            return EXIT_FAILURE;
        }

        frameSize.width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
        frameSize.height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
        int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
        int pos = cap.get(CV_CAP_PROP_POS_MSEC);
        cout << "witdth: " << width << " msec: " << pos << endl;
        cout << "start video processing" << endl;
        while (cap.read(frame)) {
            int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
            int pos = cap.get(CV_CAP_PROP_POS_MSEC);
            cout << "witdth: " << width << " msec: " << pos << endl;
            cv::imshow(videoFile, frame);
            if (cv::waitKey(10) == 27) 	{
                std::cout << "ESC pressed -> end video processing" << std::endl;
                break;
            }
        }
    } else {
        std::cerr << "cap not opened" << std::endl;
    }
    std::cout << "finished video processing" << endl;

    cv::waitKey(0);
    return 0;
}
