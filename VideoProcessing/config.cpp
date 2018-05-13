#include "stdafx.h"
#include "../include/config.h"

#include <cctype> // isdigit()
#include <io.h> // _access()

#pragma warning(disable: 4996)

using namespace std;

// observer.h
void updateObserver(Observer* pObserver) {
	pObserver->update();
}

Parameter::Parameter(std::string name, std::string type, std::string value) : mName(name), mType(type), mValue(value) {}

std::string Parameter::getName() const { return mName; }

std::string Parameter::getType() const { return mType; }

std::string Parameter::getValue() const { return mValue; }

bool Parameter::setValue(std::string& value) {
	mValue = value;
	return true;
}

class ParamEquals : public unary_function<Parameter, bool> {
	std::string mName;
public: 
	ParamEquals (const std::string& name) : mName(name) {}
	bool operator() (const Parameter& par) const { 
		return (mName == par.getName());
	}
};

//////////////////////////////////////////////////////////////////////////////
// Config ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Config::Config() :	m_appPath(std::string()),
					m_configFilePath(std::string()),
					m_configTableName(std::string()),
					m_dbHandle(nullptr),
					m_quiet(false),
					m_paramList(std::list<Parameter>()),
					m_videoFilePath(std::string()) {
	init();
}

Config::~Config() {
	if (m_dbHandle) {
		int rc = sqlite3_close(m_dbHandle);
		if (rc != SQLITE_OK)
			cerr << "Config destructor: data base was not closed correctly" << endl;
	}
}


void Config::adjustFrameSizeDependentParams(int new_size_x, int new_size_y) {
	int old_size_x = stoi(getParam("frame_size_x"));
	int old_size_y = stoi(getParam("frame_size_y"));

	try {
		// roi
		long long roi_x = stoi(getParam("roi_x")) * new_size_x / old_size_x;
		long long roi_width =  stoi(getParam("roi_x")) * new_size_x / old_size_x;
		long long roi_y = stoi(getParam("roi_y")) * new_size_y / old_size_y;
		long long roi_height = stoi(getParam("roi_y")) * new_size_y / old_size_y;	
		setParam("roi_x", to_string(roi_x));
		setParam("roi_width", to_string(roi_width));
		setParam("roi_y", to_string(roi_y));
		setParam("roi_height", to_string(roi_height));

		// blob assignment
		long long blob_area_min =  stoi(getParam("blob_area_min")) * new_size_x * new_size_y 
			/ old_size_x / old_size_y;
		long long blob_area_max =  stoi(getParam("blob_area_max")) * new_size_x * new_size_y 
			/ old_size_x / old_size_y;
		setParam("blob_area_min", to_string(blob_area_min));
		setParam("blob_area_max", to_string(blob_area_max));

		// track assignment
		long long track_max_dist =  stoi(getParam("track_max_distance")) * (new_size_x / old_size_x
			+ new_size_y / old_size_y) / 2;
		setParam("track_max_distance", to_string(track_max_dist));

		// count pos, count track length
		long long count_pos_x =  stoi(getParam("count_pos_x")) * new_size_x / old_size_x;
		setParam("count_pos_x", to_string(count_pos_x));
		long long count_track_length =  stoi(getParam("count_track_length")) * new_size_x / old_size_x;
		setParam("count_track_length", to_string(count_track_length));

		// truck_size
		long long truck_width_min =  stoi(getParam("truck_width_min")) * new_size_x / old_size_x;
		setParam("truck_width_min", to_string(truck_width_min));

		// TODO next line "invalid argument exception"
		long long truck_height_min =  stoi(getParam("truck_height_min")) * new_size_y / old_size_y;
		setParam("truck_height_min", to_string(truck_height_min));

		// save new frame size in config
		setParam("frame_size_x", to_string((long long)new_size_x));
		setParam("frame_size_y", to_string((long long)new_size_y));
	}
	catch (exception& e) {
		cerr << "invalid parameter in config: " << e.what() << endl;
	}
}


std::string Config::getParam(std::string name) {
	using namespace	std;
	list<Parameter>::iterator iParam = find_if(m_paramList.begin(), m_paramList.end(), ParamEquals(name));
	if (iParam == m_paramList.end()) {
		cerr << "getParam: parameter not in config: " << name <<endl;
		return string("");
	}
	else {
		return (iParam->getValue());
	}
}

