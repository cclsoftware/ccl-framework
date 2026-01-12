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
// Filename    : ccl/gui/commands.h
// Description : Key Commands
//
//************************************************************************************************

#ifndef _ccl_commands_h
#define _ccl_commands_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

struct KeyEvent;
class SignalSink;
class ParamContainer;
class LogBuffer;
class CommandFile;
class CommandContainer;
class Attributes;
interface IParameter;

//************************************************************************************************
// CommandMsgEx
/** Command message extension with Unicode to ASCII conversion. */
//************************************************************************************************

struct CommandMsgEx: CommandMsg
{
	char categoryBuffer[256];
	char nameBuffer[256];

	CommandMsgEx (StringRef category = nullptr,
				  StringRef name = nullptr,
				  IUnknown* invoker = nullptr,
				  int flags = 0);

	void setCategory (StringRef category);
	void setName (StringRef name);

	void setCategory (StringID category);
	void setName (StringID name);
};

//************************************************************************************************
// Command
//************************************************************************************************

class Command: public Object,
			   public ICommand
{
public:
	DECLARE_CLASS (Command, Object)

	Command (StringID category = nullptr, StringID name = nullptr);
	Command (const CommandMsg& msg);

	PROPERTY_MUTABLE_CSTRING (category, Category)
	PROPERTY_MUTABLE_CSTRING (name, Name)

	PROPERTY_VARIABLE (int, rolloutVersion, RolloutVersion)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, CommandFlags::kGlobal, isGlobal)
	PROPERTY_FLAG (flags, CommandFlags::kNoRepeat, noRepeat)

	bool hasBindings () const;
	Iterator* getBindings () const;
	KeyEvent* getDefaultKey (bool layoutSensitive = true) const;
	bool isKeyAssigned (const KeyEvent& key, bool layoutSensitive = true) const;
	void assignKey (const KeyEvent& key, bool exclusive = false);
	void copyKeys (const Command& other);
	void mergeKeys (const Command& other);
	bool removeKey (const KeyEvent& key);
	bool removeKeyAt (int index);
	void removeKeys ();

	bool interpretSafe (ICommandHandler* defaultHandler = nullptr) const;

	// ICommand
	void CCL_API getDescription (CommandDescription& description) const override;
	IUnknownIterator* CCL_API newBindingIterator () const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool equals (const Object& obj) const override;

	CLASS_INTERFACE (ICommand, Object)

protected:
	ObjectList bindings;

	bool canInterpret (ICommandHandler* defaultHandler = nullptr) const;
	bool interpret (ICommandHandler* defaultHandler = nullptr) const;
};

//************************************************************************************************
// KnownCommand
//************************************************************************************************

class KnownCommand: public Command
{
public:
	DECLARE_CLASS (KnownCommand, Command)

	KnownCommand (const CommandDescription& description = CommandDescription ());

	PROPERTY_MUTABLE_CSTRING (arguments, Arguments)
	PROPERTY_STRING (displayCategory, DisplayCategory)
	PROPERTY_STRING (displayName, DisplayName)
	PROPERTY_MUTABLE_CSTRING (englishName, EnglishName)
	PROPERTY_OBJECT (UID, classID, ClassID)

	String getTitle () const; ///< "Category - Name"

