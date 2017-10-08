#include "Parameters.h"

using namespace std;


ostream& operator<<(ostream& out, MOG2 par)
{
	for (unsigned int i = 0; i < par.parList.size(); i++)
		out << par.parList[i].name << ": " << par.parList[i].actual  << endl;
	return out; 
}


bool CreateTimeStampDir(string& dir)
{
	// date and time
	time_t t;
	time(&t);
	struct tm* tInfo;
	tInfo = localtime(&t); 
	stringstream stream;
	stream << "D:\\PerfTest\\";
	string root = stream.str();
	if (_mkdir(root.c_str()) != 0)
		if (errno != EEXIST)
			return false;
	stream << tInfo->tm_year + 1900 << "-" << tInfo->tm_mon << "-" << tInfo->tm_mday << "--";
	stream << tInfo->tm_hour << "h-" << setfill('0') << setw(2) << tInfo->tm_min << "m-" << setfill('0') << setw(2) << tInfo->tm_sec << "s";
	dir = stream.str();
	//printf(dir.c_str());
	if (_mkdir(dir.c_str()) == 0)
		return true;
	else
		return false;
}

int ser_main(int,char**)
{
	cv::namedWindow("Params", cv::WINDOW_AUTOSIZE);
	cv::Mat frame;
	frame = cv::imread("frame.jpg");
	SaveSamples ss(&frame, &frame);


	//const std::vector<int> ints = {10,20,30};

/*	// serialization
	MOG2 pM;
	pM.initPar();
	cout << pM;
	pM.fromFile("params.xml");
	cout << pM << endl;
	pM.createTrackbars();
*/
	// date and time
	time_t t;
	time(&t);
	cout << ctime(&t);
	struct tm* tInfo;
	tInfo = localtime(&t); 
	cout << "year: " << 1900 + tInfo->tm_year << endl;
	cout << "month: " << 1 + tInfo->tm_mon << endl;
	cout << "day: " << tInfo->tm_mday << endl;
	string perfDir;
	cout << boolalpha << CreateTimeStampDir(perfDir) << endl;
	cout << perfDir; 

	ss.CreateDir();
	for (int i = 0; i < 200; i++)
		ss.CheckForSave();

	cv::waitKey(0); // at least one highgui window has to be created for this function to work
	return 0;
}