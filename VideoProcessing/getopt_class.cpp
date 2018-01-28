#include <cstring>
#include "../../../cpp/inc/getopt_class.h"

int Opt::getOpt(int argc, char* argv[], char* optstr) {
	//int ret = 0;
	if (mOptInd >= argc)
		return -1;
	char *optch, *colon;
	mOptArg = nullptr;
	if (*argv[mOptInd] == '-') {
		++argv[mOptInd];
		colon = optch = strchr(optstr, *argv[mOptInd]);
		++mOptInd;
		if (optch != nullptr) {
			++colon;
			if (*colon == ':') {
				if (mOptInd < argc) // else: mOptArg = nullptr = no opt arg
					mOptArg = argv[mOptInd];
			}
			return (int)*optch;
		}
	}
	else {
		++mOptInd; }
	return 0;
}


bool Opt::parseCmdLine(int ac, char* av[], char* optDesc) {
	mOptInd = 1;
	char c;
	while ((c = getOpt(ac, av, optDesc)) != -1) {
		if (c) {
			if (mOptArg)
				mOptMap.insert(TOpt(c, mOptArg));
			else
				mOptMap.insert(TOpt(c, ""));
		}
	}
	return true;
}
	
TOptMap& Opt::getOptMap()  { 	return mOptMap; }
