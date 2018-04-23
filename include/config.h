#pragma once
#include "tracker.h"
#include "../../../cpp/inc/observer.h"
#include "../../../cpp/inc/program_options.h"

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
	bool setValue(std::string& value);
};

class Config : public Subject {
private:
	// i(nput):				video device or file name
	// r(ate):				fps for video device
	// v(ideo size):		frame size in px (width x height)
	// w(orking directory): working dir, starting in $home

	sqlite3*				mDbHandle;
	std::string				mDbFile;
	std::string				mDbPath;
	std::string				mDbTblConfig;
	std::string				mDbTblData; // table for time series
	std::string				mHomePath;
	std::list<Parameter>	mParamList;
	std::string				m_workPath; // TODO delete?
	std::string				m_videoFilePath;

	// TODO 
	// loadParam("all" - load all, "name" - load only param 'name')
	bool					loadParams(); // TODO move logic to readEnv()	
	bool					openDb(std::string dbFile);
	bool					queryDbSingle(const std::string& sql, std::string& value);
	std::string				readEnvHome();
	bool					saveParams(); // TODO move logic to readEnv()

public:
	Config(std::string dbFileName = "");
	// TODO constructor for test cases, without reading config file
	// Config();
	~Config();
	void		adjustFrameSizeDependentParams(int new_size_x, int new_size_y); // TODO implementation
	std::string getParam(std::string name);
	bool		init(); // TODO move logic to readEnv()	
	bool		insertParam(Parameter param);
	bool		locateVideoFile(std::string fileName);
	bool		populateStdParams();
	bool		readCmdLine(ProgramOptions po);
	bool		readConfigFile(); // TODO implementation
	bool		saveConfigToFile; // TODO implementation
	bool		setParam(std::string name, std::string value);
};


// Directory manipulation functions
std::string		getHome();
std::string&	appendDirToPath(std::string& path, const std::string& dir);
bool			isFileExist(const std::string& path);
bool			pathExists(const std::string& path); // TODO delete, us isFileExist instead
bool			makeDir(const std::string& dir);
bool			makePath(std::string path);

// String conversion functions
bool			isInteger(const std::string& str);
bool			stob(std::string str); // string to bool