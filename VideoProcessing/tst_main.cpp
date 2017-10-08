#define CATCH_CONFIG_RUNNER 
#include "..\..\..\catch\catch.hpp"
#include <conio.h>


int _main(int argc, char* argv[])
{
	char *_argv[2];
	_argv[0] = argv[0];
	int _argc = 1;
	//_argv[1] = "-s"; ++_argc;


	
	int result = Catch::Session().run( _argc, _argv );

	char c;
    do
    {
        printf( "\nAbbruch mit Leertaste \n" );
        c = _getch();
    }
    while(c != 32);

    return 0;
}