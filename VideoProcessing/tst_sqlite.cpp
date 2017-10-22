#include "stdafx.h"
#include "KeyPress.h"
#include "../include/Parameters.h"
#include "sqlite/sqlite3.h"


using namespace std;

//static int callback(void *notUsed)

int sqlite_main (int argc, char* argv[])
{
	cout << "test sqlite" << endl;
	// mda.sqlite öffnen
	sqlite3 *db;
	int rc;

	rc = sqlite3_open("../VideoProcessing/sqlite/mda.sqlite", &db);
	if (rc)
	{
		cout << "Datenbank kann nicht geöffnet werden" << endl;
		sqlite3_close(db);
		return 1;
	}

	// ToDo: Abfrage ausführen
	//rc = sqlite3_exec(db, "PRAGMA table_info(MachData1)", callback, 0, &errMsg)

	sqlite3_close(db);
		
	// Konsole offen lassen
	char c = 0;
	do
	{
		cout << "Abbruch mit Leertaste" << endl;
		c = _getch();
	} while (c != 32);
	return 0;
}