#include "stdafx.h"
#include "KeyPress.h"

/// Returns true, if key <iVirtKey> is pressed and KeyState has changed
bool KeyPressed(int iVirtKey)
{
	const int iKeyDownMask = 0x00000080;
	static int iKeyOld;
	int	iKey = GetKeyState(iVirtKey);
	if ((iKey & iKeyDownMask) != 0)
		if (iKey != iKeyOld)
		{
			iKeyOld = iKey;
			return true;
		}
	return false;
}

/// Place function in loop -> <Enter> stops loop and continues 
void StopStartEnter() 
{
	static bool isInitialized;
	if (!isInitialized)
	{
		isInitialized = true;
		cout <<  "Press <Enter> to stop" << endl;
	}
	if (KeyPressed(VK_RETURN))
	{
		cout << "Stopped: Press <Enter> to continue" << endl;
		while (!(KeyPressed(VK_RETURN)))
		{}
		cout <<  "Press <Enter> to stop" << endl;
	}
}

/// Place function in loop -> <Enter> stops loop and continues, <Righ Arrow> goes to next frame 
void StopStartNextFrame() 
{
	static bool isInitialized, stopped;

	if (!isInitialized)
	{
		isInitialized = true;
		cout <<  "Press <Enter> to stop" << endl;
	}

	if (stopped)
	{
		while (!(KeyPressed(VK_RIGHT)))
		{
			if (KeyPressed(VK_RETURN))
			{
				stopped = false;
				cout <<  "Press <Enter> to stop" << endl;
				break;
			}
		}
		if (stopped)
			cout << "next frame" << endl;

	}
	else // not stopped
	{
		if (KeyPressed(VK_RETURN))
		{
			cout << "Stopped: Press <Enter> to continue" << endl;
			cout <<  "or <Right Arrow Key> for next frame" << endl;
			stopped = true;
			while (!(KeyPressed(VK_RIGHT)))
			{
				if (KeyPressed(VK_RETURN))
				{
					stopped = false;
					cout <<  "Press <Enter> to stop" << endl;
					break;
				}
			}
			if (stopped)
				cout << "next frame" << endl;
		}
	}
}