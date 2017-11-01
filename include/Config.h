
class Parameter {
public:
	Parameter(std::string _name = "name", std::string _type = "type", std::string _value = "value");
	std::string getName();
	std::string getType();
	std::string getValue();
	double getDouble();
	int getInt();
	bool setValue(std::string& _value);

private:
	std::string name;
	std::string type;
	std::string value;
};

class Config {
public:
	Config();
	bool Init();
private:
	sqlite3* dbHandle;
	std::string dbFile;
	std::string workDir;
	std::string dbPath;
	std::string dbTblConfig;
	std::string dbTblData; // table for time series
	std::list<Parameter> params;

	bool LoadParams(sqlite3* db);
	bool setDbPath(std::string& workDir);
};

// Directory manipulation functions
std::string getHome();
bool PathExists(std::string& path);
std::string& appendDirToPath(std::string& path, std::string& dir);
bool MakeDir(std::string& dir);
bool MakePath(std::string& path);

// DB functions
bool queryDbSingle(const std::string& sql, sqlite3* db, std::string&);
sqlite3* openDb(const std::string& fullPath);


