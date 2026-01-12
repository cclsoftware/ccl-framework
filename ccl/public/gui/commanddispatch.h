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
// Filename    : ccl/public/gui/commanddispatch.h
// Description : Command Dispatcher
//
//************************************************************************************************

#ifndef _ccl_commanddispatch_h
#define _ccl_commanddispatch_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// CommandDelegate
/**
	\ingroup gui_command */
//************************************************************************************************

template <class T>
class CommandDelegate: public Unknown,
					   public ICommandHandler
{
public:
	typedef bool (T::*CommandDelegateMethod) (CmdArgs, VariantRef data);

	CommandDelegate (T* handler, CommandDelegateMethod method, VariantRef _data)
	: handler (handler),
	  method (method),
	  data (_data)
	{
		data.share ();
	}
	
	static AutoPtr<ICommandHandler> make (T* handler, CommandDelegateMethod method, VariantRef data)
	{
		return AutoPtr<ICommandHandler> (NEW CommandDelegate<T> (handler, method, data));
	}

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		return true;
	}
	
	tbool CCL_API interpretCommand (const CommandMsg& msg) override
	{
		return (handler->*method) (msg, data);
	}
	
	CLASS_INTERFACE (ICommandHandler, Unknown)

protected:
	T* handler;
	CommandDelegateMethod method;
	Variant data;
};

template <class T> AutoPtr<ICommandHandler> makeCommandDelegate (T* handler, typename CommandDelegate<T>::CommandDelegateMethod method, IUnknown* data);
template <class T> AutoPtr<ICommandHandler> makeCommandDelegate (T* handler, typename CommandDelegate<T>::CommandDelegateMethod method, VariantRef data);

//************************************************************************************************
// LambdaCommandDelegate
//***************************************************************************************+++++++++

template <class Lambda>
class LambdaCommandDelegate: public Unknown,
							 public ICommandHandler
{
public:
	LambdaCommandDelegate (const Lambda& interpret, VariantRef _data)
	: interpret (interpret),
	  data (_data)
	{
		data.share ();
	}

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		return true;
	}
	
	tbool CCL_API interpretCommand (const CommandMsg& msg) override
	{
		return interpret (msg, data);
	}
	
	CLASS_INTERFACE (ICommandHandler, Unknown)

protected:
	Lambda interpret;
	Variant data;
};

template <class Lambda> AutoPtr<ICommandHandler> makeCommandDelegate (const Lambda& interpret, IUnknown* data);
template <class Lambda> AutoPtr<ICommandHandler> makeCommandDelegate (const Lambda& interpret, VariantRef data);

//************************************************************************************************
// CommandAutomator
/**
	\ingroup gui_command */
//************************************************************************************************

class CommandAutomator
{
public:
	/** Get arguments associated with command message. */
	static IAttributeList* getArguments (CmdArgs args);

	class Arguments;
};

//************************************************************************************************
// class CommandAutomator::Arguments
/**
	\ingroup gui_command */
//************************************************************************************************

class CommandAutomator::Arguments
{
public:
	Arguments (CmdArgs args);
	Arguments (IAttributeList* arguments);

	bool hasArguments () const;

	bool getInt (StringID id, int& value) const;
	bool getBool (StringID id, bool& value) const;
	bool getFloat (StringID id, double& value) const;
	bool getString (StringID id, String& value) const;
	bool getCString (StringID id, MutableCString& value) const;

	int getInt (StringID id) const;
	bool getBool (StringID id) const;
	double getFloat (StringID id) const;
	String getString (StringID id) const;
	MutableCString getCString (StringID id) const;
	IUnknown* getObject (StringID id) const;

	bool getValue (StringID id, Variant& value) const;

private:
	IAttributeList* arguments;

	template<class T, T (Variant::*assign) () const> bool getValue (StringID id, T& value) const;
};

