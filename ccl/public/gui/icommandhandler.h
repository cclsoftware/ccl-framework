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
// Filename    : ccl/public/gui/icommandhandler.h
// Description : Command Handler Interface
//
//************************************************************************************************

#ifndef _ccl_icommandhandler_h
#define _ccl_icommandhandler_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Command Flags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CommandFlags
{
	enum Flags
	{ 
		kGlobal   = 1<<0,	///< command is also allowed in modal dialogs
		kNoRepeat = 1<<1,	///< command does not want key repeats
		kHidden   = 1<<2	///< command should not be displayed in editor 
	};
};

//************************************************************************************************
// CommandMsg
/**
	\ingroup gui_command */
//************************************************************************************************

struct CommandMsg
{
	enum Flags
	{ 
		kCheckOnly = 1<<0	///< test if command is executable
	};

	CString category;		///< command category (e.g. "File")
	CString name;			///< command name (e.g. "Open")
	IUnknown* invoker;		///< command invoker
	int flags;				///< command flags (see above)

	CommandMsg (CStringRef category = nullptr, 
				CStringRef name = nullptr,
				IUnknown* invoker = nullptr, 
				int flags = 0)
	: category (category),
	  name (name),
	  invoker (invoker),
	  flags (flags)
	{}

	bool checkOnly () const { return (flags & kCheckOnly) != 0; }
};

/** Command arguments. */
typedef const CommandMsg& CmdArgs;

//************************************************************************************************
// CommandWithTitle
/**
	\ingroup gui_command */
//************************************************************************************************

struct CommandWithTitle
{
	CString category;
	CString name;
	String title;

	CommandWithTitle (CStringRef category = nullptr, CStringRef name = nullptr, StringRef title = nullptr)
	: category (category),
	  name (name),
	  title (title)
	{}
};

//************************************************************************************************
// ICommandHandler
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandHandler: IUnknown
{
	virtual tbool CCL_API checkCommandCategory (CStringRef category) const = 0;

	virtual tbool CCL_API interpretCommand (const CommandMsg& msg) = 0;

	DECLARE_IID (ICommandHandler)
};

DEFINE_IID (ICommandHandler, 0xcb8108a9, 0xdc88, 0x4152, 0xad, 0xe0, 0xc3, 0xae, 0x48, 0x9d, 0x75, 0xb5)

//************************************************************************************************
// ICommandFilter
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandFilter: IUnknown
{
	/** Check if command execution is allowed */
	virtual tbool CCL_API isCommandAllowed (const CommandMsg& msg) = 0;
	
	DECLARE_IID (ICommandFilter)
};

DEFINE_IID (ICommandFilter, 0x0fc6ed54, 0x21f, 0x4f9e, 0x94, 0x1, 0x34, 0x29, 0xb7, 0x4, 0x50, 0x43)

} // namespace CCL

#endif // _ccl_icommandhandler_h
