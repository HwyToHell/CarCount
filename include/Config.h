
class Parameter {
public:
	Parameter(std::string name = "name", std::string type = "type", std::string value = "value");
	std::string getName();
	std::string getType();
	std::string getValue();
	double getDouble();
	int getInt();
	bool setValue(std::string& value);

private:
	std::string mName;
	std::string mType;
	std::string mValue;
};

class Config {
public:
	Config();
	bool Init();
private:
	sqlite3* mDbHandle;
	std::string mDbFile;
	std::string mWorkDir;
	std::string mDbPath;
	std::string mDbTblConfig;
	std::string mDbTblData; // table for time series
	std::list<Parameter> mParamList;

	bool LoadParams(sqlite3* db);
};

// Directory manipulation functions
std::string getHome();
std::string& appendDirToPath(std::string& path, std::string& dir);
bool PathExists(std::string& path);
bool MakeDir(std::string& dir);
bool MakePath(std::string& path);

// DB functions
sqlite3* openDb(const std::string& fullPath);
bool queryDbSingle(const std::string& sql, sqlite3* db, std::string&);



