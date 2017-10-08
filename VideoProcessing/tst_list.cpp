#include "stdafx.h"


using namespace std;


class Track {
public:
	Track(int _x = 10, int _y = 20);
	int x;
	int y;
	Track *pTrack;
	Track* GetPointer();
};

Track::Track(int _x, int _y)
{
	x = _x;
	y = _y;
	pTrack = this;
}

Track* Track::GetPointer()
{
	return this;
}

class Vehicle {
public:
	list<Track*> pTracks;
};


int tst_list_main(int argc, char* argv[])
{
	list<Track> lstTracks;
	for (int i = 1; i < 5; ++i)
		lstTracks.push_back(Track(i,i+10));
	list<Track>::iterator it = lstTracks.begin();
	while (it != lstTracks.end())
	{
		cout << "x: " << it->x << " y: " << it->y << endl;
		++it;
	}
	Vehicle veh;
	veh.pTracks.push_back(lstTracks.front().GetPointer());
	veh.pTracks.push_back(lstTracks.back().GetPointer());
	cout << endl << veh.pTracks.front()->x << endl;
	cout  << veh.pTracks.back()->x << endl << endl;

	list<Track*>::iterator pit = veh.pTracks.begin(); // (*pit) zeigt auf (Track*) wird mit -> auf Meber dereferenziert
	
	cout << (*pit)->x << endl;
	++pit;
	cout << (*pit)->x << endl;


	Track *pTrack = lstTracks.front().GetPointer();
	cout << pTrack->x << " len: " << lstTracks.size() << endl;
	--it;
	pTrack = it->GetPointer();
	cout << pTrack->x << " len: " << lstTracks.size() << endl;
	--it;
	it = lstTracks.erase(it);
	cout << pTrack->x << " len: " << lstTracks.size() << endl;

	char c;
    do
    {
        printf( "\nAbbruch mit Leertaste \n" );
        c = _getch();
    }
    while(c != 32);

    return 0;
}