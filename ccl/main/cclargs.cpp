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
// Filename    : ccl/main/cclargs.cpp
// Description : Commandline Arguments
//
//************************************************************************************************

#include "ccl/main/cclargs.h"

using namespace CCL;

//************************************************************************************************
// ArgumentList
//************************************************************************************************

void ArgumentList::toString (String& arguments) const
{
	arguments.empty ();
	for(int i = 0; i < count (); i++)
	{
		if(!arguments.isEmpty ())
			arguments.append (" ");

		StringRef argument = at (i);
		if(!argument.contains ("\t") && !argument.contains ("\n") && !argument.contains ("\v") && !argument.contains ("\"") && !argument.contains (" "))
		{
			arguments.append (argument);
			continue;
		}

		arguments.append ("\"");
		for(int charIndex = 0; charIndex < argument.length (); charIndex++)
		{
			int backslashCount = 0;
			uchar currentChar = argument.at (charIndex);
			while(charIndex < argument.length () && currentChar == '\\')
			{
				charIndex++;
				currentChar = argument.at (charIndex);
				backslashCount++;
			}
			if(charIndex == argument.length ())
			{
				arguments.append (String ("\\"), backslashCount * 2);
				break;
			}
			if(currentChar == '"')
			{
				arguments.append (String ("\\"), backslashCount * 2 + 1);
				arguments.append ("\"");
			}
			else
			{
				arguments.append (String ("\\"), backslashCount);
				arguments.append (&currentChar, 1);
			}
		}
		arguments.append ("\"");
	}
}

//************************************************************************************************
// MutableArgumentList
//************************************************************************************************

MutableArgumentList::MutableArgumentList (int argc, String argv[])
{
	if(argc > 0)
	{
		_count = argc;
		_args = NEW String[argc];
		for(int i = 0; i < argc; i++)
			_args[i] = argv[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (int argc, uchar* argv[])
{
	if(argc > 0)
	{
		_count = argc;
		_args = NEW String[argc];
		for(int i = 0; i < argc; i++)
			_args[i] = argv[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (int argc, char* argv[])
{
	if(argc > 0)
	{
		_count = argc;
		_args = NEW String[argc];
		for(int i = 0; i < argc; i++)
			_args[i] = argv[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (const MutableArgumentList& args)
{
	copyFrom (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (ArgsRef args)
{
	copyFrom (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (StringRef arguments)
{
	parse (arguments);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::MutableArgumentList (const PlatformArgs& args)
{
	if(args.argc > 0)
	{
		_count = args.argc;
		_args = NEW String[args.argc];
		if(args.type == PlatformArgs::kChar)
		{
			for(int i = 0; i < args.argc; i++)
				_args[i] = args.argvChar[i];
		}
		else
		{
			for(int i = 0; i < args.argc; i++)
				_args[i] = args.argvUchar[i];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList::~MutableArgumentList ()
{
	if(_args)
		delete [] _args;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MutableArgumentList::copyFrom (ArgsRef args)
{
	if(_args)
		delete [] _args,
		_args = nullptr;

	_count = args.count ();
	if(_count > 0)
	{
		_args = NEW String[_count];
		for(int i = 0; i < _count; i++)
			_args[i] = args[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList& MutableArgumentList::operator = (ArgsRef args)
{
	copyFrom (args);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableArgumentList& MutableArgumentList::operator = (const MutableArgumentList& args)
{
	copyFrom (args);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MutableArgumentList::parse (StringRef arguments)
{
	// simple separation by space char for now, no quotes etc.
	const String kArgumentSeparators (" ");
	uchar delimiter = 0;
	if(AutoPtr<IStringTokenizer> tokenizer = arguments.tokenize (kArgumentSeparators))
		while(!tokenizer->done ())
		{
			tokenizer->nextToken (delimiter);
			_count++;
		}
	if(_count > 0)
		_args = NEW String[_count];

	int argIndex = 0;
	if(AutoPtr<IStringTokenizer> tokenizer = arguments.tokenize (kArgumentSeparators))
		while(!tokenizer->done ())
			_args[argIndex++] = tokenizer->nextToken (delimiter);
}