bool Config::init() {
	using namespace std;
	// create parameter list with standard values
	// must be done before setAppPath, as m_appPath is saved in parameter list
	if (!populateStdParams()) {
		cerr << "init: cannot create parameter list" << endl;
		return false;
	}

	// set application path underneath /home
	if (!setAppPath("counter")) {
		cerr << "init: cannot create application path" << endl;
		return false;
	}

	// set path to config file and table name
	setConfigProps(m_appPath, "config");
		
	return true;
}


bool Config::insertParam(Parameter param) {
	m_paramList.push_back(param);
	return true;
}


bool Config::loadParamsFromDb() {
	using namespace	std;
	
	// check valid object state
	if (!m_dbHandle) {
		cerr << "loadParamsFromDb: data base not open yet" << endl;
		return false;
	}
	if (m_paramList.empty()) {
		cerr << "loadParamsFromDb: parameter list not initialized yet" << endl;
		return false;
	}
	if (m_configTableName.empty()) {
		cerr << "loadParamsFromDb: db table not specified" << endl;
		return false;
	}

	bool success = false;
	stringstream ss;
	string sqlCreate, sqlRead, sqlinsert;
	string noRet = "";
	
	// check, if table exists
	ss.str("");
	ss << "create table if not exists " << m_configTableName << " (name text, type text, value text);";
	sqlCreate = ss.str();
	success = queryDbSingle(sqlCreate, noRet);


	list<Parameter>::iterator iParam = m_paramList.begin();
	while (iParam != m_paramList.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from " << m_configTableName << " where name=" << "'" << iParam->getName() << "';"; 
		sqlRead = ss.str();
		string strValue;
		success = queryDbSingle(sqlRead, strValue);

		if (success && !strValue.empty()) // use parameter value from db
			iParam->setValue(strValue);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into " << m_configTableName << " values ('" << iParam->getName() << "', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
			sqlinsert = ss.str();
			strValue = iParam->getValue();
			success =  queryDbSingle(sqlinsert, noRet);
		}
		++iParam;
	}

	return success;
}


// locate video file, save full path to m_videoFilePath
// search order: 1. current directory, 2. /home, 3. /home/work_dir
bool Config::locateVideoFile(std::string fileName) {
	using namespace std;

	if (m_homePath.empty()) {
		cerr << "locateVideoFile: home path not set yet" << endl;
		return false;
	}
	m_videoFilePath.clear();

	// 1. check current_dir
	string currentPath;
	#if defined (_WIN32)
		char bufPath[_MAX_PATH];
		_getcwd(bufPath, _MAX_PATH);
		if (bufPath) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (GetLastError() == ERANGE)
				cerr << "locateVideoFile: path length > _MAX_PATH" << endl;
			return false;
		}
	#elif defined (__linux__)
		char bufPath[PATH_MAX];
		getcwd(bufPath, PATH_MAX);
		if (bufPath) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (errno == ENAMETOOLONG)
				cerr << "locateVideoFile: path length > PATH_MAX" << endl;
			return false;
		}
	#else
		throw "unsupported OS";
	#endif
	string filePath = currentPath;
	appendDirToPath(filePath, fileName);
	if (isFileExist(filePath)) {
		m_videoFilePath = filePath;
		return true;
	}

	// 2. check /home
	filePath = m_homePath;
	appendDirToPath(filePath, fileName);
	if (isFileExist(filePath)) {
		m_videoFilePath = filePath;
		return true;
	}

	// 3. check /home/app_dir
	filePath = m_appPath;
	appendDirToPath(filePath, fileName);
	if (isFileExist(filePath)) {
		m_videoFilePath = filePath;
		return true;
	}

	// file not found
	return false;
}


