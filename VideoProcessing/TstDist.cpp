#include <opencv2/opencv.hpp>
#include <iostream>
#include <conio.h>


using namespace std;
using namespace cv;


ostream& operator<<(ostream& out, vector<vector<double>> mat)
{
	for (unsigned int i = 0; i < mat.size(); i++)
	{
		out << "row " << i << " "; 
		for (unsigned int n = 0; n < mat[i].size(); n++)
			out << mat[i][n] << ", ";
		out << endl;
	}
	return out; 
}


void prnIdxMat(vector<int> idx)
{// print index matrix
	cout << endl;
	for (unsigned int i = 0; i < idx.size(); i++)
	{
		for (unsigned int n = 0; n < idx.size(); n++)
			if (n == idx[i])
				cout << 1;
			else
				cout << 0;
		cout << endl;
	}

}

double SumDst(vector<vector<double>> mat, vector<int> actTrkIdxs)
{
	double sum = 0;
	int nBlobs = mat.size();
	for (int blob = 0; blob < nBlobs; blob++)
	{
		int track = actTrkIdxs[blob];
		sum += mat[blob][track];
	}
	return sum;
}

int dist_main(int argc, char* argv[])
{
	vector<vector<double>> DstBlobToTrack;
	vector<double> temp;
	const int dimension = 4;

	for (int i = 0; i < dimension; i++)
	{
		for (int n = 0; n < dimension; n++)
			temp.push_back(i*10+n);
		DstBlobToTrack.push_back(temp);
		temp.clear();
	}

	vector<int> trkIdx(dimension, 0);
	for (unsigned int i = 0; i < trkIdx.size(); i++)
		trkIdx[i] = i;

	cout << "DstBlobToTrack Matrix:" << endl;
    cout << DstBlobToTrack;	
	cout << "Sum: " << SumDst(DstBlobToTrack, trkIdx) << endl;

	// update index
	vector<int> variations; 
	int maxLevel = DstBlobToTrack.size();
	int maxTrkIdx = DstBlobToTrack[0].size();
	// fill vector
	for (int i = 0; i < maxLevel; i++)
		variations.push_back(i);
	int level = 1;

	do
	{
		if (variations[level] == 0)
		{
			variations[level] = level;
			level++;
		}
		else
		{

			prnIdxMat(trkIdx);
			cout << "level: " << level << endl;
			cout << "variations: " << variations[level] << endl;

			int temp = trkIdx[level-1];
			trkIdx[level-1] = trkIdx[level];
			trkIdx[level] = temp;
			variations[level]--;
			if (level > 1)
				//level--;
				level = 1;

		}


	} while (level < maxLevel) ;
	prnIdxMat(trkIdx);


	char c;
    do
    {
        printf( "\nAbbruch mit Leertaste \n" );
        c = _getch();
    }
    while(c != 32);

    return 0;

}