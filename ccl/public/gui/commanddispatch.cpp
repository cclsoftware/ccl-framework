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
// Filename    : ccl/public/gui/commanddispatch.cpp
// Description : Command Dispatcher
//
//************************************************************************************************

#include "ccl/public/gui/commanddispatch.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// CommandAutomator
//************************************************************************************************

IAttributeList* CommandAutomator::getArguments (CmdArgs args)
{
	// LATER TODO: ICommandAutomator interface!
	return UnknownPtr<IAttributeList> (args.invoker);
}

//************************************************************************************************
// CommandAutomator::Arguments
//************************************************************************************************

CommandAutomator::Arguments::Arguments (CmdArgs args)
: arguments (CommandAutomator::getArguments (args))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandAutomator::Arguments::Arguments (IAttributeList* arguments)
: arguments (arguments)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::hasArguments () const
{
	return arguments != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, T (Variant::*assign) () const>
inline bool CommandAutomator::Arguments::getValue (StringID id, T& value) const
{
	Variant v;
	if(arguments && arguments->getAttribute (v, id))
	{
		value = (v.*assign)();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getValue (StringID id, Variant& value) const
{
	return arguments && arguments->getAttribute (value, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getInt (StringID id, int& value) const
{
	return getValue <int, &Variant::parseInt> (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getBool (StringID id, bool& value) const
{
	int intValue = value;
	bool result = getValue <int, &Variant::parseInt> (id, intValue);
	value = intValue != 0;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getFloat (StringID id, double& value) const
{
	return getValue <double, &Variant::parseDouble> (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getString (StringID id, String& value) const
{
	return getValue <String, &Variant::asString> (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getCString (StringID id, MutableCString& value) const
{
	String string;
	if(!getString (id, string))
		return false;

	value = string;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandAutomator::Arguments::getInt (StringID id) const
{
	int value = 0; getInt (id, value); return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAutomator::Arguments::getBool (StringID id) const
{
	return getInt (id) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CommandAutomator::Arguments::getFloat (StringID id) const
{
	double value = 0.; getFloat (id, value); return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CommandAutomator::Arguments::getString (StringID id) const
{
	String value; getString (id, value); return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString CommandAutomator::Arguments::getCString (StringID id) const
{
	MutableCString value; getCString (id, value); return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CommandAutomator::Arguments::getObject (StringID id) const
{
	Variant v;
	if(arguments)
		arguments->getAttribute (v, id);
	return v.asUnknown ();
}

//************************************************************************************************
// CommandRegistry::Entry
//************************************************************************************************

struct CommandRegistry::Entry
{
	CStringPtr category;
	CStringPtr name;
	CStringPtr arguments;
	int flags;

	Entry (CStringPtr category = nullptr,
		   CStringPtr name = nullptr,
		   CStringPtr arguments = nullptr,
		   int flags = 0)
	: category (category),
	  name (name),
	  arguments (arguments),
	  flags (flags)
	{}

	typedef LinkedList<Entry> List;

	static List& getList ()
	{
		static Deleter<List> theList (nullptr);
		if(!theList._ptr)
			theList._ptr = NEW List;
		return *theList._ptr;
	}
};

//************************************************************************************************
// CommandRegistry
//************************************************************************************************

void CommandRegistry::registerCommand (CStringPtr category, CStringPtr name, int flags, CStringPtr arguments)
{
	if(CString (name).isEmpty ())
		return;
	if(flags & CommandFlags::kHidden)
		return;
	Entry::getList ().append (Entry (category, name, arguments, flags));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandRegistry::registerAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName)
{
	System::GetCommandTable ().addCommandAlias (category, name, aliasCategory, aliasName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandRegistry::registerWithCommandTable ()
{
	LocalString::BeginScope beginScope ("Command");

	ListForEach (Entry::getList (), Entry, e)
		LocalString categoryString (e.category);
		LocalString nameString (e.name);
		CommandDescription description (e.category, e.name, categoryString, nameString, e.flags);
		description.englishName = e.name;
		description.arguments = e.arguments;
		System::GetCommandTable ().registerCommand (description);
	EndFor

	LocalString::EndScope endScope;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandRegistry::addToCommandTable (CStringPtr category, CStringPtr name, int flags, CStringPtr arguments)
{
	ASSERT ((flags & CommandFlags::kHidden) == 0)

	LocalString::BeginScope beginScope ("Command");
	LocalString categoryString (category);
	LocalString nameString (name);
	CommandDescription description (category, name, categoryString, nameString, flags);
	description.englishName = name;
	description.arguments = arguments;
	System::GetCommandTable ().registerCommand (description);
	LocalString::EndScope endScope;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandWithTitle CommandRegistry::find (CStringPtr category, CStringPtr name)
{
	if(ICommand* c = System::GetCommandTable ().findCommand (category, name))
	{
		CommandDescription description;
		c->getDescription (description);
		return CommandWithTitle (category, name, description.displayName);
	}
	else
		return CommandWithTitle (category, name, String (name)); // fallback to non-localized name
}

//************************************************************************************************
// CommandAliasTable 
//************************************************************************************************

void CommandAliasTable::addCategoryAlias (CStringPtr category, CStringPtr aliasCategory)
{
	aliases.add ({category, nullptr, aliasCategory, nullptr});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandAliasTable::addCommandAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName)
{
	aliases.add ({category, name, aliasCategory, aliasName});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandAliasTable::resolveCommandAlias (CommandMsg& resultMsg, const CommandMsg& aliasMsg) const
{
	// 1. find an exact alias for category + name
	if(AliasItem* alias = aliases.findIf ([&] (const AliasItem& a) { return a.aliasName == aliasMsg.name && a.aliasCategory == aliasMsg.category; }))
	{
		resultMsg = CommandMsg (alias->category, alias->name, aliasMsg.invoker, aliasMsg.flags);
		return true;
	}

	// 2. find a category alias (matches any command name)
	AliasItem* categoryAlias = aliases.findIf ([&] (const AliasItem& a) { return a.aliasName.isEmpty () && a.aliasCategory == aliasMsg.category; });
	if(categoryAlias)
	{
		resultMsg = CommandMsg (categoryAlias->category, aliasMsg.name, aliasMsg.invoker, aliasMsg.flags);
		return true;
	}
	return false;
}
