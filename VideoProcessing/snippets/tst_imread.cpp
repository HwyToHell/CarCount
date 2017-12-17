#include "stdafx.h"
#include "tracking.h"


using namespace std;
using namespace cv;



	


int tst_imread_main(int argc, char* argv[])
{
    string fgMaskFile = "..\\Dataset\\f200.jpg";
	Mat fgMask;
	fgMask = imread(fgMaskFile);
	namedWindow("FGMask");
	imshow("FGMask", fgMask);
	waitKey(0);
	return 0;
}