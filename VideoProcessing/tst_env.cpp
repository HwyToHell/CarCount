#include "stdafx.h"
#include <cstdlib>
#include "sqlite/sqlite3.h"

using namespace std;


class Parameter {
public:
	Parameter(string _name = "name", string _type = "type", string _value = "value");
	string getName();
	string getType();
	string getValue();

private:
	string name;
	string type;
	string value;
};

string getHome();
sqlite3* openDb(const char *filename);
list<Parameter> loadParams(sqlite3* db);

/**ToDo
open db = OK
parameterlist "name" "value"
does table exist -y-> read columns
	-n-> create one
if column exist -y-> read parameter from db
	-n-> use std parameter, write to db
	
if not exist take standard and write back to db
*/



int main(int argc, char *argv[])
{
	string str;
	str = getHome();
	cout << "GetHome: " << str << endl;
	sqlite3 *db = openDb("carcounter.sqlite");
	if (db != NULL)
		list<Parameter> parList = loadParams(db);
	else
		cerr << __LINE__ << " db pointer == NULL" << endl;


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
		{
			home += pHomePath;
			home += '\\';
		}
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


static int callback(void *notUsed, int nCols, char *azRowFields[], char *azColNames[])
{
	for (int i = 0; i < nCols; ++i)
	{
		cout << azColNames[i] << ": " << azRowFields[i] << endl;
	}
	cout << endl;
	return 0;
}
	
list<Parameter> loadParams(sqlite3* db)
{
	list<Parameter> params;

	params.push_back(Parameter("framesize_x", "int", "320"));
	params.push_back(Parameter("framesize_y", "int", "240"));

	char *errMsg = NULL;
	int rc = sqlite3_exec(db, "select count(type) from sqlite_master where type='table' and name='parameters';", callback, 0, &errMsg);
	if (rc != SQLITE_OK)
	{
		cerr << "SQL error: " << errMsg << endl;
		sqlite3_free(errMsg);
		sqlite3_close(db);
	}
	const char *sql = "select framesize_x from parameters;";
	rc = sqlite3_exec(db, sql, callback, 0, &errMsg);

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

