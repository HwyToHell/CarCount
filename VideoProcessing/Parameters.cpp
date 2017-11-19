#include "stdafx.h"
#include "../include/Parameters.h"


double round(double number) // not necessary in C++11
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

// ToDo: Constructor - init min, max, actual (w/ std value), set sliderPos, throw exeption for out of range 
Parameter::Parameter(std::string _name, double _actual, double _min, double _max, int _steps)
{
	name = _name;  // alternative: this->name = _name;
	min = _min;
	max = _max;
	iSteps = _steps;
	// ToDo: check for range violation, throw(std::range_error)
	actual = (_actual < min) ? min : _actual;
	actual = (actual > max) ? max : actual;
	iSliderPos = (int) round ((actual - min) / (max - min) * iSteps);
}


void Parameter::OnSlider(int pos, void* usr_par) // static fcn!
{
	Parameter* pPar = (Parameter*)usr_par;
	pPar->actual = (pPar->min + (pPar->max - pPar->min) * pos / pPar->iSteps);
	//std::cout << "actual: " << pPar->actual << std::endl;
	//actual = (min + (max - min) * pos / iSteps);
}

void Parameter::write(cv::FileStorage& file) const
{
	file << "{" << name << actual << "}";
}
	
void Parameter::read(const cv::FileNode& node)
{
	actual = (double)node[name];
}

cv::BackgroundSubtractorMOG2 MOG2::bs = cv::BackgroundSubtractorMOG2(); // definition of static member variable

void MOG2::initPar()
{
	parList.push_back(Parameter("mHistory", 100, 50, 500, 9)); // std: 500 
	parList.push_back(Parameter("varThreshold", 15, 10, 100, 18)); // std: 16 --> use 25
	parList.push_back(Parameter("nmixtures", 3, 3, 7, 4)); // std: 5
	parList.push_back(Parameter("backgroundRatio", 0.9, 0.5, 1, 5)); // std: 0.9
	parList.push_back(Parameter("varThresholdGen", 9, 5, 21, 8)); // std: 9
	//parList.push_back(Parameter("dVarInit;
	//parList.push_back(Parameter("dVarMin;
	//parList.push_back(Parameter("dVarMax;
	parList.push_back(Parameter("fCT", 0.05, 0, 0.1, 2)); // std: 0.05
	parList.push_back(Parameter("fTau", 0.5, 0.3, 0.7, 4)); // std: 0.5	
	parList.push_back(Parameter("detectShadows", 0, 0, 1, 1)); // std: 0	
}

bool MOG2::fromFile(std::string file)
{
	cv::FileStorage fs;
	fs.open(file, cv::FileStorage::READ);
	if (fs.isOpened())
		for (unsigned int i = 0; i < parList.size(); i++)
			fs[parList[i].name] >> parList[i].actual;
	else
		return false;
	fs.release();
	return true;
}

bool MOG2::toFile(std::string file)
{
	cv::FileStorage fs;
	fs.open(file, cv::FileStorage::WRITE);
	if (fs.isOpened())
		bs.write(fs);
	else
		return false;
	fs.release();
	return true;
}

bool MOG2::createTrackbars()
{
	cv::namedWindow("Params", cv::WINDOW_NORMAL);
	for (unsigned int i = 0; i < parList.size(); i++)
		cv::createTrackbar(parList[i].name, "Params", &parList[i].iSliderPos, parList[i].iSteps, this->OnChange, &parList[i]); 
	return true;
}	

void MOG2::OnChange(int pos, void* usr_par) // static fcn!
{
	Parameter* pPar = (Parameter*)usr_par;
	pPar->actual = (pPar->min + (pPar->max - pPar->min) * pos / pPar->iSteps);
	std::cout << pPar->name << ": " << pPar->actual << std::endl;
	updateAlgorithm(pPar->name, pPar->actual);
}

bool MOG2::updateAlgorithm(std::string parameter, double value) // static fcn!
{
	//ToDo: implement set params algorithm
	const char *cstr = parameter.c_str();
	bs.set(cstr, value);
	//std::cout << "Parameter to set: " << value << std::endl;
	//std::cout << "Reading back: " << bs.get<double>(cstr) << std::endl;
	return true;
}

void MOG2::updateAll()
{
	for (unsigned int i = 0; i < parList.size(); i++)
	{
		updateAlgorithm(parList[i].name, parList[i].actual);
		std::cout << parList[i].name << ": " << parList[i].actual << std::endl;
	}
}

SaveSamples::SaveSamples(cv::Mat *frame, cv::Mat *mask) : count(0), root("D:\\PerfTest\\")
{
	//const int iArr[] = {90, 105, 140, 185, 210, 490, 520, 820, 850, 910, 930, 1430, 1450, 1480, 1990};
	const int iArr[] = {120, 180, 200, 220, 830, 850, 870, 1950, 1970, 1990};
	for (int i = 0; i < sizeof(iArr) / sizeof(iArr[0]); i++) 
		samples.push_back(iArr[i]);

	this->frame = frame;
	this->mask = mask;
}

bool SaveSamples::CheckForSave()
{
	count++;
	for (unsigned int i = 0; i < samples.size(); i++)
		if (samples[i] == count)
		{
			std::stringstream stream;
			stream << dir << "\\f" << count << ".jpg";
			//std::cout << "frame: " << stream.str() << std::endl;;
			cv::imwrite(stream.str(), *frame);
			std::stringstream stream2;
			stream2 << dir << "\\m" << count << ".jpg";
			//std::cout << "mask: " << stream2.str() << std::endl;
			cv::imwrite(stream2.str(), *mask);
			return true;
		}
	return false;
}

std::string SaveSamples::CreateDir()
{
	time_t t;
	time(&t);
	struct tm* tInfo;
	tInfo = localtime(&t); 
	std::stringstream stream;
	stream << root;
	if (_mkdir(root.c_str()) != 0)
		if (errno != EEXIST)
			return "";
	stream << tInfo->tm_year + 1900 << "-" << tInfo->tm_mon + 1 << "-" << tInfo->tm_mday << "--";
	stream << tInfo->tm_hour << "h-" << std::setfill('0') << std::setw(2);
	stream << tInfo->tm_min << "m-" << std::setfill('0') << std::setw(2) << tInfo->tm_sec << "s";
	dir = stream.str();
	//printf(dir.c_str());
	if (_mkdir(dir.c_str()) == 0)
		return dir;
	else
		return "";
}
