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
// Filename    : ccl/public/gui/framework/icommandtable.h
// Description : Command Table Interface
//
//************************************************************************************************

#ifndef _ccl_icommandtable_h
#define _ccl_icommandtable_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

class FileType;
struct KeyEvent;
struct CommandMsg;
interface ICommandHandler;
interface IParameter;
interface IUnknownIterator;
interface ICommandFilter;
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (CommandContainer, 0xD0271918, 0xF7B0, 0x4CB4, 0x9A, 0xAB, 0x96, 0x93, 0xDE, 0x78, 0x13, 0x9A);
}

//************************************************************************************************
// CommandDescription
/**
	\ingroup gui_command */
//************************************************************************************************

struct CommandDescription
{
	MutableCString category;
	MutableCString name;
	MutableCString arguments;
	String displayCategory;
	String displayName;
	MutableCString englishName;
	UIDBytes classID;
	int flags;

	CommandDescription (StringID category = nullptr,
						StringID name = nullptr,
						StringRef displayCategory = nullptr,
						StringRef displayName = nullptr,
						int flags = 0)
	: category (category),
	  name (name),
	  displayCategory (displayCategory),
	  displayName (displayName),
	  flags (flags),
	  classID (kNullUID)
	{}
	
	bool isValid () const { return !name.isEmpty () && !category.isEmpty (); }
};

//************************************************************************************************
// ICommand
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommand: IUnknown
{
	/** Get command description. */
	virtual void CCL_API getDescription (CommandDescription& description) const = 0;

	/** Create iterator of IKeyBinding objects associated with this command. */
	virtual IUnknownIterator* CCL_API newBindingIterator () const = 0;

	DECLARE_IID (ICommand)
};

DEFINE_IID (ICommand, 0x2286c052, 0x8edf, 0x44a2, 0x9d, 0x16, 0xb6, 0x19, 0x40, 0xa5, 0x15, 0x24)

//************************************************************************************************
// IKeyBinding
/**
	\ingroup gui_command */
//************************************************************************************************

interface IKeyBinding: IUnknown
{
	/** Get key description. */
	virtual void CCL_API copyTo (KeyEvent& key) const = 0;

	DECLARE_IID (IKeyBinding)
};

DEFINE_IID (IKeyBinding, 0x392041b2, 0x3f81, 0x43c7, 0xb8, 0xf7, 0x79, 0x9a, 0xcb, 0x36, 0xd1, 0xba)

//************************************************************************************************
// ICommandCategory
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandCategory: IUnknown
{
	/** Get category string for display. */
	virtual StringRef CCL_API getDisplayCategory () const = 0;

	/** Create iterator of ICommand objects in this category. */
	virtual IUnknownIterator* CCL_API newCommandIterator () const = 0;

	DECLARE_IID (ICommandCategory)
};

DEFINE_IID (ICommandCategory, 0xa03104b4, 0xa4a, 0x4e91, 0x8b, 0x17, 0x72, 0x5f, 0xda, 0x24, 0xd6, 0xa8)

//************************************************************************************************
// ICommandContainer
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandContainer: IUnknown
{
	/** Create iterator of unique ICommandCategory objects. */
	virtual IUnknownIterator* CCL_API newCategoryIterator () const = 0;

	/** Find command object by category & name. */
	virtual ICommand* CCL_API findCommand (StringID category, StringID name) const = 0;

	/** Add commands from command table. */
	virtual void CCL_API addBuiltInCommands () = 0;

	/** Load category / commands tree from attributes. */
	virtual tbool CCL_API loadCommands (const IAttributeList& attributes) = 0;

	/** Save category / commands tree. */
	virtual tbool CCL_API saveCommands (IAttributeList& attributes) const = 0;

	DECLARE_IID (ICommandContainer)
};

DEFINE_IID (ICommandContainer, 0xA63C33BE, 0x396D, 0x4A75, 0xAD, 0xB8, 0x48, 0x92, 0x00, 0x7B, 0x60, 0x89)

//************************************************************************************************
// ICommandTable
/** Command Table - Access singleton instance via System::GetCommandTable ()
	\ingroup gui_command */
