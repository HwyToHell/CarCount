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

Config::Config(std::string dbFileName)  : mDbFile(dbFileName) {

	// TODO clear set-up of db open and close
	bool success = populateStdParams();
	m_workPath = getParam("work_path");

	if (dbFileName != "") {
		success = openDb(dbFileName);
		if (!success)
			cerr << __LINE__ << " error initializing config instance" << endl;
	}
	else {
		cerr << "no config file" << endl;
		mDbHandle = nullptr;
	}
}

Config::~Config() {
	if (mDbHandle) {
		int rc = sqlite3_close(mDbHandle);
		if (rc != SQLITE_OK)
			cerr << __LINE__ << " data base was not closed correctly" << endl;
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
	list<Parameter>::iterator iParam = find_if(mParamList.begin(), mParamList.end(), ParamEquals(name));
	if (iParam == mParamList.end()) {
		cerr << "parameter not in config: " << name <<endl;
		return string("");
	}
	else {
		return (iParam->getValue());
	}
}

bool Config::init() {
	return loadParams();
	///TODO move working path logic to here
	/// store workingPath for db and video files in ParamList

}


bool Config::insertParam(Parameter param) {
	mParamList.push_back(param);
	return true;
}


bool Config::loadParams() {
	using namespace	std;
	bool success = false;
	stringstream ss;
	string sqlCreate, sqlRead, sqlinsert;
	string noRet = "";
	
	// check, if table exists
	ss.str("");
	ss << "create table if not exists " << mDbTblConfig << " (name text, type text, value text);";
	sqlCreate = ss.str();
	success = queryDbSingle(sqlCreate, noRet);


	list<Parameter>::iterator iParam = mParamList.begin();
	while (iParam != mParamList.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from " << mDbTblConfig << " where name=" << "'" << iParam->getName() << "';"; 
		sqlRead = ss.str();
		string strValue;
		success = queryDbSingle(sqlRead, strValue);

		if (success && !strValue.empty()) // use parameter value from db
			iParam->setValue(strValue);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into " << mDbTblConfig << " values ('" << iParam->getName() << "', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
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
	// TODO delete after debug
	readEnvHome(); // set mHomePath
	if (!mHomePath.size()) {
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
	filePath = mHomePath;
	appendDirToPath(filePath, fileName);
	if (isFileExist(filePath)) {
		m_videoFilePath = filePath;
		return true;
	}

	// 3. check /home/work_dir
	filePath = m_workPath;
	appendDirToPath(filePath, fileName);
	if (isFileExist(filePath)) {
		m_videoFilePath = filePath;
		return true;
	}

	// file not found
	return false;
}


bool Config::openDb(std::string dbFile) {
	using namespace	std;
	bool success = false;

	/* set working dir, db file and config table, compose working path */
	if (dbFile == "")
		mDbFile = "carcounter.sqlite";
	else mDbFile = dbFile;
	
	mDbTblConfig = "config";
	mDbPath = m_workPath;

	if (!pathExists(mDbPath))
		if (!makePath(mDbPath)) {
			cerr << __LINE__ << " cannot create working directory" << endl;
			cerr << GetLastError() << endl;
			return false;
		};

	string fullPath = mDbPath + mDbFile;
	
	/* open db and get handle */
	int rc = sqlite3_open(fullPath.c_str(), &mDbHandle);
	if (rc == SQLITE_OK)
		success = true;
	else {
		cerr << __LINE__ << " error opening " << mDbPath << endl;
		cerr << __LINE__ << " " << sqlite3_errmsg(mDbHandle) << endl;
		sqlite3_close(mDbHandle);
		mDbHandle = NULL;
		success = false;
	}
	return success;
}


bool Config::populateStdParams() {
	mParamList.clear();

	/* TODO set standard parameters */
	// TODO parameter for capFileName
	// capFile, capDevice, framesize, framerate
	mParamList.push_back(Parameter("video_file", "string", "traffic640x480.avi"));
	mParamList.push_back(Parameter("work_path", "string", "D:/Users/Holger/count_traffic/"));
	mParamList.push_back(Parameter("video_device", "int", "0"));
	mParamList.push_back(Parameter("is_video_from_cam", "bool", "false"));
	mParamList.push_back(Parameter("frame_size_id", "int", "0"));
	mParamList.push_back(Parameter("frame_size_x", "int", "320"));
	mParamList.push_back(Parameter("frame_size_y", "int", "240"));
	mParamList.push_back(Parameter("frame_rate", "int", "10"));
	// region of interest
	mParamList.push_back(Parameter("roi_x", "int", "80"));
	mParamList.push_back(Parameter("roi_y", "int", "80"));
	mParamList.push_back(Parameter("roi_width", "int", "180"));
	mParamList.push_back(Parameter("roi_height", "int", "80"));
	// blob assignment
	mParamList.push_back(Parameter("blob_area_min", "int", "200"));
	mParamList.push_back(Parameter("blob_area_max", "int", "20000"));
	// track assignment
	mParamList.push_back(Parameter("track_max_confidence", "int", "4"));
	mParamList.push_back(Parameter("track_max_deviation", "double", "80"));
	mParamList.push_back(Parameter("track_max_distance", "double", "30"));
	// scene tracker
	mParamList.push_back(Parameter("max_n_of_tracks", "int", "9")); // maxNoIDs
	// counting
	mParamList.push_back(Parameter("count_confidence", "int", "3"));
	mParamList.push_back(Parameter("count_pos_x", "int", "90")); // counting position (within roi)
	mParamList.push_back(Parameter("count_track_length", "int", "20")); // min track length for counting
	// classification
	mParamList.push_back(Parameter("truck_width_min", "int", "60")); // classified as truck, if larger
	mParamList.push_back(Parameter("truck_height_min", "int", "28")); // classified as truck, if larger
	return true;
}


bool Config::queryDbSingle(const std::string& sql, std::string& value) {
	bool success = false;
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare_v2(mDbHandle, sql.c_str(), -1, &stmt, 0);
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
		cerr << "SQL error: " << sqlite3_errmsg(mDbHandle) << endl;
		rc = sqlite3_finalize(stmt);
		success = false;
	}
	return success;
}


// TODO only perform lexical checks in this function
//	physical test, e.g. file existence will be checked 
bool Config::readCmdLine(ProgramOptions programOptions) {
	using namespace	std;
	// arguments
	//  i(nput): cam (single digit number) or file name,
	//		if empty take standard cam
	//  r(ate):	fps for video device
	//  v(ideo size): frame size in px (width x height)
	//  w(orking directory): working dir, below $home

	// TODO perform check after workdir has been checked
	//  look for input file in
	//		- current directory
	//		- workdir

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
				setParam("video_device", camID);
				setParam("is_video_from_cam", "true");
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
		}
	} // end if (programOptions.exists('i'))

	// r: frame rate in fps
	if (programOptions.exists('r')) {
		string frameRate = programOptions.getOptArg('r');
		if (isInteger(frameRate)) {
			setParam("frame_rate", frameRate);
		}
		else {
			cerr << "invalid argument: frame rate option '-r " << frameRate << "'" << endl;
			cerr << "frame rate must be integer" << endl;
			return false;
		}
	} // end if (programOptions.exists('r'))

	// v: video size
	if (programOptions.exists('v')) {
		string videoSize = programOptions.getOptArg('v');
		string delimiter = "x";

		size_t firstPos = 0;
		size_t nextPos = videoSize.find_first_of(delimiter, firstPos);
		string framesize_x = videoSize.substr(firstPos, nextPos);

		if (isInteger(framesize_x))
			setParam("frame_size_x", framesize_x);
		else {
			cerr << "invalid argument: video size option '-v " << videoSize << "'" << endl;
			cerr << "video width must be integer" << endl;
			return false;
		}

		firstPos = nextPos + 1;
		nextPos = videoSize.find_first_of(delimiter, firstPos);
		string framesize_y = videoSize.substr(firstPos, nextPos);

		if (isInteger(framesize_y))
			setParam("frame_size_y", framesize_y);
		else {
			cerr << "invalid argument: video size option '-v " << videoSize << "'" << endl;
			cerr << "video height must be integer" << endl;
			return false;
		}
	} // end if (programOptions.exists('v'))

	//  w: working directory, below $home
	if (programOptions.exists('w')) {
		string workPath = appendDirToPath(mHomePath, programOptions.getOptArg('w'));
		setParam("work_path", workPath);
	} // end if (programOptions.exists('w'))

	return true;
}


std::string Config::readEnvHome() {
	using namespace	std;
	mHomePath.clear();

	#if defined (_WIN32)
		char *pHomeDrive, *pHomePath;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive != nullptr)
			mHomePath += pHomeDrive;
		else {
			cerr << __FILE__ << __LINE__ << 
				" no home drive set in env" << endl;
			return mHomePath;
		}
		pHomePath = getenv("HOMEPATH");
		appendDirToPath(mHomePath, string(pHomePath));
	#elif defined (__linux__)
		char *pHomePath;
		pHomePath = getenv("HOME");
		if (pHome != nullptr)	{
			home += pHome;
			home += '/';
		}
	#else
		throw 1;
	#endif

	return mHomePath;
}


