#include "stdafx.h"

#include "../include/config.h"

bool queryDbSingle(sqlite3* dbHandle, const std::string& sql, std::string& value);

int main(int argc, char* argv[]) {
	using namespace std;
	
	try {
		cout << "true  " << "stob: " << stob("true") << endl;
		cout << "1     " << "stob: " << stob("1") << endl;
		cout << "0     " << "stob: " << stob("0") << endl;
		cout << "      " << "stob: " << stob("") << endl;
		cout << "2     " << "stob: " << stob("2") << endl;
	}
	catch (const char* p) {
		cout << "char* exeption: " << p << endl;
	}
	catch (...) {
		cout << "default exeption" << endl;
	}

	// sqlite api test
	sqlite3* db = nullptr;
	string dbFilePath("D:\\Users\\Holger\\counter\\xyz.sqlite");
	int rc = sqlite3_open(dbFilePath.c_str(), &db);
	string answer;
	
	// create file and table 
	string sql("create table if not exists config (name text, type text, value text);");
	bool succ = queryDbSingle(db, sql, answer);

	// insert row
	sql = "insert into config values ('roi', 'int', '9');";
	succ = queryDbSingle(db, sql, answer);

	// close db
	rc = sqlite3_close(db);


	// performance test
	string testFile("D:\\Users\\Holger\\counter\\xyz_perf.sqlite");
	Config config;

	double before = (double)cv::getTickCount();
	bool succ = config.saveConfigToFile(testFile);
	double after = (double)cv::getTickCount();
	double sec = (after - before) / cv::getTickFrequency();
	cout << "config saved in " << sec << " seconds" << endl;

	before = (double)cv::getTickCount();
	succ = config.readConfigFile(testFile);
	after = (double)cv::getTickCount();
	sec = (after - before) / cv::getTickFrequency();
	cout << "config read in " << sec << " seconds" << endl;
	
	cout << "Press <enter> to exit" << endl;
	string str;
	getline(std::cin, str);
	return 0;
}

bool queryDbSingle(sqlite3* dbHandle, const std::string& sql, std::string& value) {
	using namespace std;

	bool success = false;
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare_v2(dbHandle, sql.c_str(), -1, &stmt, 0);
	if (rc == SQLITE_OK)
	{
		int step = SQLITE_ERROR;
		int nRow = 0; 
		do 
		{
			step = sqlite3_step(stmt);
			
			switch (step) {
			case SQLITE_ROW: 
				{
					// one result expected: take first row only and discard others
					if (nRow == 0)
					{
						int nCol = sqlite3_column_count(stmt);
						nCol = 0; // one result expected: take first column only
						if (sqlite3_column_type(stmt, nCol) == SQLITE_NULL) 
							cerr << __LINE__ << " NULL value in table" << endl;
						else
							value = (const char*)sqlite3_column_text(stmt, nCol);
					}
				}
				break;
			case SQLITE_DONE: break;
			default: 
				cerr << __LINE__ << "Error executing step statement" << endl;
				break;
			}
			++nRow;
		} while (step != SQLITE_DONE);
		
		rc = sqlite3_finalize(stmt);			
		if (rc == SQLITE_OK)
			success = true;
		else
			success = false;
	} 

	else // sqlite3_prepare != OK
	{
		cerr << "SQL error: " << sqlite3_errmsg(dbHandle) << endl;
		rc = sqlite3_finalize(stmt);
		success = false;
	}
	return success;
}


