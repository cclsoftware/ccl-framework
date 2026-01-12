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
// Filename    : ccl/gui/commands.cpp
// Description : Key Commands
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/commands.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/logfile.h"

#include "ccl/gui/keyevent.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/windowbase.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/menu.h"

#include "ccl/app/paramcontainer.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/ierrorhandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// KeyBinding
//************************************************************************************************

class KeyBinding: public Boxed::KeyEvent,
				  public IKeyBinding
{
public:
	DECLARE_CLASS (KeyBinding, Boxed::KeyEvent)

	KeyBinding (const CCL::KeyEvent& e = CCL::KeyEvent ());

	PROPERTY_MUTABLE_CSTRING (layout, Layout)
	PROPERTY_BOOL (notLayout, NotLayout)

	bool checkLayout (StringID layout) const;

	// IKeyBinding
	void CCL_API copyTo (CCL::KeyEvent& key) const override;

	// Boxed::KeyEvent
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IKeyBinding, Boxed::KeyEvent)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool tryWindowBase (WindowBase* windowBase, const CommandMsg& msg)
{
	UnknownPtr<ICommandHandler> handler (windowBase->getController ());
	if(handler && handler->interpretCommand (msg))
		return true;

	// hack: if no activatable child WindowBase, try controller of first view...
	if(windowBase->getFirstActivatableChild () == nullptr)
	{
		View* firstChild = windowBase->getChild (0);
		if(firstChild)
		{
			UnknownPtr<ICommandHandler> handler (firstChild->getController ());
			if(handler && handler->interpretCommand (msg))
				return true;
		}
	}

	IterForEach (windowBase->getChildWindows (), WindowBase, child)
		if(child->isActive () && tryWindowBase (child, msg))
			return true;
	EndFor

	if(!windowBase->isActive ())
	{
		// in case of inactive application (no active window), try "active" child of inactive window
		WindowBase* child = windowBase->getActiveChild ();
		if(child && tryWindowBase (child, msg))
			return true;
	}
	return false;
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (CommandFile, "Keyboard Mapping Scheme")
END_XSTRINGS

//************************************************************************************************
// KeyBinding
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (KeyBinding, Boxed::KeyEvent, "Key")
DEFINE_CLASS_NAMESPACE (KeyBinding, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyBinding::KeyBinding (const CCL::KeyEvent& e)
: Boxed::KeyEvent (e),
  notLayout (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyBinding::checkLayout (StringID _layout) const
{
	if(layout.isEmpty ())
		return true;

	if(notLayout)
		return layout != _layout;
	else
		return layout == _layout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API KeyBinding::copyTo (CCL::KeyEvent& key) const
{
	key = *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyBinding::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	MutableCString string = storage.getAttributes ().getCString ("layout");
	if(!string.isEmpty ())
	{
		if(string.startsWith ("!"))
		{
			layout = string.subString (1);
			notLayout = true;
		}
		else
			layout = string;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyBinding::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();

	if(!layout.isEmpty ())
	{
		MutableCString string;
		if(notLayout)
			string += "!";
		string += layout;
		a.set ("layout", layout);
	}
	return true;
}

//************************************************************************************************
// CommandMsgEx
//************************************************************************************************

CommandMsgEx::CommandMsgEx (StringRef _category, StringRef _name, IUnknown* invoker, int flags)
: CommandMsg (nullptr, nullptr, invoker, flags)
{
	this->category = categoryBuffer;
	this->name = nameBuffer;

	if(!_category.isEmpty ())
		setCategory (_category);
	if(!_name.isEmpty ())
		setName (_name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandMsgEx::setCategory (StringRef category)
{
	category.toASCII (categoryBuffer, sizeof(categoryBuffer));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandMsgEx::setName (StringRef name)
{
	name.toASCII (nameBuffer, sizeof(nameBuffer));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandMsgEx::setCategory (StringID category)
{
	::strncpy (this->categoryBuffer, category.str (), 256);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandMsgEx::setName (StringID name)
{
	::strncpy (this->nameBuffer, name, 256);
}

//************************************************************************************************
// Command
//************************************************************************************************

DEFINE_CLASS (Command, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Command::Command (StringID category, StringID name)
: category (category),
  name (name),
  rolloutVersion (0),
  flags (0)
{
	bindings.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Command::Command (const CommandMsg& msg)
: category (msg.category),
  name (msg.name),
  rolloutVersion (0),
  flags (0)
{
	bindings.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Command::getDescription (CommandDescription& description) const
{
	description.category = category;
	description.name = name;
	description.flags = flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API Command::newBindingIterator () const
{
	return getBindings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Command::getBindings () const
{
	return bindings.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::hasBindings () const
{
	return !bindings.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyEvent* Command::getDefaultKey (bool layoutSensitive) const 
{ 
	StringID activeLayout = CommandTable::instance ().getActiveLayout ();
	ListForEachObject (bindings, KeyBinding, k)
		if(layoutSensitive && !k->checkLayout (activeLayout))
			continue;
		return k;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::isKeyAssigned (const KeyEvent& key, bool layoutSensitive) const
{
	StringID activeLayout = CommandTable::instance ().getActiveLayout ();
	ListForEachObject (bindings, KeyBinding, k)
		if(layoutSensitive && !k->checkLayout (activeLayout))
			continue;
		if(k->isSimilar (key))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Command::assignKey (const KeyEvent& key, bool exclusive)
{
	ASSERT (key.isValid () == true)

	if(exclusive)
	{
		bindings.removeAll ();
		bindings.add (NEW KeyBinding (key));
	}
	else if(!isKeyAssigned (key, false))
		bindings.add (NEW KeyBinding (key));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Command::copyKeys (const Command& other)
{
	removeKeys ();
	mergeKeys (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Command::mergeKeys (const Command& other)
{
	IterForEach (other.getBindings (), KeyBinding, k)
		assignKey (*k);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::removeKey (const KeyEvent& key)
{
	bool removed = false;
	ListForEachObject (bindings, KeyBinding, k)
		if(k->isSimilar (key))
		{
			bindings.remove (k);
			k->release ();
			removed = true;
		}
	EndFor
	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::removeKeyAt (int index)
{
	int i = 0;
	ListForEachObject (bindings, KeyBinding, k)
		if(i == index)
		{
			bindings.remove (k);
			k->release ();
			return true;
		}
		i++;
	EndFor
	ASSERT (false)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Command::removeKeys ()
{
	bindings.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::interpretSafe (ICommandHandler* defaultHandler) const
{
	#if 1 // check first
	if(!canInterpret (defaultHandler))
		return false;
	#endif

	return interpret (defaultHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::canInterpret (ICommandHandler* defaultHandler) const
{
	CommandMsg msg (category, name, ccl_as_unknown (*this), CommandMsg::kCheckOnly);
	return CommandTable::instance ().interpretCommand (msg, defaultHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::interpret (ICommandHandler* defaultHandler) const
{
	CommandMsg msg (category, name, ccl_as_unknown (*this));
	return CommandTable::instance ().interpretCommand (msg, defaultHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	a.get (category, "category");
	a.get (name, "name");
	a.getInt (rolloutVersion, "rollout");
	
	/* flags are handled by known commands!
	if(a.getString ("global") == "true")
		isGlobal (true);
	if(a.getString ("repeat") == "false")
		noRepeat (true);
	*/

	// Key bindings
	KeyBinding* key;
	while((key = (KeyBinding*)a.unqueueObject (nullptr, ccl_typeid<KeyBinding> ())) != nullptr)
		bindings.add (key);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("category", category);
	a.set ("name", name);
	// "rollout" is not saved, can only be specified in a default scheme we provide
	
	/* flags are handled by known commands!
	if(isGlobal ())
		a.set ("global", "true");
	if(noRepeat ())
		a.set ("repeat", "false");
	*/
	
	// Key bindings
	ForEach (bindings, KeyBinding, key)
		a.queue (nullptr, key, Attributes::kShare);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Command::equals (const Object& obj) const
{
	const Command* c = ccl_cast<Command> (&obj);
	return c && c->category == category && c->name == name;
}

//************************************************************************************************
// KnownCommand
//************************************************************************************************

DEFINE_CLASS_HIDDEN (KnownCommand, Command)

//////////////////////////////////////////////////////////////////////////////////////////////////

KnownCommand::KnownCommand (const CommandDescription& description)
: Command (description.category, description.name),
  arguments (description.arguments),
  displayCategory (description.displayCategory),
  displayName (description.displayName),
  englishName (description.englishName),
  classID (description.classID)
{
	setFlags (description.flags);
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API KnownCommand::getDescription (CommandDescription& description) const
{
	SuperClass::getDescription (description);
	description.arguments = arguments;
	description.displayCategory = displayCategory;
	description.displayName = displayName;
	description.englishName = englishName;
	description.classID = classID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String KnownCommand::getTitle () const
{
	return String () << getDisplayCategory () << CCLSTR (" - ") << getDisplayName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int KnownCommand::compare (const Object& obj) const
{
	const KnownCommand* other = ccl_cast<KnownCommand> (&obj);
	return other ? displayName.compareWithOptions (other->displayName, Text::kCompareNumerically) : SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API KnownCommand::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "displayName")
	{
		setDisplayName (var.asString ());
		return true;
	}
	else if(propertyId == "displayCategory")
	{
		setDisplayCategory (var.asString ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API KnownCommand::getProperty (Variant& var, MemberID propertyId) const
{
#define RETURN_PROPERTY(Name,member) \
	if(propertyId == Name) { String s (member); var = s; var.share (); return true; }

	RETURN_PROPERTY ("name", name)
	RETURN_PROPERTY ("category", category)
	RETURN_PROPERTY ("arguments", arguments)
	RETURN_PROPERTY ("displayCategory", displayCategory)
	RETURN_PROPERTY ("displayName", displayName)

#undef RETURN_PROPERTY

	if(propertyId == "classID")
	{
		String s;
		if(classID.isValid ()) // keep string empty otherwise
			classID.toString (s);
		var = s;
		var.share ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// CommandCategory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CommandCategory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandCategory::CommandCategory (StringRef title)
: title (title)
{
	commands.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandCategory::add (Command* command, bool share)
{
	if(share)
		command->retain ();
	commands.addSorted (command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* CommandCategory::newIterator () const
{
	return commands.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CommandCategory::getDisplayCategory () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandCategory::newCommandIterator () const
{
	return commands.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandCategory::equals (const Object& obj) const
{
	const CommandCategory* other = ccl_cast<CommandCategory> (&obj);
	return other ? other->title == title : SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandCategory::compare (const Object& obj) const
{
	const CommandCategory* other = ccl_cast<CommandCategory> (&obj);
	return other ? title.compareWithOptions (other->title, Text::kCompareNumerically) : SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandCategory::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "title" || propertyId == "displayCategory")
	{
		var = title;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (CommandCategory)
	DEFINE_METHOD_NAME ("newCommandIterator")
END_METHOD_NAMES (CommandCategory)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandCategory::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "newCommandIterator")
	{
		returnValue.takeShared (AutoPtr<IUnknownIterator> (newCommandIterator ()));
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// CommandTable
//************************************************************************************************

CCL_KERNEL_INIT_LEVEL (CommandTable, kFrameworkLevelSecond+1)
{
	// load built-in commands
	ResourceUrl url (CCLSTR ("commands.xml"));
	bool loaded = CommandTable::instance ().loadCommands (url, CommandTable::kReplaceAll) != 0;
	SOFT_ASSERT (loaded == true, "Built-in commands not loaded")

	// register key scheme file type
	System::GetFileTypeRegistry ().registerFileType (CommandTable::instance ().getCommandFileType ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (CommandTable, Object)
DEFINE_CLASS_NAMESPACE (CommandTable, NAMESPACE_CCL)
DEFINE_SINGLETON (CommandTable)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandTable::CommandTable ()
: localeSink (*NEW SignalSink (Signals::kLocales)),
  errorSink (*NEW SignalSink (Signals::kErrorHandler)),
  logBuffer (*NEW LogBuffer),
  commandParams (nullptr),
  version (0),
  activeSchemeModified (false),
  commandRegistrationChanged (false),
  transactionActiveCounter (0),
  blocklist (nullptr)
{
	commands.objectCleanup ();
	knownCommands.objectCleanup ();
	logBuffer.setTitle ("Commands:");

	localeSink.setObserver (this);
	localeSink.enable (true);

	errorSink.setObserver (this);
	errorSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandTable::~CommandTable ()
{
	if(commandParams)
		commandParams->release ();

	ASSERT (globalHandlers.isEmpty () == true)

	cancelSignals ();

	safe_release (blocklist);

	localeSink.enable (false);
	delete &localeSink;

	errorSink.enable (false);
	delete &errorSink;

	delete &logBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandTable::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IController> ())
	{
		if(!commandParams)
			commandParams = NEW ParamContainer;
		return commandParams->queryInterface (iid, ptr);
	}

	QUERY_INTERFACE (ICommandTable)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::updateMenuKeys ()
{
	for(int i = 0, count = Desktop.countWindows (); i < count; i++)
		if(Window* window = unknown_cast<Window> (Desktop.getWindow (i)))
			if(MenuBar* menuBar = window->getMenuBar ())
				menuBar->updateKeys ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::loadCommands (UrlRef url, int loadHint)
{
	CommandFile file;
	if(!file.loadFromFile (url))
		return false;

	load (file, loadHint);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::loadBlocklist (UrlRef path)
{
	ASSERT (blocklist == nullptr) // should be called only once!

	AutoPtr<CommandFile> file = NEW CommandFile;
	if(!file->loadFromFile (path))
		return false;
	take_shared<CommandFile> (blocklist, file);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::saveCommands (UrlRef path) const
{
	CommandFile file;
	save (file);
	return file.saveToFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CommandTable::getActiveSchemeName (tbool* modified) const
{
	if(modified)
		*modified = activeSchemeModified;
	return activeSchemeName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API CommandTable::getCommandFileType () const
{
	static FileType fileType (nullptr, "keyscheme", CCL_MIME_TYPE "-keyscheme+xml");
	return FileTypes::init (fileType, XSTR (CommandFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::setCommand (const Command& command)
{
	// check for existing entry
	Command* c = (Command*)commands.findEqual (command);

	// check if bindings have been removed
	if(!command.hasBindings ()) 
	{
		if(c) // existing entry ain't needed anymore
		{
			commands.remove (c);
			c->release ();
		}
		return;
	}

	if(c == nullptr)
	{
		// create new entry
		c = NEW Command (command.getCategory (), command.getName ());
		c->setRolloutVersion (command.getRolloutVersion ());
		commands.add (c);

		// apply flags
		KnownCommand* knownCommand = (KnownCommand*)knownCommands.findEqual (*c);
		if(knownCommand)
			c->setFlags (knownCommand->getFlags ());
	}

	// update bindings
	c->copyKeys (command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandTable::matchesBlocklist (const Command& command) const
{
	if(blocklist)
		IterForEach ((*blocklist)->newIterator (), Command, c)
			if(command.getCategory () == c->getCategory ())
			{
				if(c->getName ().isEmpty ()) // whole category
					return true;
				if(command.getName () == c->getName ())
					return true;
			}
		EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::load (CommandFile& file, int loadHint)
{
	if(loadHint == kReplaceAll)
	{
		CCL_PRINTLN ("[Commands] Load with kReplaceAll")
		commands.removeAll ();
		IterForEach (file->newIterator (), Command, c)
			if(matchesBlocklist (*c))
			{
				CCL_PRINTF ("[Commands] Command [%s|%s] filtered via blocklist!\n", c->getCategory ().str (), c->getName ().str ())
				continue;
			}
			setCommand (*c);
		EndFor
	}
	else if(loadHint == kOverwriteExisting)
	{
		CCL_PRINTLN ("[Commands] Load with kOverwriteExisting")
		IterForEach (file->newIterator (), Command, c)
			if(matchesBlocklist (*c))
			{
				CCL_PRINTF ("[Commands] Command [%s|%s] filtered via blocklist!\n", c->getCategory ().str (), c->getName ().str ())
				continue;
			}

			if(c->hasBindings ())
				IterForEach (c->getBindings (), KeyBinding, key)
					Command* conflict = lookupCommand (*key, false);
					if(conflict && !conflict->equals (*c)) // other command using the same key
					{
						bool keepExisting = conflict->getRolloutVersion () > file.getVersion ();
						if(keepExisting)
						{
							#if DEBUG_LOG
							String keyString;
							key->toString (keyString);
							CCL_PRINTF ("[Commands] Key \"%s\" of [%s|%s] skipped because of usage by [%s|%s] (rollout version %d)\n", MutableCString (keyString).str (), 
										c->getCategory ().str (), c->getName ().str (),
										conflict->getCategory ().str (), conflict->getName ().str (), conflict->getRolloutVersion ())
							#endif

							c->removeKey (*key);
						}
						else
						{
							#if DEBUG_LOG
							String keyString;
							key->toString (keyString);
							CCL_PRINTF ("[Commands] Key \"%s\" of [%s|%s] overwrites previous usage by [%s|%s]\n", MutableCString (keyString).str (), 
										c->getCategory ().str (), c->getName ().str (),
										conflict->getCategory ().str (), conflict->getName ().str ())
							#endif
							conflict->removeKey (*key);
							if(!conflict->hasBindings ())
							{
								commands.remove (conflict);
								conflict->release ();
							}
						}
					}
				EndFor

			if(!c->hasBindings ()) // command in file has no bindings, or they were just removed by the code above
			{
				Command* existing = (Command*)commands.findEqual (*c);
				if(existing && existing->getRolloutVersion () > file.getVersion ())
					continue;
			}

			setCommand (*c);
		EndFor
	}
	else if(loadHint == kKeepExisting)
	{
		CCL_PRINTLN ("[Commands] Load with kKeepExisting")
		IterForEach (file->newIterator (), Command, _c)
			if(matchesBlocklist (*_c))
			{
				CCL_PRINTF ("[Commands] Command [%s|%s] filtered via blocklist!\n", _c->getCategory ().str (), _c->getName ().str ())
				continue;
			}

			if(!_c->hasBindings ()) // ignore empty entries
				continue;

			// create a mutable copy
			AutoPtr<Command> c = NEW Command (_c->getCategory (), _c->getName ());
			c->setRolloutVersion (_c->getRolloutVersion ());
			c->copyKeys (*_c);

			// merge with existing bindings for this command
			Command* existing = (Command*)commands.findEqual (*c);
			if(existing)
				c->mergeKeys (*existing);

			IterForEach (c->getBindings (), KeyBinding, key)
				Command* conflict = lookupCommand (*key, false);
				if(conflict && !conflict->equals (*c)) // other command using the same key
				{
					#if DEBUG_LOG
					String keyString;
					key->toString (keyString);
					CCL_PRINTF ("[Commands] Key \"%s\" of [%s|%s] skipped because of usage by [%s|%s]\n", MutableCString (keyString).str (), 
								c->getCategory ().str (), c->getName ().str (),
								conflict->getCategory ().str (), conflict->getName ().str ())
					#endif

					c->removeKey (*key); // other has priority, remove from this command
				}
			EndFor

			setCommand (*c);
		EndFor
	}
	else if(loadHint == kDefineKnown)
	{
		CCL_PRINTLN ("[Commands] Load with kDefineKnown")
		commands.removeAll ();
		IterForEach (file->newIterator (), Command, c)
			KnownCommand* knownCommand = (KnownCommand*)knownCommands.findEqual (*c);
			ASSERT (knownCommand)
			if(knownCommand)
			{
				c->retain ();
				commands.add (c);
				c->setFlags (knownCommand->getFlags ());
			}
		EndFor	

		ObjectListIterator iter (knownCommands);
		while(iter.done () == false)
		{
			KnownCommand* c = (KnownCommand*)iter.next ();
			if(commands.findEqual (*c) == nullptr)
			{
				knownCommands.remove (iter);
				c->release ();
			}
		}	
	}
	else
	{
		CCL_DEBUGGER ("Unknown command load hint!")
	}

	// update scheme name
	if(loadHint != kKeepExisting)
	{
		SOFT_ASSERT (!file.getName ().isEmpty (), "Command scheme name not set!")
		activeSchemeName = file.getName ();
		activeSchemeModified = file.isModified ();
	}

	if(version == 0 && file.getVersion () != 0 && loadHint <= kOverwriteExisting)
	{
		setVersion (file.getVersion ());
		CCL_PRINTF ("[Commands] init version: %d\n", version);
	}

	// update menus
	updateMenuKeys ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::save (CommandFile& file) const
{
	file.setName (getActiveSchemeName ());
	file.setModified (activeSchemeModified);
	file.setVersion (version);

#if 1 // save all known commands to allow removing key bindings
	ObjectArray unassigned;
	unassigned.objectCleanup (true);
	
	ForEach (knownCommands, Command, k)
		if(Command* c = (Command*)commands.findEqual (*k))
			file->add (return_shared (c));
		else
			unassigned.add (NEW Command (k->getCategory (), k->getName ()));
	EndFor

	ForEach (unassigned, Command, c)
		file->add (return_shared (c));
	EndFor
#else
	ForEach (commands, Command, c)
		file->add (c);
		c->retain ();
	EndFor
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::addHandler (ICommandHandler* handler)
{
	ASSERT (handler != nullptr)
	if(!handler)
		return;

	globalHandlers.append (handler);
	handler->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::removeHandler (ICommandHandler* handler)
{
	ASSERT (handler != nullptr)
	if(!handler)
		return;

	bool removed = globalHandlers.remove (handler);
	ASSERT (removed == true)
	if(removed)
		handler->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::addFilter (ICommandFilter* filter)
{
	ASSERT (filter != nullptr)
	if(!filter)
		return;

	filters.append (filter);
	filter->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::removeFilter (ICommandFilter* filter)
{
	ASSERT (filter != nullptr)
	if(!filter)
		return;

	bool removed = filters.remove (filter);
	ASSERT (removed == true)
	if(removed)
		filter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::addCommandAlias (CStringPtr category, CStringPtr name, CStringPtr aliasCategory, CStringPtr aliasName)
{
	return aliasTable.addCommandAlias (category, name, aliasCategory, aliasName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandTable::resolveCommandAlias (CommandMsg& resultMsg, const CommandMsg& aliasMsg) const
{
	return aliasTable.resolveCommandAlias (resultMsg, aliasMsg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::assignKey (ICommand& _command, const KeyEvent& key)
{
	if(Command* command = unknown_cast<Command> (&_command))
	{
		Command c (command->getCategory (), command->getName ());
		c.assignKey (key);
		setCommand (c);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandTable::lookupKeyBindings (const ICommand& _command) const
{
	Command* command = unknown_cast<Command> (&_command);
	return command ? lookupBindings (*command) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::isTransactionActive () const
{
	return transactionActiveCounter > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::beginTransaction (StringRef title)
{
	transactionActiveCounter++;		
	if(transactionActiveCounter == 1)
		signal (Message (kBeginTransaction, title));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::endTransaction ()
{
	transactionActiveCounter--;
	if(transactionActiveCounter == 0)
		signal (Message (kEndTransaction));

	ASSERT (transactionActiveCounter >= 0)
	if(transactionActiveCounter < 0)
		transactionActiveCounter = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandTable::isCommandAllowed (const CommandMsg& msg) const
{
	for(ICommandFilter* filter : filters)
		if(!filter->isCommandAllowed (msg))
			return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::performCommand (const CommandMsg& msg, tbool deferred)
{
	if(deferred)
	{
		ASSERT (msg.checkOnly () == false)
		if(msg.checkOnly ())
			return false;

		// check for command arguments (see also: CommandAutomator::getArguments())
		AutoPtr<Attributes> argsCopy;
		if(UnknownPtr<IAttributeList> args = msg.invoker)
		{
			argsCopy = NEW Attributes;
			argsCopy->copyFrom (*args);
		}

		deferCommand (msg.category, msg.name, argsCopy);
		return true;
	}
	else
		return interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CommandTable::getActiveLayout () const
{
	if(activeLayout.isEmpty ())
		activeLayout = System::GetLocaleManager ().getInputLanguage ();
	return activeLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyEvent* CommandTable::lookupKeyEvent (const Command& command, bool layoutSensitive) const
{
	Command* c = (Command*)commands.findEqual (command);
	return c ? c->getDefaultKey (layoutSensitive) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* CommandTable::lookupBindings (const Command& command) const
{
	Command* c = (Command*)commands.findEqual (command);
	if(c)
		return c->getBindings ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Command* CommandTable::lookupCommand (const KeyEvent& key, bool layoutSensitive) const
{
	ListForEachObject (commands, Command, c)
		if(c->isKeyAssigned (key, layoutSensitive))
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandTable::translateKey (const KeyEvent& _key, ICommandHandler* defaultHandler) const
{
	KeyEvent key (_key);

	// If the keyevent is a virtual key, we ignore
	// the character. Otherwise numeric numpad keys would trigger
	// command with numeric key assignments
	if(key.isVKeyValid ())
		key.character = 0;
	else
	// Characters are encoded in uppercase
		key.character = Unicode::toUppercase (key.character);

	#if 0//DEBUG_LOG
	String string;
	key.toString (string);
	static int keyCounter = 0;
	CCL_PRINTF ("%d) Translate Key ", ++keyCounter)
	CCL_PRINTLN (string)
	#endif

	Command* c = lookupCommand (key);
	if(c)
	{
		// filter key repeats
		if(key.isRepeat () && c->noRepeat ())
			return false;
		
		return c->interpretSafe (defaultHandler);
	}

	//if(key.state.getModifiers () != 0)
	//{
	//	key.state.keys = 0;
	//	return translateKey (key, defaultHandler);
	//}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const CommandMsgEx& CommandTable::getLastCommand () const
{
	return lastCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::setLastCommand (const CommandMsg& msg)
{
	lastCommand.setCategory (msg.category);
	lastCommand.setName (msg.name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandTable::interpretCommand (const CommandMsg& msg, ICommandHandler* defaultHandler) const
{
	if(!isCommandAllowed (msg))
		return false;

	if(msg.checkOnly () == false)
	{
		GUI.updateUserActivity ();

		if(msg.name != "Keyboard Shortcuts" && !(msg.category == "Application" && msg.name == "Options"))
			ccl_const_cast (this)->setLastCommand (msg);

		MutableCString str (msg.category);
		str += "|";
		str += msg.name;
		const_cast<CommandTable*> (this)->logBuffer.print (str);
	}

	// 1) try default handler...
	if(defaultHandler && defaultHandler->interpretCommand (msg))
		return true;

	// 2) check if we are in modal state...
	Window* modalWindow = Desktop.getTopWindow (kDialogLayer);
	if(modalWindow && (modalWindow->isInCloseEvent () || modalWindow->isInDestroyEvent ()))
		modalWindow = nullptr;
	Window* targetWindow = modalWindow;

	// 3) try focus view and active window...
	if(!targetWindow)
	{
		targetWindow = Desktop.getActiveWindow ();

		// 3.a) try window of inactive application (no window is active in this case)
		if(!targetWindow && !GUI.isApplicationActive ())
		{
			// note: the modal case is not affected, as the check under 2) does not require a modal window to be active
			// so it's quite safe to take the application window as the hottest candidate
			targetWindow = unknown_cast<Window> (Desktop.getApplicationWindow ());
			if(!targetWindow)
				targetWindow = Desktop.getLastWindow ();
		}
	}
	
	if(targetWindow)
	{
		if(IView* focusView = targetWindow->getFocusIView ())
		{
			UnknownPtr<ICommandHandler> focusHandler (focusView->getController ());
			if(focusHandler && focusHandler->interpretCommand (msg))
				return true;
		}

		// try window base and it's childs recursively
		if(tryWindowBase (targetWindow, msg))
			return true;

		UnknownPtr<ICommandHandler> windowHandler (targetWindow->getController ());
		if(windowHandler && windowHandler->interpretCommand (msg))
			return true;
	}

	// in modal state only allow global commands
	if(modalWindow)
	{
		// only restrict if there is a "real" modal dialog, ignoring counting popup selectors
		// skip all PopupSelectorWindows to find the "real" target window
		ASSERT (targetWindow == modalWindow)
		while(PopupSelectorWindow* popup = ccl_cast<PopupSelectorWindow> (targetWindow))
			targetWindow = unknown_cast<Window> (popup->getParentWindow ());

		if(targetWindow && targetWindow->getLayer () == kDialogLayer)
		{
			Command* cmd = unknown_cast<Command> (msg.invoker);
			if(cmd == nullptr) // we really need to know if it's a global command
				cmd = (KnownCommand*)knownCommands.findEqual (Command (msg.category, msg.name));
			if(!cmd || !cmd->isGlobal ())
				return false;
		}
	}

	// 4) try global handlers...
	ListForEach (globalHandlers, ICommandHandler*, globalHandler)
		if(globalHandler->interpretCommand (msg))
			return true;
	EndFor

	// 5) Internal commands
	#if DEBUG
	if(msg.category == "CommandTable")
	{
		if(msg.checkOnly ())
			return true;

		if(msg.name == "Dump")
			dump ();
		else if(msg.name == "Dump Available Keys")
			dumpAvailableKeys ();
		return true;
	}
	#endif
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::deferCommand (StringID category, StringID name, Attributes* args)
{
	AutoPtr<Command> command = NEW Command (category, name);

	// apply flags, required for handling in modal dialogs
	KnownCommand* knownCommand = (KnownCommand*)knownCommands.findEqual (*command);
	if(knownCommand)
		command->setFlags (knownCommand->getFlags ());

	Message* msg = NEW Message ("interpretCommand", command->asUnknown (), args ? args->asUnknown () : nullptr);
	msg->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "interpretCommand") // message from deferCommand()
	{
		Command* c = unknown_cast<Command> (msg.getArg (0));
		if(c)
		{
			// check for command arguments
			UnknownPtr<IAttributeList> args;
			if(msg.getArgCount () > 1)
				args = msg[1].asUnknown ();

			if(args.isValid ())
				performCommand (CommandMsg (c->getCategory (), c->getName (), args));
			else
				c->interpretSafe ();
		}
	}
	else if(msg == "registrationChanged")
	{
		commandRegistrationChanged = false;
		signal (Message (kChanged));
	}
	else if(msg == Signals::kInputLanguageChanged)
	{
		activeLayout = System::GetLocaleManager ().getInputLanguage ();
		updateMenuKeys ();
	}
	else if(msg == Signals::kCrashReported)
	{
		UnknownPtr<IStream> dumpStream (msg[0]);
		if(dumpStream)
			logBuffer.dump (*dumpStream);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API CommandTable::getCommandParam (StringID category, StringID name, tbool create)
{
	IParameter* p = commandParams ? commandParams->byCommand (category, name) : nullptr;
	if(!p && create)
	{
		if(!commandParams)
			commandParams = NEW ParamContainer;

		int tag = commandParams->count () + 100;
		MutableCString paramName;
		paramName.appendFormat ("%s.%s", category.str (), name.str ());
		p = commandParams->addCommand (category, name, paramName, tag);
	}
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::registerCommand (const CommandDescription& description)
{
	// Note: Registered commands do not affect the list of commands used for key translation
	KnownCommand* knownCommand = NEW KnownCommand (description);
	if(!knownCommands.contains (*knownCommand)) // only add new ones
	{
		knownCommands.add (knownCommand);

		// apply flags
		if(knownCommand->getFlags () != 0)
			if(Command* c = (Command*)commands.findEqual (*knownCommand))
				c->setFlags (knownCommand->getFlags ());		

		commandsChanged ();
	}
	else
		knownCommand->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTable::unregisterCommand (StringID category, StringID name)
{
	KnownCommand* knownCommand = (KnownCommand*)knownCommands.findEqual (Command (category, name));
	if(knownCommand)
	{
		knownCommands.remove (knownCommand);
		knownCommand->release ();

		commandsChanged ();
	}

	Command* command = (Command*)commands.findEqual (Command (category, name));
	if(command)
	{
		commands.remove (command);
		command->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::commandsChanged ()
{
	if(!GUI.isStarted ()) // suppress during initialization
		return;

	if(!commandRegistrationChanged)
	{
		commandRegistrationChanged = true;
		(NEW Message ("registrationChanged"))->post (this, 1000); // defer changed signal, collect multiple messages during the period
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommand* CCL_API CommandTable::findCommand (StringID category, StringID name) const
{
	ListForEachObject (knownCommands, KnownCommand, c)
		if(c->getCategory () == category && c->getName () == name)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandTable::newCommandIterator () const
{
	return knownCommands.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandTable::newCategoryIterator () const
{
	AutoPtr<CommandContainer> categories = createCategories ();
	return NEW HoldingIterator (categories, categories->newIterator ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandContainer* CommandTable::createCategories () const
{
	CommandContainer* container = NEW CommandContainer;
	getCategories (*container);
	return container;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::getCategories (CommandContainer& container) const
{
	ForEach (knownCommands, KnownCommand, c)
		KnownCommand* commandCopy = container.addCommand (*c);

		// transfer key bindings from current mapping
		if(Command* existing = (Command*)commands.findEqual (*c))
			commandCopy->copyKeys (*existing);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (CommandTable)
	DEFINE_METHOD_ARGR ("interpretCommand", "category: string, name: string, checkOnly: bool = false, invoker: Object = null", "bool")
	DEFINE_METHOD_ARGR ("deferCommand", "category: string, name: string, checkOnly: bool = false, invoker: Object = null", "bool")
	DEFINE_METHOD_ARGS ("addHandler", "ICommandHandler")
	DEFINE_METHOD_ARGS ("removeHandler", "ICommandHandler")
	DEFINE_METHOD_ARGR ("findCommand", "category: string, name: string", "Object") // TODO: replace Object return type
	DEFINE_METHOD_ARGS ("registerCommand", "category: string, name: string, displayCategory: string, displayName: string, englishName: string, arguments: string = ''")
	DEFINE_METHOD_ARGS ("unregisterCommand", "category: string, name: string")	
	DEFINE_METHOD_NAME ("newCommandIterator")
	DEFINE_METHOD_NAME ("newCategoryIterator")
	DEFINE_METHOD_ARGR ("lookupBindings", "command", "Object") // TODO replace Object return type
	DEFINE_METHOD_ARGR ("lookupKeyEvent", "command: Command, layoutSensitive: bool = true", "Key")
	DEFINE_METHOD_ARGS ("assignKey", "command, key")
	DEFINE_METHOD_NAME ("beginTransaction")
	DEFINE_METHOD_NAME ("endTransaction")
END_METHOD_NAMES (CommandTable)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTable::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "interpretCommand" || msg == "deferCommand")
	{
		bool deferred = msg == "deferCommand";
		MutableCString category (msg[0].asString ());
		MutableCString name (msg[1].asString ());
		bool checkOnly = msg.getArgCount () > 2 ? msg[2].asBool () : false;
		IUnknown* invoker = msg.getArgCount () > 3 ? msg[3].asUnknown () : nullptr;
		
		returnValue = performCommand (CommandMsg (category, name, invoker, checkOnly ? CommandMsg::kCheckOnly : 0), deferred);
		return true;
	}
	else if(msg == "addHandler")
	{
		UnknownPtr<ICommandHandler> handler (msg[0]);
		returnValue = handler.isValid ();
		if(handler)
			addHandler (handler);
		return true;
	}
	else if(msg == "removeHandler")
	{
		UnknownPtr<ICommandHandler> handler (msg[0]);
		returnValue = handler.isValid ();
		if(handler)
			removeHandler (handler);
		return true;
	}
	else if(msg == "findCommand")
	{
		returnValue.takeShared (findCommand (MutableCString (msg[0].asString ()), MutableCString (msg[1].asString ())));
		return true;
	}
	else if(msg == "registerCommand")
	{
		CommandDescription desc;
		desc.category = msg[0].asString ();
		desc.name = msg[1].asString ();
		desc.displayCategory = msg[2].asString ();
		desc.displayName = msg[3].asString ();
		desc.englishName = msg[4].asString ();

		if(msg.getArgCount () > 5)
			desc.arguments = msg[5].asString ();
		registerCommand (desc);

		return true;
	}
	else if(msg == "unregisterCommand")
	{
		MutableCString category (msg[0].asString ());
		MutableCString name (msg[1].asString ());
		unregisterCommand (category, name);
		return true;
	}
	else if(msg == "newCommandIterator")
	{
		returnValue.takeShared (AutoPtr<IUnknownIterator> (newCommandIterator ()));
		return true;
	}
	else if(msg == "newCategoryIterator")
	{
		returnValue.takeShared (AutoPtr<IUnknownIterator> (newCategoryIterator ()));
		return true;
	}
	else if(msg == "lookupBindings")
	{
		Command* command = unknown_cast<Command> (msg[0]);
		if(command)
			returnValue.takeShared (ccl_as_unknown (AutoPtr<Iterator> (lookupBindings (*command))));
		return true;
	}
	else if(msg == "lookupKeyEvent")
	{
		Command* command = unknown_cast<Command> (msg[0]);
		if(command)
			if(KeyEvent* key = lookupKeyEvent (*command))
				returnValue.takeShared (ccl_as_unknown (AutoPtr<Boxed::KeyEvent> (NEW Boxed::KeyEvent (*key))));
		return true;
	}
	else if(msg == "assignKey")
	{
		Command* command = unknown_cast<Command> (msg[0]);
		Boxed::KeyEvent* key = unknown_cast<Boxed::KeyEvent> (msg[1]);
		if(command && key)
			assignKey (*command, *key);
		return true;
	}
	else if(msg == "beginTransaction")
	{
		beginTransaction (msg[0].asString ());
		return true;
	}
	else if(msg == "endTransaction")
	{
		endTransaction ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::dump () const
{
	ListForEachObject (knownCommands, KnownCommand, c)
		String key;
		Command* used = (Command*)commands.findEqual (*c);
		if(used)
		{
			#if 1 // all bindings
			IterForEach (used->getBindings (), KeyBinding, k)
				String str;
				static_cast<KeyEvent*> (k)->toString (str);
				if(!key.isEmpty ())
					key << ";";
				key << "[" << str << "]";
			EndFor
			#else
			if(used->getDefaultKey ())
				used->getDefaultKey ()->toString (key);
			#endif
		}
		CCL::Debugger::printf ("%s;%s;%s\n", c->getCategory ().str (), c->getName ().str (), MutableCString (key).str ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandTable::dumpAvailableKeys () const
{
	struct DumpKey
	{
		enum { kFieldLength = 20 };

		DumpKey (KeyEvent& key, const CommandTable* table)
		{
			int modifiers[] = { 0, KeyState::kShift, KeyState::kCommand, KeyState::kOption,
				KeyState::kShift|KeyState::kCommand, KeyState::kShift|KeyState::kOption, KeyState::kCommand|KeyState::kOption };

			for(int m = 0; m < ARRAY_COUNT (modifiers); m++)
			{
				key.state = KeyState (modifiers[m]);

				// check if key is used with these modifiers
				String keyString;
				if(!table->lookupCommand (key))
					key.toString (keyString);

				int len = keyString.length ();
				if(len > kFieldLength)
					keyString.truncate (kFieldLength);
				else
					keyString.append (" ", kFieldLength - len);

				CCL::Debugger::printf ("%s", MutableCString (keyString).str ());
			}
			CCL::Debugger::println ("");
		}
	};

	// letters
	uchar c;
	for(c = 'A'; c <= 'Z'; c++)
	{
		KeyEvent keyEvent (KeyEvent::kKeyDown, VKey::kUnknown, c);
		DumpKey (keyEvent, this);
	}
	// digits
	for(c = '0'; c <= '9'; c++)
	{
		KeyEvent keyEvent (KeyEvent::kKeyDown, VKey::kUnknown, c);
		DumpKey (keyEvent, this);
	}
	
	// special characters
	const uchar specialChars[] = { '[', ']', '}', '+', '-', ',' }; // more?
	for(int i = 0; i < ARRAY_COUNT (specialChars); i++)
	{
		KeyEvent keyEvent (KeyEvent::kKeyDown, VKey::kUnknown, specialChars[i]);
		DumpKey (keyEvent, this);
	}
	
	// virtual keys
	for(int vKey = 0; vKey < VKey::kNumVirtualKeys; vKey++)
	{
		KeyEvent keyEvent (KeyEvent::kKeyDown, vKey);
		DumpKey (keyEvent, this);
	}
}

//************************************************************************************************
// CommandFile
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CommandFile, StorableObject, "Commands")

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandFile::CommandFile ()
: modified (false),
  version (0)
{
	commands.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandFile::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	name = a.getString ("name");
	modified = a.getBool ("modified");
	version = a.getInt ("version");
	a.unqueue (commands, nullptr, ccl_typeid<Command> ());

	for(auto* command : iterate_as<Command> (commands))
	{
		CommandMsg msg (command->getCategory (), command->getName ());
		CommandMsg actualCmd;
		if(CommandTable::instance ().resolveCommandAlias (actualCmd, msg))
		{
			// alias resolved successfuly - replace if the alias command from the file doesn't exist as official command
			if(!CommandTable::instance ().findCommand (command->getCategory (), command->getName ()))
			{
				CCL_PRINTF ("CommandFile::load (%s): resolved alias [%s|%s] \t-> [%s|%s]\n", MutableCString (name).str (), msg.category.str (), msg.name.str (), actualCmd.category.str (), actualCmd.name.str ())
				command->setCategory (actualCmd.category);
				command->setName (actualCmd.name);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandFile::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!name.isEmpty ())
		a.set ("name", name);
	if(modified)
		a.set ("modified", modified);
	if(version > 0)
		a.set ("version", version);
	a.queue (nullptr, commands, Attributes::kShare);
	return true;
}

//************************************************************************************************
// CommandContainer
//************************************************************************************************

DEFINE_CLASS (CommandContainer, Object)
DEFINE_CLASS_UID (CommandContainer, 0xD0271918, 0xF7B0, 0x4CB4, 0x9A, 0xAB, 0x96, 0x93, 0xDE, 0x78, 0x13, 0x9A)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandContainer::CommandContainer ()
{
	categories.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* CommandContainer::newIterator () const
{
	return categories.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandContainer::newCategoryIterator () const
{
	return categories.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommand* CCL_API CommandContainer::findCommand (StringID categoryName, StringID commandName) const
{
	ForEach (categories, CommandCategory, category)
		ForEach (*category, Command, command)
			if(command->getCategory () == categoryName && command->getName () == commandName)
				return command;
		EndFor
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandCategory* CommandContainer::addCategory (StringRef title)
{
	CommandCategory* category = (CommandCategory*)categories.findEqual (CommandCategory (title));
	if(category == nullptr)
		categories.addSorted (category = NEW CommandCategory (title));
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KnownCommand* CommandContainer::addCommand (const KnownCommand& c)
{
	CommandCategory* category = addCategory (c.getDisplayCategory ());

	KnownCommand* commandCopy = NEW KnownCommand (c);
	category->add (commandCopy, false);
	return commandCopy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandContainer::addBuiltInCommands ()
{
	CommandTable::instance ().getCategories (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandContainer::loadCommands (const IAttributeList& attributes)
{
	AttributeAccessor a (const_cast<IAttributeList&> (attributes));
	while(UnknownPtr<IAttributeList> categoryAttribs = a.unqueueUnknown ("categories"))
	{
		AttributeAccessor ca (*categoryAttribs);
		StringRef displayCategory = ca.getString ("displayName");
		CommandCategory* category = addCategory (displayCategory);

		while(UnknownPtr<IAttributeList> commandAttribs = ca.unqueueUnknown ("commands"))
		{
			AttributeAccessor a (*commandAttribs);

			CommandDescription desc;
			desc.category = a.getCString ("category");
			desc.name = a.getCString ("name");
			desc.displayCategory = displayCategory;
			desc.displayName = a.getString ("displayName");
			desc.englishName = a.getCString ("englishName");

			KnownCommand* commandCopy = NEW KnownCommand (desc);
			category->add (commandCopy, false);

			commandAttribs->release ();
		}
		categoryAttribs->release ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandContainer::saveCommands (IAttributeList& attributes) const
{
	AttributeAccessor rootAttribs (attributes);

	IAttributeQueue* categoryQueue = rootAttribs.newAttributeQueue ();
	if(!categoryQueue)
		return false;

	rootAttribs.set ("categories", categoryQueue, Attributes::kOwns);

	ForEach (categories, CommandCategory, category)
		IAttributeList* categoryAttr = rootAttribs.newAttributes ();
		if(!categoryAttr)
			return false;

		categoryQueue->addValue (categoryAttr, Attributes::kOwns);

		AttributeAccessor categoryAttribs (*categoryAttr);
		categoryAttribs.set ("displayName", category->getDisplayCategory ());	
		
		IAttributeQueue* commandQueue = rootAttribs.newAttributeQueue ();
		categoryAttribs.set ("commands", commandQueue, Attributes::kOwns);

		IterForEachUnknown (category->newCommandIterator (), unk)
			if(UnknownPtr<ICommand> c = unk)
			{
				CommandDescription description;
				c->getDescription (description);
				if(description.flags & CommandFlags::kHidden)
					continue;

				IAttributeList* commandAttr = rootAttribs.newAttributes ();
				AttributeAccessor commandAttribs (*commandAttr);
				commandAttribs.set ("displayName", description.displayName);
				commandAttribs.set ("name", description.name);
				commandAttribs.set ("category", description.category);
				commandAttribs.set ("englishName", description.englishName);

				commandQueue->addValue (commandAttr, Attributes::kOwns);
			}
		EndFor
	EndFor
	return true;
}