//************************************************************************************************
// Command Macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
	class MyClass:	public BaseClass,
					public CommandDispatcher<MyClass>
	{
	public:
		DECLARE_COMMANDS (MyClass)
		DECLARE_COMMAND_CATEGORY ("File", BaseClass)

		bool onFileNew (CmdArgs) { return false; }
	};

	BEGIN_COMMANDS (MyClass)
		DEFINE_COMMAND ("File", "New",  MyClass::onFileNew)	// one command specified by category+name
		DEFINE_COMMAND ("Edit", 0,		MyClass::onEditAny)	// any command of a certain category
		
		DEFINE_COMMAND_("File", "Quit", MyClass::onQuit, CommandFlags::kNoRepeat) // specify additional options
	END_COMMANDS (MyClass)

	IMPLEMENT_COMMANDS (MyClass, BaseClass)

	// register commands without dispatcher:
	REGISTER_COMMAND ("Edit", "Copy")
	REGISTER_COMMAND ("Edit", "Paste")
	REGISTER_COMMAND_("Edit", "Delete", CommandFlags::kGlobal) // specify additional options
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare command dispatcher for class. */
#define DECLARE_COMMANDS(Class) \
protected: \
friend class CCL::CommandDispatcher<Class>; \
friend class CCL::CommandDispatcherRegistrar<Class>; \
friend class CCL::CommandRegistry; \
static CCL::CommandDispatchEntry<Class> __dispatchTable[]; \
static CCL::CommandDispatcherRegistrar<Class> __registrar; \
public: \
CCL::tbool CCL_API interpretCommand (const CCL::CommandMsg& msg) override;

/** Declare check for command category (1 category). */
#define DECLARE_COMMAND_CATEGORY(_cat, Parent) \
CCL::tbool CCL_API checkCommandCategory (CCL::CStringRef category) const override \
{ if(category == _cat) return true; \
  return Parent::checkCommandCategory (category); }

/** Declare check for command category (2 categories). */
#define DECLARE_COMMAND_CATEGORY2(_cat1, _cat2, Parent) \
CCL::tbool CCL_API checkCommandCategory (CCL::CStringRef category) const override \
{ if(category == _cat1 || category == _cat2) return true; \
  return Parent::checkCommandCategory (category); }

/** Declare check for command category (3 categories). */
#define DECLARE_COMMAND_CATEGORY3(_cat1, _cat2, _cat3, Parent) \
CCL::tbool CCL_API checkCommandCategory (CCL::CStringRef category) const override \
{ if(category == _cat1 || category == _cat2 || category == _cat3) return true; \
  return Parent::checkCommandCategory (category); }

/** Begin command dispatch table definition for class. */
#define BEGIN_COMMANDS(Class) \
CommandDispatchEntry<Class> Class::__dispatchTable[] = {

/** Define command dispatch entry for class. */
#define DEFINE_COMMAND(category, name, Method) \
DEFINE_COMMAND_(category, name, Method, 0)

/** Define command dispatch entry for class (+ command options). */
#define DEFINE_COMMAND_(category, name, Method, flags) \
DEFINE_COMMAND_ARGS (category, name, Method, flags, nullptr)

/** Define command dispatch entry for class (+ command options and arguments). */
#define DEFINE_COMMAND_ARGS(category, name, Method, flags, arguments) \
{category, name, &Method, flags, arguments},

/** End command dispatch table definition for class (without registration). */
#define END_COMMANDS_UNREGISTERED \
{nullptr, nullptr, nullptr, 0, nullptr}};

/** End command dispatch table definition for class (+ registration). */
#define END_COMMANDS(Class) \
END_COMMANDS_UNREGISTERED \
CCL::CommandDispatcherRegistrar<Class> Class::__registrar;

/** Declare alias table for handling renamed commands. */
#define DECLARE_COMMAND_ALIASES(Class) \
private: \
static CCL::CommandAliasTable __commandAliasTable; \
friend class CCL::CommandAliasRegistrar<Class>; \
public:

