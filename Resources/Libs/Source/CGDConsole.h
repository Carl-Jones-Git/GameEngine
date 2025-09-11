
//
// CGDConsole.h
//

// CGDConsole represents a (singleton) console window

#pragma once

//#include <GUObject.h>

class CGDConsole{// : public GUObject {

	// Flag to determine if AllocConsole was successful or not
	bool _allocSuccessful = false;

	// Standard input / output file handles
	FILE *stdinFile = nullptr;
	FILE *stdoutFile = nullptr;
	FILE *stderrFile = nullptr;


	//
	// Private interface
	//

	// Private constructor
	CGDConsole(const wchar_t* consoleTitle);

public:

	//
	// Public interface
	//

	// Console factory method
	static CGDConsole* CreateConsole(const wchar_t* consoleTitle);

	// Destructor
	~CGDConsole();
};
