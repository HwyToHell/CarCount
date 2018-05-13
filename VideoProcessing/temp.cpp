bool Config::loadParamsFromDb() {
	using namespace	std;
	
	// check valid object state
	if (!m_dbHandle) {
		cerr << "loadParamsFromDb: data base not open yet" << endl;
		return false;
	}
	if (m_paramList.empty()) {
		cerr << "loadParamsFromDb: parameter list not initialized yet" << endl;
		return false;
	}
	if (m_configTableName.empty()) {
		cerr << "loadParamsFromDb: db table not specified" << endl;
		return false;
	}

	bool success = false;
	stringstream ss;
	string sqlCreate, sqlRead, sqlinsert;
	string noRet = "";
	
	// check, if table exists
	ss.str("");
	ss << "create table if not exists " << m_configTableName << " (name text, type text, value text);";
	sqlCreate = ss.str();
	success = queryDbSingle(sqlCreate, noRet);


	list<Parameter>::iterator iParam = m_paramList.begin();
	while (iParam != m_paramList.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from " << m_configTableName << " where name=" << "'" << iParam->getName() << "';"; 
		sqlRead = ss.str();
		string strValue;
		success = queryDbSingle(sqlRead, strValue);

		if (success && !strValue.empty()) // use parameter value from db
			iParam->setValue(strValue);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into " << m_configTableName << " values ('" << iParam->getName() << "', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
			sqlinsert = ss.str();
			strValue = iParam->getValue();
			success =  queryDbSingle(sqlinsert, noRet);
		}
		++iParam;
	}

	return success;
}



