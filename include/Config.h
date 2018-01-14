#pragma once
#include "tracker.h"
#include "../../../cpp/inc/observer.h"

class Parameter {
private:
	std::string mName;
	std::string mType;
	std::string mValue;
public:
	Parameter(std::string name = "name", std::string type = "type", std::string value = "value");
	std::string getName() const;
	std::string getType() const;
	std::string getValue() const;
	double getDouble() const;
	int getInt() const;
	bool setValue(std::string& value);
};

class Config : public Subject {
private:
	sqlite3* mDbHandle;
	std::string mDbFile;
	std::string mWorkDir;
	std::string mDbPath;
	std::string mDbTblConfig;
	std::string mDbTblData; // table for time series
	std::list<Parameter> mParamList;

	// TODO 
	// loadParam("all" - load all, "name" - load only param 'name')
	bool loadParams();
	bool saveParams();
	bool openDb(std::string dbFile);
	bool queryDbSingle(const std::string& sql, std::string& value);

public:
	Config(std::string dbFileName = "");
	~Config();
	double getDouble(std::string name);
	std::string getParam(std::string name);
	bool init();	
	bool insertParam(Parameter param);
	bool populateStdParams();
	bool setParam(std::string name, std::string value);
};


// Directory manipulation functions
std::string getHome();
std::string& appendDirToPath(std::string& path, std::string& dir);
bool pathExists(std::string& path);
bool makeDir(std::string& dir);
bool makePath(std::string& path);