bool Config::populateStdParams() {
	m_paramList.clear();

	/* TODO set standard parameters */
	// TODO parameter for capFileName
	// capFile, capDevice, framesize, framerate
	m_paramList.push_back(Parameter("application_path", "string", "D:\\Users\\Holger"));
	m_paramList.push_back(Parameter("video_file_path", "string", "D:\\Users\\Holger\\counter\\traffic640x480.avi"));
	m_paramList.push_back(Parameter("is_video_from_cam", "bool", "false"));

	m_paramList.push_back(Parameter("cam_device_ID", "int", "0"));
	m_paramList.push_back(Parameter("cam_resolution_ID", "int", "0"));
	m_paramList.push_back(Parameter("cam_fps", "int", "10"));

	m_paramList.push_back(Parameter("frame_size_x", "int", "320"));
	m_paramList.push_back(Parameter("frame_size_y", "int", "240"));
	// region of interest
	m_paramList.push_back(Parameter("roi_x", "int", "80"));
	m_paramList.push_back(Parameter("roi_y", "int", "80"));
	m_paramList.push_back(Parameter("roi_width", "int", "180"));
	m_paramList.push_back(Parameter("roi_height", "int", "80"));
	// blob assignment
	m_paramList.push_back(Parameter("blob_area_min", "int", "200"));
	m_paramList.push_back(Parameter("blob_area_max", "int", "20000"));
	// track assignment
	m_paramList.push_back(Parameter("track_max_confidence", "int", "4"));
	m_paramList.push_back(Parameter("track_max_deviation", "double", "80"));
	m_paramList.push_back(Parameter("track_max_distance", "double", "30"));
	// scene tracker
	m_paramList.push_back(Parameter("max_n_of_tracks", "int", "9")); // maxNoIDs
	// counting
	m_paramList.push_back(Parameter("count_confidence", "int", "3"));
	m_paramList.push_back(Parameter("count_pos_x", "int", "90")); // counting position (within roi)
	m_paramList.push_back(Parameter("count_track_length", "int", "20")); // min track length for counting
	// classification
	m_paramList.push_back(Parameter("truck_width_min", "int", "60")); // classified as truck, if larger
	m_paramList.push_back(Parameter("truck_height_min", "int", "28")); // classified as truck, if larger
	return true;
}


bool Config::queryDbSingle(const std::string& sql, std::string& value) {
	bool success = false;
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare_v2(m_dbHandle, sql.c_str(), -1, &stmt, 0);
	if (rc == SQLITE_OK)
	{
		int step = SQLITE_ERROR;
		int nRow = 0; 
		do 
		{
			step = sqlite3_step(stmt);
			
			switch (step) {
			case SQLITE_ROW: 
				{
					// one result expected: take first row only and discard others
					if (nRow == 0)
					{
						int nCol = sqlite3_column_count(stmt);
						nCol = 0; // one result expected: take first column only
						if (sqlite3_column_type(stmt, nCol) == SQLITE_NULL) 
							cerr << __LINE__ << " NULL value in table" << endl;
						else
							value = (const char*)sqlite3_column_text(stmt, nCol);
					}
				}
				break;
			case SQLITE_DONE: break;
			default: 
				cerr << __LINE__ << "Error executing step statement" << endl;
				break;
			}
			++nRow;
		} while (step != SQLITE_DONE);
		
		rc = sqlite3_finalize(stmt);			
		if (rc == SQLITE_OK)
			success = true;
		else
			success = false;
	} 

	else // sqlite3_prepare != OK
	{
		cerr << "SQL error: " << sqlite3_errmsg(m_dbHandle) << endl;
		rc = sqlite3_finalize(stmt);
		success = false;
	}
	return success;
}


// command line arguments
//  i(nput):		cam ID (single digit number) or file name,
//					if empty take standard cam
//  q(uiet)			quiet mode (take standard arguments from config file
//  r(ate):			cam fps
//  v(ideo size):	cam resolution ID (single digit number)
// only lexical checks are performed in this function
// physical test are done in other submodules, e.g. file existence will be checked 
bool Config::readCmdLine(ProgramOptions programOptions) {
	using namespace	std;
	bool isVideoSourceValid = false;

	// 1 lexical checks, argument by argument
	// i: video input from cam or file
	if (programOptions.exists('i')) { 
		string videoInput = programOptions.getOptArg('i');
		// check if single digit -> cam
		// check if file exists -> error message
		if (videoInput.size() == 0) // use std cam
			videoInput.append("0");
		
		if (videoInput.size() == 1) { // cam ID
			string camID = videoInput.substr(0, 1);
			
			if (isdigit(camID.at(0))) { // cam ID must be int digit
				setParam("cam_device_ID", camID);
				setParam("is_video_from_cam", "true");
				isVideoSourceValid = true;
			}
			else {
				cerr << "invalid argument: camera ID option '-i " << camID << "'" << endl;
				cerr << "camera ID must be integer within range 0 ... 9" << endl;

				return false;
			}
		}

		else { // video file name
			setParam("video_file", videoInput);
			setParam("is_video_from_cam", "false");
			isVideoSourceValid = true;
		}
	} // end if (programOptions.exists('i'))

	//  q: quiet mode = take standard arguments, don't query for user input
	if (programOptions.exists('q')) {
		m_quiet = true;
	} else { 
		m_quiet = false;
	}

	// r: frame rate in fps
	if (programOptions.exists('r')) {
		string frameRate = programOptions.getOptArg('r');
		if (isInteger(frameRate)) {
			setParam("cam_fps", frameRate);
		}
		else {
			cerr << "invalid argument: frame rate option '-r " << frameRate << "'" << endl;
			cerr << "frame rate must be integer" << endl;
			return false;
		}
	} // end if (programOptions.exists('r'))

	// TODO change lexical check for single digit
	// v: video size
	if (programOptions.exists('v')) {
		string resolutionID = programOptions.getOptArg('v');
		// check if single digit
		// check if file exists -> error message
		if (resolutionID.size() == 1)  { // OK, must be one digit
			if (isdigit(resolutionID.at(0))) { // OK, must be int digit 
				setParam("cam_resolution_ID", resolutionID);
			} else {
				cerr << "invalid argument: resolution ID option '-v " << resolutionID << "'" << endl;
				cerr << "resolution ID must be integer within range 0 ... 9" << endl;
				return false;
			}
		} else {
				cerr << "invalid argument: resolution ID option '-v " << resolutionID << "'" << endl;
				cerr << "resolution ID must be integer within range 0 ... 9" << endl;
				return false;
		}
		return true;
	} // end if (programOptions.exists('v'))

	// not quiet, no input specified -> printHelp, exit
	if (!isVideoSourceValid && !m_quiet) {
		printCommandOptions();
		return false;
	}
	
	// 2 save config
	// TODO implementation


	return true;
}