/** Implement command alias table. */
#define DEFINE_COMMAND_ALIASES(Class) CCL::CommandAliasTable Class::__commandAliasTable;

/** Add alias definition for a renamed command. */
#define COMMAND_ALIAS(Class,category,name,aliasCategory,aliasName) static CCL::CommandAliasRegistrar<Class> UNIQUE_IDENT (CommandAlias) (category, name, aliasCategory, aliasName);

/** Add alias definition for a whole category. */
#define COMMAND_CATEGORY_ALIAS(Class,category,aliasCategory) static CCL::CommandAliasRegistrar<Class> UNIQUE_IDENT (CommandCategoryAlias) (category, aliasCategory);

/** Implement command dispatcher for class. */
#define IMPLEMENT_COMMANDS(Class, Parent) \
CCL::tbool CCL_API Class::interpretCommand (const CCL::CommandMsg& msg) \
{ if(CCL::CommandDispatcher<Class>::dispatchCommand (msg)) \
	return true; \
  return Parent::interpretCommand (msg); }

/** Implement command dispatcher with alias definitions for class. */
#define IMPLEMENT_COMMANDS_WITH_ALIASES(Class, Parent) \
CCL::tbool CCL_API Class::interpretCommand (const CCL::CommandMsg& msg) \
{ if(CCL::CommandDispatcher<Class>::dispatchCommandWithAliases (msg)) \
	return true; \
  return Parent::interpretCommand (msg); }

/** Register command. */
#define REGISTER_COMMAND(category, name) \
REGISTER_COMMAND_(category, name, 0)

/** Register command (+ command options). */
#define REGISTER_COMMAND_(category, name, flags) \
REGISTER_COMMAND_ARGS (category, name, flags, nullptr)

/** Register command (+ command options and arguments). */
#define REGISTER_COMMAND_ARGS(category, name, flags, arguments) \
static CCL::CommandRegistrar UNIQUE_IDENT (CommandRegistrar) (category, name, flags, arguments);

//************************************************************************************************
// Command Dispatcher
//************************************************************************************************


/** Command dispatch table entry. */
template <class T>
struct CommandDispatchEntry
{
	typedef bool (T::*CommandDispatchMethod) (CmdArgs);

	CStringPtr category;
	CStringPtr name;
	CommandDispatchMethod method;
	int flags;
	CStringPtr arguments;

	bool call (T* obj, CmdArgs args)
	{ 
		bool result = (obj->*method) (args);
		return result; 
	}
};

/** Template class for command dispatcher. */
template <class T>
class CommandDispatcher
{
public:
	bool dispatchCommand (const CommandMsg& msg);
	bool dispatchCommand (const CommandMsg& msg, CommandDispatchEntry<T> dispatchTable[]);
	bool dispatchCommandWithAliases (const CommandMsg& msg);
};

//************************************************************************************************
// Command Registration
//************************************************************************************************

/** Command registry. */
class CommandRegistry
{
public:
	struct Entry;
	static void registerCommand (CStringPtr category, CStringPtr name, int flags, CStringPtr arguments);
	static void registerWithCommandTable ();
	
	/** Helper to add single command to table with translation. */
	static void addToCommandTable (CStringPtr category, CStringPtr name, int flags = 0, CStringPtr arguments = nullptr);

	template <class T>
	static void addToCommandTable (CommandDispatchEntry<T> dispatchTable[])
	{
		for(CommandDispatchEntry<T>* t = dispatchTable; t->category; t++)
		{
			if(t->flags & CommandFlags::kHidden)
				continue;
			addToCommandTable (t->category, t->name, t->flags, t->arguments);
		}
	}

	template <class T>
	static void addToCommandTable ()
	{
		addToCommandTable (T::__dispatchTable);
	}

	/** Helper to find translated command title. */
	static CommandWithTitle find (CStringPtr category, CStringPtr name);
	static inline CommandWithTitle find (CmdArgs args) { return find (args.category, args.name); }

