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

/// config: hold application parameters, command line parsing,
/// read from / write to sqlite config file,
/// automatic update of subscribed observers
class Config : public Subject {
public:
	Config();
	~Config();
	void		adjustFrameSizeDependentParams(int new_size_x, int new_size_y); // TODO implementation
	std::string getParam(std::string name);
	bool		insertParam(Parameter param);
	bool		locateVideoFile(std::string fileName);
	bool		readCmdLine(ProgramOptions po);
	bool		readConfigFile(std::string configFile); 
	bool		saveConfigToFile(); // TODO implementation
	bool		setParam(std::string name, std::string value);

private:
	std::string				m_appPath;
	std::string				m_configFilePath;
	std::string				m_configTableName;
	sqlite3*				m_dbHandle;
	std::string				m_homePath;
	bool					m_quiet;
	std::list<Parameter>	m_paramList;
	std::string				m_videoFilePath;

	// init: set path to application and con-fig file,
	// create parameter list (with std values)
	bool					init(); 
	bool					loadParamsFromDb();
	bool					populateStdParams();
	bool					queryDbSingle(const std::string& sql, std::string& value);
	bool					saveParams(); // TODO move logic to readEnv()
	bool					setAppPath(std::string appDir = "counter");
	void					setConfigProps(	std::string configDirPath, 
											std::string configFileName = "config.sqlite",
											std::string configTable = "config");

};


// Helper
void printCommandOptions();

// Directory manipulation functions
std::string		getHomePath();
std::string&	appendDirToPath(std::string& path, const std::string& dir);
bool			isFileExist(const std::string& path);
bool			makeDir(const std::string& dir);
bool			makePath(std::string path);

// String conversion functions
bool			isInteger(const std::string& str);
bool			stob(std::string str); // string to bool