//************************************************************************************************

interface ICommandTable: IUnknown
{
	/** Hint for loadCommands() method. */
	enum CommandLoadHint
	{
		kReplaceAll,
		kOverwriteExisting,
		kKeepExisting,
		kDefineKnown
	};

	DECLARE_STRINGID_MEMBER (kCommandsLoaded) ///< signaled when commands have been loaded
	DECLARE_STRINGID_MEMBER (kBeginTransaction) ///< signaled when transaction starts - msg[0] : transaction title
	DECLARE_STRINGID_MEMBER (kEndTransaction) ///< signaled when transaction ends

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Command Load/Save
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Load commands from file. */
	virtual tbool CCL_API loadCommands (UrlRef path, int loadHint) = 0;

	/** Load list of commands to be ignored on load. */
	virtual tbool CCL_API loadBlocklist (UrlRef path) = 0;

	/** Save commands to file. */
	virtual tbool CCL_API saveCommands (UrlRef path) const = 0;

	/** Get name of active command scheme loaded from file. */
	virtual StringRef CCL_API getActiveSchemeName (tbool* modified = nullptr) const = 0;

	/** Get file type of command scheme files. */
	virtual const FileType& CCL_API getCommandFileType () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Command Handling
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Add global command handler. */
	virtual void CCL_API addHandler (ICommandHandler* handler) = 0;

	/** Remove global command handler. */
	virtual void CCL_API removeHandler (ICommandHandler* handler) = 0;

	/** Perform given command. */
	virtual tbool CCL_API performCommand (const CommandMsg& msg, tbool deferred = false) = 0;

	/** Get a parameter that triggers the given command. Parameters are owned by the table. */
	virtual IParameter* CCL_API getCommandParam (StringID category, StringID name, tbool create = true) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Command Registration
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Register command. */
	virtual void CCL_API registerCommand (const CommandDescription& description) = 0;

	/** Find registered command. */
	virtual ICommand* CCL_API findCommand (StringID category, StringID name) const = 0;

	/** Create iterator of registered ICommand objects (flat). */
	virtual IUnknownIterator* CCL_API newCommandIterator () const = 0;

	/** Create iterator of unique ICommandCategory objects. */
	virtual IUnknownIterator* CCL_API newCategoryIterator () const = 0;

	/** Unregister command. */
	virtual void CCL_API unregisterCommand (StringID category, StringID name) = 0;

	/** Assign a key to a command . */
	virtual void CCL_API assignKey (ICommand& command, const KeyEvent& key) = 0;

	/** Create iterator of IKeyBinding objects for the given command. */
	virtual IUnknownIterator* CCL_API lookupKeyBindings (const ICommand& command) const = 0;

	/** Add an alias name for a command. */
	virtual void CCL_API addCommandAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Command Execution Filter
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Add global command filter. */
	virtual void CCL_API addFilter (ICommandFilter* filter) = 0;

	/** Remove global command filter. */
	virtual void CCL_API removeFilter (ICommandFilter* filter) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Command Transactions (multiple commands run in sequence)
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Begin Transaction */
	virtual void CCL_API beginTransaction (StringRef title) = 0;

	/** End Transaction */
	virtual void CCL_API endTransaction () = 0;

	/** Returns true of transaction is active*/
	virtual tbool CCL_API isTransactionActive () const = 0;

	DECLARE_IID (ICommandTable)
};

DEFINE_IID (ICommandTable, 0xa5dbfb97, 0xff0d, 0x4ae2, 0xb2, 0xe7, 0xe7, 0xac, 0x21, 0x37, 0x5e, 0xf2)
DEFINE_STRINGID_MEMBER (ICommandTable, kCommandsLoaded, "CommandsLoaded")
DEFINE_STRINGID_MEMBER (ICommandTable, kBeginTransaction, "BeginTransaction")
DEFINE_STRINGID_MEMBER (ICommandTable, kEndTransaction, "EndTransaction")

} // namespace CCL

#endif // _ccl_icommandtable_h