// config file must reside in m_appPath
bool Config::readConfigFile(std::string configFile) {
	using namespace std;

	if (configFile.empty()) {
		cerr << "readConfigFile: no filename specified" << endl;
		return false;
	}

	// db already open?
	if (!m_dbHandle) {
		int rc = sqlite3_open(configFile.c_str(), &m_dbHandle);
		if (rc != SQLITE_OK) {
			cerr << "readConfigFile: error opening " << configFile << endl;
			cerr << "message: " << sqlite3_errmsg(m_dbHandle) << endl;
			sqlite3_close(m_dbHandle);
			m_dbHandle = nullptr;
			return false;
		}
	}

	// read all parameters from config db file
	if (loadParamsFromDb())
		return true;
	else
		return false;
}


bool Config::saveConfigToFile() {
	using namespace std;
	// validate object state
	if (!m_dbHandle) {
		cerr << "saveConfigToFile: data base not open yet" << endl;
		return false;
	}
	if (m_configTableName.empty()) {
		cerr << "saveConfigToFile: db table not specified" << endl;
		return false;
	}

	// create table, if not exist
	stringstream ss;
	ss << "create table if not exists " << m_configTableName << " (name text, type text, value text);";
	string sqlStmt = ss.str();
	string answer;
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "saveConfigToFile: db table does not exist, cannot create it" << endl;
		return false;
	}

	// TODO check, if parameter exists in db
	// if it does not exist -> insert
	// otherwise            -> update

	// read all parameters and insert into db
	bool success = false;
	list<Parameter>::iterator iParam = m_paramList.begin();
	while (iParam != m_paramList.end())	{
		ss.str("");
		ss << "select 1 from " << m_configTableName << "where name='" << iParam->getName() << "';";
		string sqlStmt = ss.str();
		success = queryDbSingle(sqlStmt, answer);

		// check, if parameter row exists in db
		ss.str("");
		if (stob(answer)) {// row does exist -> update
			ss << "update " << m_configTableName << " set value='" << iParam->getValue() << "' " 
				<< "where name='" << iParam->getName() << "';"; 
		} else { // row does not exist -> insert
			ss << "insert into " << m_configTableName << " values ('" << iParam->getName() << 
				"', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
		}
		sqlStmt = ss.str();
		success = queryDbSingle(sqlStmt, answer);
	}

	return success;
}



// set path to application directory 
// create, if it does not exist
bool Config::setAppPath(std::string appDir) {
	using namespace std;
	m_appPath = m_homePath = getHomePath();
	
	if (m_homePath.empty()) {
		cerr << "setAppPath: no home path set" << endl;
		return false;
	} 
	
	// set app path and save in parameter list
	appendDirToPath(m_appPath, appDir);
	if (!setParam("application_path", m_appPath)) {
		cerr << "setAppPath: cannot save application path in parameter list" << endl;
		return false;
	}

	// if appPath does not exist, create it
	if (!isFileExist(m_appPath)) {
		if (!makePath(m_appPath)) {
			cerr << "setAppPath: cannot create path: " << m_appPath << endl;
			return false;
		}
	}

	return true;
}	

