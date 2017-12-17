#pragma once

#include "tracking.h"

class Parameter {
public:
	Parameter(std::string name = "name", std::string type = "type", std::string value = "value");
	std::string getName() const;
	std::string getType() const;
	std::string getValue() const;
	double getDouble() const;
	int getInt() const;
	bool setValue(std::string& value);
private:
	std::string mName;
	std::string mType;
	std::string mValue;
};

class Config {
private:
	sqlite3* mDbHandle;
	std::string mDbFile;
	std::string mWorkDir;
	std::string mDbPath;
	std::string mDbTblConfig;
	std::string mDbTblData; // table for time series
	std::list<Parameter> mParamList;
	Scene* mScene; // Observer

	// TODO 
	// loadParam("all" - load all, "name" - load only param 'name')
	bool notifyScene();	
	bool loadParams();
	bool saveParams();
	bool openDb(std::string dbFile);
	bool queryDbSingle(const std::string& sql, std::string& value);

public:
	Config(std::string dbFileName = "");
	~Config();
	bool attachScene(Scene* pScene);
	bool init();
	bool populateStdParams();
	bool changeParam(std::string name, std::string value);
	bool insertParam(Parameter param);
	double getDouble(std::string name);
};


// Directory manipulation functions
std::string getHome();
std::string& appendDirToPath(std::string& path, std::string& dir);
bool pathExists(std::string& path);
bool makeDir(std::string& dir);
bool makePath(std::string& path);