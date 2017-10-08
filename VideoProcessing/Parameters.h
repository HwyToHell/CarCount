#include "stdafx.h"

double round(double number); // not necessary in C++11

class Parameter {
public:
	Parameter(std::string _name = "", double _actual = 0, double _min = 0, double _max = 0, int _steps = 0);
	double GetFromSlider();
	double GetActual();
	static void OnSlider(int pos, void* usr_par);
	void write(cv::FileStorage& fs) const;
	void read(const cv::FileNode& node);
	std::string name;
	int iSliderPos;
	int iSteps;
	double actual;
	double min;
	double max;

private:
	//double min;
	//double max;


	// ToDo: memberfcn for transformation double GetActualFromSlider() --> sets actual and returns this value
};

struct MOG2 { // MOG2 Parameters
	static cv::BackgroundSubtractorMOG2 bs;
	std::vector<Parameter> parList;
	void initPar();
	bool fromFile(std::string file);
	bool toFile(std::string file);
	bool createTrackbars();
	static void OnChange(int pos, void* usr_par);
	static bool updateAlgorithm(std::string parameter, double value);
	void MOG2::updateAll();
};

class SaveSamples {
public:
	SaveSamples(cv::Mat *frame, cv::Mat *mask);
	int count;
	std::vector<int> samples;
	std::string CreateDir();
	bool CheckForSave();
private:
	cv::Mat *frame, *mask;
	std::string root;
	std::string dir;
};

// why do I need to define the two following functions in the header file? --> because of static binding = local to source file
// does not work with declaration in header and definition in source
static void write(cv::FileStorage& fs, const std::string&, const Parameter& x)
{
	x.write(fs);
}

static void read(const cv::FileNode& node, Parameter& x, const Parameter& default_value = Parameter())
{
	if(node.empty())
		x = default_value;
	else
		x.read(node);
}