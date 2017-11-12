
class Parameter {
public:
	Parameter(std::string name = "name", std::string type = "type", std::string value = "value");
	std::string getName() const;
	std::string getType() const;
	std::string getValue() const;
	double getDouble() const;
	bool setValue(std::string& value);
private:
	std::string mName;
	std::string mType;
	std::string mValue;
};

class Config {
public:
	Config(std::string dbFileName = "");
	~Config();
	bool init();
	bool populateStdParams();
	bool insertParam(Parameter param);
	bool openDb(std::string dbFile);
	bool queryDbSingle(const std::string& sql, std::string& value);
	double getDouble(std::string name);

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
};

// Directory manipulation functions
std::string getHome();
std::string& appendDirToPath(std::string& path, std::string& dir);
bool pathExists(std::string& path);
bool makeDir(std::string& dir);
bool makePath(std::string& path);