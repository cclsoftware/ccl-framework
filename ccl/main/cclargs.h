//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/main/cclargs.h
// Description : Commandline Arguments
//
//************************************************************************************************

#ifndef _ccl_args_h
#define _ccl_args_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// PlatformArgs
//************************************************************************************************

struct PlatformArgs
{
	DEFINE_ENUM (Type)
	{
		kChar,
		kUchar
	};

	Type type;
	int argc;
	union
	{
		char** argvChar;
		uchar** argvUchar;
	};
	
	PlatformArgs (int _argc = 0, char** _argv = nullptr)
	: type (kChar),
	  argc (_argc),
	  argvChar (_argv)
	{}
	
	PlatformArgs (int _argc, uchar** _argv)
	: type (kUchar),
	  argc (_argc),
	  argvUchar (_argv)	  
	{}
};

//************************************************************************************************
// ArgumentList
//************************************************************************************************

class ArgumentList
{
public:
	ArgumentList (int _count = 0, String* _args = nullptr)
	: _count (_count),
	  _args (_args)
	{}

	int count () const
	{ return _count; }
	
	StringRef at (int index) const
	{ return index >= 0 && index < _count ? _args[index] : String::kEmpty; }

	StringRef operator[] (int index) const
	{ return at (index); }

	void toString (String& arguments) const;

protected:
	int _count;
	String* _args;
};

//************************************************************************************************
// MutableArgumentList
//************************************************************************************************

class MutableArgumentList: public ArgumentList
{
public:
	MutableArgumentList (int argc, String argv[]);
	MutableArgumentList (int argc, uchar* argv[]);
	MutableArgumentList (int argc, char* argv[]);
	MutableArgumentList (ArgsRef args);
	MutableArgumentList (const MutableArgumentList& args);
	MutableArgumentList (StringRef arguments);
	MutableArgumentList (const PlatformArgs& args);
	~MutableArgumentList ();

	MutableArgumentList& operator = (ArgsRef args);
	MutableArgumentList& operator = (const MutableArgumentList& args);

protected:
	void parse (StringRef arguments);
	void copyFrom (ArgsRef args);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

// arguments the application was called with
extern const ArgumentList* g_ArgumentList;

} // namespace CCL

#endif // _ccl_args_h
