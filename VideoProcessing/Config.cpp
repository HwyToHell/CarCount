#include "stdafx.h"
#include "../include/config.h"

using namespace std;

Parameter::Parameter(string name, string type, string value) : mName(name), mType(type), mValue(value)
{

}

string Parameter::getName()
{
	return mName;
}

string Parameter::getType()
{
	return mType;
}

string Parameter::getValue()
{
	return mValue;
}

double Parameter::getDouble()
{
	return stod(mValue);
}

int Parameter::getInt()
{
	return stoi(mValue);
}

bool Parameter::setValue(string& value)
{
	mValue = value;
	return true;
}


Config::Config()
{
	if (!Init())
		cerr << __LINE__ << " error initializing config instance" << endl;
}

bool Config::Init()
{
	bool success = false;
	mParamList.clear();

	/* TODO set standard parameters */
	mParamList.push_back(Parameter("framesize_x", "int", "640"));
	mParamList.push_back(Parameter("framesize_y", "int", "480"));
	mParamList.push_back(Parameter("roi_x", "int", "320"));
	mParamList.push_back(Parameter("roi_y", "int", "240"));
	mParamList.push_back(Parameter("confidence", "int", "5"));
	mParamList.push_back(Parameter("same_velocity", "real", "0.2"));


	/* set working dir, db file and config table, compose working path */
	mDbFile = "carcounter.sqlite";
	mWorkDir = "count_traffic";
	mDbTblConfig = "config";
	mDbPath = appendDirToPath(getHome(), mWorkDir);

	if (!PathExists(mDbPath))
		if (!MakePath(mDbPath))
		{
			cerr << __LINE__ << " cannot create working directory" << endl;
			cerr << GetLastError() << endl;
			return false;
		}


	/* load actual parameters from file */
	string strDebug = mDbPath+mDbFile;

	sqlite3* mDbHandle = openDb(mDbPath+mDbFile);
	if (mDbHandle == NULL)
		success = false;
	else
		success = LoadParams(mDbHandle);

	return success;
}

bool Config::LoadParams(sqlite3* db)
{
	bool success = false;
	stringstream ss;
	string sqlCreate, sqlRead, sqlinsert;
	string noRet = "";
	
	// check, if table exists
	ss.str("");
	ss << "create table if not exists " << mDbTblConfig << " (name text, type text, value text);";
	sqlCreate = ss.str();
	success = queryDbSingle(sqlCreate, db, noRet);


	list<Parameter>::iterator iParam = mParamList.begin();
	while (iParam != mParamList.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from " << mDbTblConfig << " where name=" << "'" << iParam->getName() << "';"; 
		sqlRead = ss.str();
		string strValue;
		success = queryDbSingle(sqlRead, db, strValue);
		cout << setw(10) << "standard: " << setw(14) << iParam->getName() << setw(4) << iParam->getValue();
		//TODO: new params
		if (success && !strValue.empty()) // use parameter value from db
			iParam->setValue(strValue);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into " << mDbTblConfig << " values ('" << iParam->getName() << "', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
			sqlinsert = ss.str();
			strValue = iParam->getValue();
			success =  queryDbSingle(sqlinsert, db, noRet);
		}
		cout << setw(12) << "  database: " << setw(14) << iParam->getName() << setw(4) << iParam->getValue() << endl;
		++iParam;
	}

	return success;
}


// Directory manipulation functions
string getHome()
{
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

string& appendDirToPath(string& path, string& dir)
{
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

bool PathExists(string& path)
{
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

bool MakeDir(string& dir)
{
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

bool MakePath(string& path)
{
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
			success = MakeDir(path.substr(0, pos));
		++pos;
		pos = path.find_first_of(delim, pos);
	}
	return success;
}

// DB functions
sqlite3* openDb(const string& fullPath)
{
	sqlite3 *db;

	int rc = sqlite3_open(fullPath.c_str(), &db);
	if (rc != SQLITE_OK)
	{
		cerr << __LINE__ << " error opening " << fullPath << endl;
		cerr << __LINE__ << " " << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		db = NULL;
	}
	return db;
}

bool queryDbSingle(const string& sql, sqlite3* db, string& value)
{
	bool success = false;
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
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
		cerr << "SQL error: " << sqlite3_errmsg(db) << endl;
		rc = sqlite3_finalize(stmt);
		success = false;
	}
	return success;
}