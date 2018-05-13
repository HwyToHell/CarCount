#pragma once

class ConfigFile {
	std::string m_dbFile;
public:
	ConfigFile(std::string dbFileName = "");
	~ConfigFile();
	bool appendParam(std::string parName, std::string parValue, std::string parType = "int");
	std::string readParam(std::string parName);
	bool writeParam(std::string parName, std::string parValue);

};