bool Config::setParam(std::string name, std::string value) {
	list<Parameter>::iterator iParam = find_if(mParamList.begin(), mParamList.end(),
		ParamEquals(name));
	if (iParam == mParamList.end())
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


std::string getHome() {
	std::string home;
	#if defined (_WIN32)
		char *pHomeDrive, *pHomePath;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive != 0)
			home += pHomeDrive;
		pHomePath = getenv("HOMEPATH");
		if (pHomePath !=0)
			home = appendDirToPath(home, std::string(pHomePath));
	#elif defined (__linux__)
		char *pHome;
		pHomePath = getenv("HOME");
		if (pHome !=0)
		{
			home += pHome;
			home += '/';
		}
	#else
		throw "unsupported OS";
	#endif
	
	return home;
}


bool pathExists(const std::string& path) {
	#if defined (_WIN32)
		std::wstring wPath;
		wPath.assign(path.begin(), path.end());

		DWORD fileAttrib = GetFileAttributes(wPath.c_str());
		if (fileAttrib == INVALID_FILE_ATTRIBUTES)
		{
			DWORD lastError = GetLastError();
			if (lastError != ERROR_FILE_NOT_FOUND && lastError != ERROR_PATH_NOT_FOUND)
				throw "file access error";
			return false;
		}
		else
			return true;
	#elif defined (__linux__)
		// TODO use stat from <sys/stat.h>
		throw "linux";
	#else
		throw "unsupported OS";
	#endif
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
	if (_access(path.c_str(), 0) == 0)
		return true;
	else
		return false;
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