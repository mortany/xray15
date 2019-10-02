// TestPluginInits.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "../../../../xrCore/xrCore.h"
#include <iostream>
#include <conio.h>
using namespace std;


#define _HAS_EXCEPTIONS	0

#pragma warning(disable:4995)

#pragma comment	(lib,"xrCoreStatic.lib")

int main()
{

	Core._initialize(TEXT("S.T.A.L.K.E.R.Plugin"), 0, FALSE);
	FS._initialize(CLocatorAPI::flScanAppRoot, NULL, TEXT("xray_path.ltx"));

	do
	{
		cout << '\n' << "Press a key to continue...";
	} while (cin.get() != '\n');

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
