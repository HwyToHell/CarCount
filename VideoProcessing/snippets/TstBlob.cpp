#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;
//using namespace cv;

bool fileExists (const string& name) 
{
    ifstream f(name.c_str(), ifstream::in);
    if (f.good()) 
	{
        f.close();
        return true;
    } 
	else
	{
        f.close();
        return false;
    }   
}

void printParams_(cv::Ptr<cv::SimpleBlobDetector> detector) // works only for SimpleBlobDetector
{
	vector<string> parNames;
	detector->getParams(parNames);
	for (unsigned int i = 0; i < parNames.size(); i++)
	{
		double d = detector->get<double>(parNames[i]);
		std::cout << parNames[i] << ": " << d << std::endl;
	}
}

void printParams(cv::Ptr<cv::Algorithm> algorithm) // Generic version, works for all algorithms
{
	vector<string> parNames;
	algorithm->getParams(parNames);
	for (unsigned int i = 0; i < parNames.size(); i++)
	{
		double d = algorithm->get<double>(parNames[i]);
		std::cout << parNames[i] << ": " << d << std::endl;
	}
}


bool writeParamsToFile(cv::Ptr<cv::SimpleBlobDetector> detector, std::string file)
{
	cv::FileStorage fs;
	fs.open(file, cv::FileStorage::WRITE);
	if (fs.isOpened())
		detector->write(fs);
	else
		return false;
	fs.release();
	return true;
}

cv::Scalar green = cv::Scalar(0,255,0);
cv::Scalar red = cv::Scalar(0,0,255);

int blob_main(int argc, char* argv[])
{
	string filename = "D:\\PerfTest\\2015-6-13--18h-05m-56s_Tresh=15\\f220.jpg";
	cv::Mat frame_grey;
	if (fileExists(filename))
	{
		frame_grey = cv::imread(filename, 0); // load as one channel image
		if(frame_grey.data)
			cv::imshow("Mask", frame_grey);
		else
			cout << "Could not open or find the image" << endl ;
	}
	cv::Mat frame;
	cv::threshold(frame_grey, frame, 127, 255, cv::THRESH_BINARY); // thresholding for removing compression artefacts

	cv::SimpleBlobDetector::Params params;
	params.minArea = 25.;
	params.minDistBetweenBlobs = 10.;

	vector<cv::KeyPoint> keyPoints;
	cv::Ptr<cv::SimpleBlobDetector> blobDet = cv::SimpleBlobDetector::create("SimpleBlob");
	// Initialize Params
	blobDet->set("blobColor", 255);
	blobDet->set("minDistBetweenBlobs", 10);
	blobDet->setBool("filterByInertia", false);
	blobDet->setBool("filterByConvexity", false);
	printParams(blobDet);
	writeParamsToFile(blobDet, "D:\\PerfTest\\Params.xml");
	blobDet->detect(frame_grey, keyPoints);
	cv::Mat outImage;
	cv::drawKeypoints(frame_grey, keyPoints, outImage, red, cv::DrawMatchesFlags::DEFAULT);
//	cv::imshow("Blobs", outImage);

	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	//cv::cvtColor(frame_grey, frame_bgr, cv::COLOR_GRAY2BGR);
	cv::findContours(frame, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		cv::drawContours(outImage, contours, i, cv::Scalar(0,0,255), 1, 8, hierarchy, 0, cv::Point() );
		cv::Rect bbox = cv::boundingRect(contours[i]);
		cv::rectangle(outImage, bbox, green, 1, 8, 0);
		cv::Point textPos(bbox.x, bbox.y);

		cv::putText(outImage, to_string((long long) bbox.area()), textPos, 0, 0.5, green, 1);
		cv::imshow("Contours", outImage);
		cout << "Area contour " << i << ": " << cv::contourArea(contours[i]) << endl;
		cout << "Area bbox " << i  << ": " << bbox.area() << endl;
		if (cv::waitKey(0) == 27)
			break;
		
	}
	return 0;
}