	// Command
	void CCL_API getDescription (CommandDescription& description) const override;
	int compare (const Object& obj) const override;

protected:
	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// CommandCategory
//************************************************************************************************

class CommandCategory: public Object,
					   public ICommandCategory
{
public:
	DECLARE_CLASS (CommandCategory, Object)
	DECLARE_METHOD_NAMES (CommandTable)

	CommandCategory (StringRef title = nullptr);

	PROPERTY_STRING (title, Title)

	void add (Command* command, bool share);
	Iterator* newIterator () const;

	// ICommandCategory
	StringRef CCL_API getDisplayCategory () const override;
	IUnknownIterator* CCL_API newCommandIterator () const override;

	// Object
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;

	CLASS_INTERFACE (ICommandCategory, Object)

protected:
	ObjectList commands;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// CommandTable
//************************************************************************************************

class CommandTable: public Object,
					public ICommandTable,
					public Singleton<CommandTable>
{
public:
	DECLARE_CLASS (CommandTable, Object)
	DECLARE_METHOD_NAMES (CommandTable)

	CommandTable ();
	~CommandTable ();

	// ICommandTable
	tbool CCL_API loadCommands (UrlRef url, int loadHint) override;
	tbool CCL_API loadBlocklist (UrlRef path) override;
	tbool CCL_API saveCommands (UrlRef path) const override;
	StringRef CCL_API getActiveSchemeName (tbool* modified = nullptr) const override;
	const FileType& CCL_API getCommandFileType () const override;
	void CCL_API addHandler (ICommandHandler* handler) override;
	void CCL_API removeHandler (ICommandHandler* handler) override;
	tbool CCL_API performCommand (const CommandMsg& msg, tbool deferred = false) override;
	IParameter* CCL_API getCommandParam (StringID category, StringID name, tbool create = true) override;
	void CCL_API registerCommand (const CommandDescription& description) override;
	ICommand* CCL_API findCommand (StringID category, StringID name) const override;
	IUnknownIterator* CCL_API newCommandIterator () const override;
	IUnknownIterator* CCL_API newCategoryIterator () const override;
	void CCL_API unregisterCommand (StringID category, StringID name) override;
	void CCL_API addFilter (ICommandFilter* filter) override;
	void CCL_API removeFilter (ICommandFilter* filter) override;
	void CCL_API assignKey (ICommand& command, const KeyEvent& key) override;
	IUnknownIterator* CCL_API lookupKeyBindings (const ICommand& command) const override;
	void CCL_API beginTransaction (StringRef title) override;
	void CCL_API endTransaction () override;
	tbool CCL_API isTransactionActive () const override;
	void CCL_API addCommandAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName) override;

	StringID getActiveLayout () const; ///< get active keyboard layout

	KeyEvent* lookupKeyEvent (const Command& command, bool layoutSensitive = true) const;
	Iterator* lookupBindings (const Command& command) const;
	Command* lookupCommand (const KeyEvent& key, bool layoutSensitive = true) const;
	
	bool translateKey (const KeyEvent& key, ICommandHandler* defaultHandler = nullptr) const;
	bool interpretCommand (const CommandMsg& msg, ICommandHandler* defaultHandler = nullptr) const;
	void deferCommand (StringID category, StringID name, Attributes* args = nullptr);
	bool isCommandAllowed (const CommandMsg& msg) const;
	bool resolveCommandAlias (CommandMsg& resultMsg, const CommandMsg& aliasMsg) const;

	CommandContainer* createCategories () const;
	void getCategories (CommandContainer& container) const;

	const CommandMsgEx& getLastCommand () const;
	void setLastCommand (const CommandMsg& msg);

	PROPERTY_VARIABLE (int, version, Version)

	void load (CommandFile& file, int loadHint);
	void save (CommandFile& file) const;

	void dump () const;					///< write commands to debug output
	void dumpAvailableKeys () const;	///< write available key combinations to debug output

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	SignalSink& localeSink;
	SignalSink& errorSink;
	LogBuffer& logBuffer;
	mutable MutableCString activeLayout;
	ObjectList commands;
	ObjectList knownCommands;
	CommandMsgEx lastCommand;
	LinkedList<ICommandHandler*> globalHandlers;
	ParamContainer* commandParams;
	String activeSchemeName;
	bool activeSchemeModified;
	bool commandRegistrationChanged;
	int transactionActiveCounter;
	CommandFile* blocklist;
	LinkedList<ICommandFilter*> filters;
	CommandAliasTable aliasTable;

	bool matchesBlocklist (const Command& command) const;
	void setCommand (const Command& command);
	void updateMenuKeys ();
	void commandsChanged ();

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// CommandFile
//************************************************************************************************

class CommandFile: public StorableObject
{
public:
	DECLARE_CLASS (CommandFile, StorableObject)

	CommandFile ();

	PROPERTY_STRING (name, Name)
	PROPERTY_BOOL (modified, Modified)
	PROPERTY_VARIABLE (int, version, Version)

	Container* operator -> () { return &commands; }

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectList commands;
};

//************************************************************************************************
// CommandContainer
//************************************************************************************************

class CommandContainer: public Object,
						public ICommandContainer
{
public:
	DECLARE_CLASS (CommandContainer, Object)

	CommandContainer ();

	KnownCommand* addCommand (const KnownCommand& c);
	Iterator* newIterator () const; ///< of CommandCategory

	// ICommandContainer
	IUnknownIterator* CCL_API newCategoryIterator () const override;
	ICommand* CCL_API findCommand (StringID category, StringID name) const override;
	void CCL_API addBuiltInCommands () override;
	tbool CCL_API loadCommands (const IAttributeList& attributes) override;
	tbool CCL_API saveCommands (IAttributeList& attributes) const override;

	CLASS_INTERFACE (ICommandContainer, Object)

private:
	ObjectList categories;

	CommandCategory* addCategory (StringRef title);
};

} // namespace CCL

#endif // _ccl_commands_h
