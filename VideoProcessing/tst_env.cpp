#include "stdafx.h"
#include <cstdlib>
#include <cstring>
#include <locale>
#include "sqlite/sqlite3.h"
//#include <boost/filesystem.hpp>
//..VideoProcessing/..OpenCV/..AppDev/

using namespace std;


class Parameter {
public:
	Parameter(string _name = "name", string _type = "type", string _value = "value");
	string getName();
	string getType();
	string getValue();
	double getDouble();
	int getInt();
	bool setValue(string& _value);

private:
	string name;
	string type;
	string value;
};

class Config {
public:
	Config();
	bool Init();
private:
	string dbFileName;
	string workDir;
	string dbPath;
	string dbTblConfig;
	string dbTblData; // table for time series
	list<Parameter> parameters;
	bool setDbPath(string& workDir);
};

string getHome();
string& appendDirToPath(string& path, string& dir);
sqlite3* openDb(const char *filename);
list<Parameter> loadParams(sqlite3* db);


int main(int argc, char *argv[])
{
	Config cfg;
	cfg.Init();
	sqlite3 *db = openDb("carcounter.sqlite");
	if (db != NULL)
		list<Parameter> parList = loadParams(db);
	else
		cerr << __LINE__ << " db pointer == NULL" << endl;

	cout << endl << "hit enter to exit";
	string str;
	getline(cin, str);
	return 0;
}

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

sqlite3* openDb(const char *filename)
{
	string fullPath;
	sqlite3 *db;
	int rc;

	fullPath = getHome();
	fullPath += "count_traffic\\";
	fullPath += filename;

	rc = sqlite3_open(fullPath.c_str(), &db);
	if (rc != SQLITE_OK)
	{
		cerr << __LINE__ << " error opening " << fullPath << endl;
		cerr << __LINE__ << " " << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		return NULL;
	}
	return db;
}



int queryDbSingle(const string& sql, sqlite3* db, string& value)
{
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
		
		if (value == "")
			return SQLITE_ERROR;
		else
			return SQLITE_OK;
	} 

	else // sqlite3_prepare != OK
	{
		cerr << "SQL error: " << sqlite3_errmsg(db) << endl;
		rc = sqlite3_finalize(stmt);
		return SQLITE_ERROR;
	}

}



list<Parameter> loadParams(sqlite3* db)
{
	list<Parameter> params;

	params.push_back(Parameter("framesize_x", "int", "640"));
	params.push_back(Parameter("framesize_y", "int", "480"));
	params.push_back(Parameter("roi_x", "int", "320"));
	params.push_back(Parameter("roi_y", "int", "240"));
	params.push_back(Parameter("confidence", "int", "5"));
	params.push_back(Parameter("same_velocity", "real", "0.2"));


	
	list<Parameter>::iterator it = params.begin();
	stringstream ss;
	string sqlRead, sqlinsert;
	while (it != params.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from par1 where name=" << "'" << it->getName() << "';"; 
		sqlRead = ss.str();
		string strValue;
		int rc = queryDbSingle(sqlRead, db, strValue);
		cout << "standard: " << setw(4) << it->getValue();
		
		if (rc == SQLITE_OK) // use parameter value from db
			it->setValue(strValue);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into par1 values ('" << it->getName() << "', '" << it->getType() << "', '" << it->getValue() << "');";
			sqlinsert = ss.str();
			strValue = it->getValue();
			rc =  queryDbSingle(sqlinsert, db, strValue);
		}
		cout << "  database: " << setw(4) << strValue << endl;
		++it;
	}





	/** ToDo db stuff
		select count(type) from params where 
		*/

	return params;
}


	


Parameter::Parameter(string _name, string _type, string _value) : name(_name), type(_type), value(_value)
{

}

string Parameter::getName()
{
	return name;
}

string Parameter::getType()
{
	return type;
}

string Parameter::getValue()
{
	return value;
}

double Parameter::getDouble()
{
	return stod(value);
}

int Parameter::getInt()
{
	return stoi(value);
}

bool Parameter::setValue(string& _value)
{
	//TODO: extract typed value from parameter string
	value = _value;
	return true;
}


Config::Config()
{
	if (!Init())
		cerr << __LINE__ << " error initializing config instance" << endl;
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
		// use stat from <sys/stat.h>
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
		//use mkdir from <unistd.h>
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
	size_t posBegin = 0, posEnd = 0;

	posEnd = path.find_first_of(delim);
	while (success == true && posEnd != string::npos)
	{
		size_t length = posEnd - posBegin;

		if (length > 0)
		{
			// skip leading and subsequent delimiters
			string dirToCreate = path.substr(0, posEnd);
			success = MakeDir(dirToCreate);
		}
		posBegin = posEnd + 1;
		posEnd = path.find_first_of(delim, posBegin);
	}
	return success;
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

bool Config::Init()
{
	/* TODO set standard parameters */

	/* compose working path */
	workDir = "count_traffic";
	dbPath = appendDirToPath(getHome(), workDir);
	
	// if working directory does not exists -> create it
	if (!PathExists(dbPath))
		if (!MakePath(dbPath))
		{
			cerr << __LINE__ << " cannot create working directory" << endl;
			cerr << GetLastError() << endl;
			return false;
		}


	/* TODO load actual parameters from file 
	 *  open db, (create if not exists)
	 *  create config table (if not exists)
	 *  query parameters
	 */
	dbTblConfig = "config";



	return true;
}



