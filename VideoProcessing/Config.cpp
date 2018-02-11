#include "stdafx.h"
#include "../include/config.h"

using namespace std;

void updateObserver(Observer* pObserver) {
	pObserver->update();
}

Parameter::Parameter(string name, string type, string value) : mName(name), mType(type), mValue(value) {}

string Parameter::getName() const { return mName; }

string Parameter::getType() const { return mType; }

string Parameter::getValue() const { return mValue; }

double Parameter::getDouble() const { return stod(mValue); }

bool Parameter::setValue(string& value) {
	mValue = value;
	return true;
}


Config::Config(string dbFileName) {
	bool success = false;
	success = populateStdParams();
	success = openDb(dbFileName);
	if (!success)
		cerr << __LINE__ << " error initializing config instance" << endl;
}

Config::~Config() {
	int rc = sqlite3_close(mDbHandle);
	if (rc != SQLITE_OK)
		cerr << __LINE__ << " data base was not closed correctly" << endl;
}


bool Config::init() {
	return loadParams();
	///TODO move working path logic to here
	/// store workingPath for db and video files in ParamList

}

bool Config::populateStdParams() {
	mParamList.clear();

	/* TODO set standard parameters */
	// TODO parameter for capFileName
	// capFile, capDevice, framesize
	mParamList.push_back(Parameter("video_file", "string", "traffic.avi"));
	mParamList.push_back(Parameter("video_path", "string", "D:/Users/Holger/count_traffic/"));
	mParamList.push_back(Parameter("video_device", "int", "0"));
	mParamList.push_back(Parameter("framesize_x", "int", "320"));
	mParamList.push_back(Parameter("framesize_y", "int", "240"));
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


bool Config::insertParam(Parameter param) {
	mParamList.push_back(param);
	return true;
}

bool Config::openDb(string dbFile) {
	bool success = false;

	/* set working dir, db file and config table, compose working path */
	if (dbFile == "")
		mDbFile = "carcounter.sqlite";
	else mDbFile = dbFile;
	
	mWorkDir = "count_traffic";
	mDbTblConfig = "config";
	mDbPath = appendDirToPath(getHome(), mWorkDir);

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

bool Config::loadParams() {
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

bool Config::queryDbSingle(const string& sql, string& value) {
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

class ParamEquals : public unary_function<Parameter, bool> {
	string mName;
public: 
	ParamEquals (const string& name) : mName(name) {}
	bool operator() (const Parameter& par) const { 
		return (mName == par.getName());
	}
};

double Config::getDouble(string name) {
	list<Parameter>::iterator iParam = find_if(mParamList.begin(), mParamList.end(), ParamEquals(name));
	return (iParam->getDouble());
}

string Config::getParam(string name) {
	list<Parameter>::iterator iParam = find_if(mParamList.begin(), mParamList.end(), ParamEquals(name));
	return (iParam->getValue());
}

bool Config::setParam(string name, string value) {
	list<Parameter>::iterator iParam = find_if(mParamList.begin(), mParamList.end(), ParamEquals(name));
	if (iParam == mParamList.end())
		return false;
	iParam->setValue(value);
	return true;
}


// Directory manipulation functions
string getHome() {
	string home;
	#if defined (_WIN32)
		char *pHomeDrive, *pHomePath;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive != 0)
			home += pHomeDrive;
		pHomePath = getenv("HOMEPATH");
		if (pHomePath !=0)
			home = appendDirToPath(home, string(pHomePath));
	#elif defined (__linux__)
		char *pHome;
		pHomePath = getenv("HOME");
		if (pHome !=0)
		{
			home += pHome;
			home += '/';
		}
	#else
		throw 1;
	#endif
	
	return home;
}

string& appendDirToPath(string& path, string& dir) {
	path += dir;
	#if defined (_WIN32)
			path += '\\';
	#elif defined (__linux__)
			path += '/';
	#else
		throw 1;
	#endif
	return path;
}

bool pathExists(string& path) {
	#if defined (_WIN32)
		wstring wPath;
		wPath.assign(path.begin(), path.end());

		DWORD fileAttrib = GetFileAttributes(wPath.c_str());
		if (fileAttrib == INVALID_FILE_ATTRIBUTES)
		{
			DWORD lastError = GetLastError();
			if (lastError != ERROR_FILE_NOT_FOUND && lastError != ERROR_PATH_NOT_FOUND)
				throw 3;
			return false;
		}
		else
			return true;
	#elif defined (__linux__)
		// TODO use stat from <sys/stat.h>
		throw 2;
	#else
		throw 1;
	#endif
}

bool makeDir(string& dir) {
	#if defined (_WIN32)
		wstring wDir;
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
		throw 2;
	#else
		throw 1;
	#endif
}

bool makePath(string& path) {
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw 1;
	#endif
	bool success = true;
	size_t pos = 0;

	// path must end with delim character
	if (path.back() != delim)
		path += delim;

	pos = path.find_first_of(delim);
	while (success == true && pos != string::npos)
	{
		if (pos != 0)
			success = makeDir(path.substr(0, pos));
		++pos;
		pos = path.find_first_of(delim, pos);
	}
	return success;
}