	/** Helper to register a command alias with the global command table. */
	static void registerAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName);
};

/** Command registrar. */
class CommandRegistrar
{
public:
	CommandRegistrar (CStringPtr category, CStringPtr name, int flags, CStringPtr arguments)
	{
		CommandRegistry::registerCommand (category, name, flags, arguments);
	}
};

/** Template class for command dispatcher registrar. */
template <class T>
class CommandDispatcherRegistrar
{
public:
	CommandDispatcherRegistrar ()
	{
		for(CommandDispatchEntry<T>* t = T::__dispatchTable; t->category; t++)
			CommandRegistry::registerCommand (t->category, t->name, t->flags, t->arguments);
	}
};

//************************************************************************************************
// CommandAliasTable
//************************************************************************************************

class CommandAliasTable
{
public:
	void addCategoryAlias (CStringPtr category, CStringPtr aliasCategory);
	void addCommandAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName);

	bool resolveCommandAlias (CommandMsg& resultMsg, const CommandMsg& aliasMsg) const;

private:
	struct AliasItem
	{
		CString category;
		CString name;
		CString aliasCategory;
		CString aliasName;
	};
	Vector<AliasItem> aliases;
};

//************************************************************************************************
// CommandAliasRegistrar
//************************************************************************************************

template <class T>
class CommandAliasRegistrar
{
public:
	CommandAliasRegistrar (CStringPtr category, CStringPtr aliasCategory)
	{
		T::__commandAliasTable.addCategoryAlias (category, aliasCategory);
		CommandRegistry::registerAlias (category, nullptr, aliasCategory, nullptr);
	}

	CommandAliasRegistrar (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName)
	{
		T::__commandAliasTable.addCommandAlias (category, name, aliasCategory, aliasName);
		CommandRegistry::registerAlias (category, name, aliasCategory, aliasName);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// CommandDispatcher inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool CommandDispatcher<T>::dispatchCommand (const CommandMsg& msg, CommandDispatchEntry<T> dispatchTable[])
{
	for(CommandDispatchEntry<T>* t = dispatchTable; t->category; t++)
		if(CString (t->category) == msg.category)
		{
			CString name (t->name);
			if(name.isEmpty () || name == msg.name)
				return t->call (static_cast<T*> (this), msg);
		}
	return false;
}

template <class T>
bool CommandDispatcher<T>::dispatchCommand (const CommandMsg& msg)
{
	return dispatchCommand (msg, T::__dispatchTable);
}

template <class T>
bool CommandDispatcher<T>::dispatchCommandWithAliases (const CommandMsg& msg)
{
	// first try exact match in dispatch table
	if(dispatchCommand (msg, T::__dispatchTable))
		return true;

	// try to resolve an alias
	CommandMsg actualCmd;
	if(T::__commandAliasTable.resolveCommandAlias (actualCmd, msg))
		if(dispatchCommand (actualCmd, T::__dispatchTable))
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
inline AutoPtr<ICommandHandler> makeCommandDelegate (T* handler, typename CommandDelegate<T>::CommandDelegateMethod method, IUnknown* data)
{ return CommandDelegate<T>::make (handler, method, data); }

template <class T>
inline AutoPtr<ICommandHandler> makeCommandDelegate (T* handler, typename CommandDelegate<T>::CommandDelegateMethod method, VariantRef data)
{ return CommandDelegate<T>::make (handler, method, data); }

template <class Lambda>
inline AutoPtr<ICommandHandler> makeCommandDelegate (const Lambda& interpret, IUnknown* data)
{ return makeCommandDelegate (interpret, Variant (data)); }

template <class Lambda>
inline AutoPtr<ICommandHandler> makeCommandDelegate (const Lambda& interpret, VariantRef data)
{ return AutoPtr<ICommandHandler> (NEW LambdaCommandDelegate<Lambda> (interpret, data)); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_commanddispatch_h
