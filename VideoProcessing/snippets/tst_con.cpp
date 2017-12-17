#include "stdafx.h"
#include "KeyPress.h"

using namespace std;


void tst_con_main()
{

	cv::Rect rc1(0, 0, 100, 50);
	cv::Rect rc2(50, 25, 100, 50);
	cout << rc1 << endl << rc2 << endl << endl;
	cout << (rc1 | rc2);


	char c;
    do
    {
        printf( "\nAbbruch mit Leertaste \n" );
        c = _getch();
    }
    while(c != 32);

    return;
}