// set path to config file and table name within config database
void Config::setConfigProps(std::string configDirPath, std::string configFileName, std::string configTable) {
	// set name and location of config file
	// note: existence of config file is checked in readConfigFile()
	m_configFilePath = configDirPath;
	appendDirToPath(m_configFilePath, configFileName);

	// table name for config data
	m_configTableName = configTable;
	return;
}


bool Config::setParam(std::string name, std::string value) {
	list<Parameter>::iterator iParam = find_if(m_paramList.begin(), m_paramList.end(),
		ParamEquals(name));
	if (iParam == m_paramList.end())
		return false;
	iParam->setValue(value);
	return true;
}


// Directory manipulation functions
std::string& appendDirToPath(std::string& path, const std::string& dir) {
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw "unsupported OS";
	#endif
	
	// append delimiter, if missing 
	if (path.back() != delim && dir.front() != delim)
		path += delim;
	path += dir;

	return path;
}


std::string getHomePath() {
	using namespace	std;
	std::string home(std::string(""));
	
	#if defined (_WIN32)
		char *pHomeDrive = nullptr, *pHomePath = nullptr;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive == nullptr) {
			cerr << "getHomePath: no home drive set in $env" << endl;
			std::string("");
		} else {
			home += pHomeDrive;
		}
		
		pHomePath = getenv("HOMEPATH");
		if (pHomePath == nullptr) {
			cerr << "getHomePath: no home path set in $env" << endl;
			return std::string("");
		} else {
			home = appendDirToPath(home, std::string(pHomePath));
		}

	#elif defined (__linux__)
		char *pHome = nullptr;
		pHomePath = getenv("HOME");
		if (pHome ==0) {
			cerr << "getHomePath: no home path set in $env" << endl;
			return std::string("");
		} else {
			home += pHome;
			home += '/';
		}
	#else
		throw "unsupported OS";
	#endif
	
	return home;
}


bool makeDir(const std::string& dir) {
	#if defined (_WIN32)
		std::wstring wDir;
		wDir.assign(dir.begin(), dir.end());

		if (CreateDirectory(wDir.c_str(), 0))
			return true;
		DWORD error = GetLastError();
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			return true;
		else 
			return false;
	#elif defined (__linux__)
		// TODO use mkdir from <unistd.h>
		throw "linux";
	#else
		throw "unsupported OS";
	#endif
}


// make full path
bool makePath(std::string path) {
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw "unsupported OS";
	#endif
	bool success = true;
	size_t pos = 0;

	// path must end with delim character
	if (path.back() != delim)
		path += delim;

	pos = path.find_first_of(delim);
	while (success == true && pos != std::string::npos)
	{
		if (pos != 0)
			success = makeDir(path.substr(0, pos));
		++pos;
		pos = path.find_first_of(delim, pos);
	}
	return success;
}


bool isFileExist(const std::string& path) {
	#if defined (_WIN32)
	if (_access(path.c_str(), 0) == 0) {
		return true;
	} else {
		return false;
		// errno: EACCES == acess denied
		//        ENOENT == file name or path not found
		//        EINVAL == invalid parameter
	}
	#elif defined (__linux__)
		// TODO use stat from <sys/stat.h>
		throw "linux";
	#else
		throw "unsupported OS";
	#endif
}


// String conversion functions
bool isInteger(const std::string& str) {
	std::string::const_iterator iString = str.begin();
	while (iString != str.end() && isdigit(*iString))
		++iString;
	return !str.empty() && iString == str.end();
}


bool stob(std::string str) {
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	if (str == "true")
		return true;
	else if (str == "false")
		return false;
	else
		throw "bad bool string";
}

// command line arguments
//  i(nput):		cam ID (single digit number) or file name,
//					if empty take standard cam
//  q(uiet)			quiet mode (take standard arguments from config file
//  r(ate):			cam fps
//  v(ideo size):	cam resolution ID (single digit number)
void printCommandOptions() {
	using namespace std;
	cout << "usage: video [options]" << endl;
	cout << endl;
	cout << "options:" << endl;
	cout << "-i                 specify video input, either" << endl;
	cout << "-i cam_ID            camera -or-" << endl;
	cout << "-i file              file" << endl;
	cout << "-q                 quiet mode (yes to all questions)" << endl;
	cout << "-r fps             set camera frame rate" << endl;
	cout << "-v resolution_ID   set camera resolution ID" << endl; 
	return;
}
