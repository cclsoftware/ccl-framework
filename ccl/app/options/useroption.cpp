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
// Filename    : ccl/app/options/useroption.cpp
// Description : User Option
//
//************************************************************************************************

#include "ccl/app/options/useroption.h"
#include "ccl/app/options/useroptionelement.h"
#include "ccl/app/params.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/collections/hashmap.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UserOption")
	XSTRING (Options, "Options")
	XSTRING (General, "General")
	XSTRING (Locations, "Locations")
	XSTRING (Advanced, "Advanced")
	XSTRING (Preferences, "Preferences")
END_XSTRINGS

//************************************************************************************************
// UserOption
//************************************************************************************************

StringRef UserOption::Options ()	{ return getOptionString (XSTR_REF (Options)); }
StringRef UserOption::General ()	{ return getOptionString (XSTR_REF (General)); }
StringRef UserOption::Locations ()	{ return getOptionString (XSTR_REF (Locations)); }
StringRef UserOption::Advanced ()	{ return getOptionString (XSTR_REF (Advanced)); }

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UserOption::getOptionString (const LocalString& string)
{
	ITranslationTable* altTable = nullptr;
	if(!System::IsInMainAppModule ())
		altTable = System::GetLocaleManager ().getStrings (ILocaleManager::kMainTableID);
	return string.getText (altTable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (UserOption, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOption::UserOption (StringRef name, StringRef title)
: Component (name, title),
  applyPending (false)
{
	elements.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOption::~UserOption ()
{
	ForEach (elements, UserOptionElement, e)
		if(e->getEditParam ())
			ISubject::removeObserver (e->getEditParam (), this);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionList* UserOption::getOptionList ()
{
	return getParentNode<UserOptionList> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionElement* UserOption::addElement (UserOptionElement* element)
{
	IParameter* editParam = element->getEditParam ();
	ASSERT (editParam != nullptr)
	if(editParam)
	{
		ASSERT (!editParam->getName ().isEmpty ())
		paramList.addShared (editParam);
		ISubject::addObserver (editParam, this);
	}

	if(IParameter* labelParam = element->getLabelParam ())
	{
		ASSERT (!labelParam->getName ().isEmpty ())
		paramList.addShared (labelParam);
	}

	elements.add (element);
	return element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API UserOption::getName () const
{
	return SuperClass::getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API UserOption::getTitle () const
{
	return SuperClass::title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOption::setCategory (StringRef category)
{
	// replace existing category (if any) in title string
	int index = title.index (IUserOption::strSeparator);
	String remainingTitle = title.subString (index + 1);
	String newTitle;
	newTitle << category << IUserOption::strSeparator << remainingTitle;
	setTitle (newTitle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API UserOption::getIcon () const
{
	MutableCString imageName ("OptionIcon:");
	imageName += getName ();

	ITheme* theme = getTheme ();
	ASSERT (theme != nullptr)
	return theme ? theme->getImage (imageName) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOption::needsApply () const
{
	if(applyPending)
		return true;

	ForEach (elements, UserOptionElement, e)
		if(e->needsApply ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOption::apply ()
{
	ForEach (elements, UserOptionElement, e)
		if(e->needsApply ())
			e->apply ();
	EndFor

	applyPending = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOption::opened ()
{
	ForEach (elements, UserOptionElement, e)
		e->init ();
	EndFor

	applyPending = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOption::closed ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOption::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		// update apply state when edit parameter of element changed
		if(UnknownPtr<IParameter> (subject))
			signal (Message (kChanged));
	}

	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API UserOption::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Options")
	{
		ASSERT (formName.isEmpty () == false)

		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		if(theme)
			return theme->createView (formName, this->asUnknown ());
	}
	return nullptr;
}

//************************************************************************************************
// UserOptionList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserOptionList, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionList::UserOptionList (StringRef name, StringRef title)
: Component (name.isEmpty () ? CCLSTR ("UserOptions") : name, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionList::~UserOptionList ()
{
	ASSERT (plugInList.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionList::addOption (UserOption* option)
{
	addChild (option);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionList::PlugInOptionList& UserOptionList::getPlugInList ()
{
	return plugInList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserOption* UserOptionList::findOptionByName (StringRef name) const
{
	for(int i = 0, count = countOptions (); i < count; i++)
		if(IUserOption* option = getOption (i))
			if(option->getName () == name)
				return option;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionList::runDialog (IUserOption* selected)
{
	UserOptionManager::instance ().runDialog (this, selected);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API UserOptionList::getName () const
{
	return SuperClass::getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API UserOptionList::getTitle () const
{
	return SuperClass::getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UserOptionList::countOptions () const
{
	return countChildren () + plugInList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserOption* CCL_API UserOptionList::getOption (int index) const
{
	int childCount = countChildren ();
	if(index < childCount)
		return unknown_cast<UserOption> (getChild (index));
	else
		return plugInList.at (index - childCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API UserOptionList::getLastSelected () const
{
	return lastSelected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOptionList::setLastSelected (StringRef name)
{
	lastSelected = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserOptionList::load (const Storage& storage)
{
	storage.getAttributes ().get (lastSelected, "lastSelected");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserOptionList::save (const Storage& storage) const
{
	storage.getAttributes ().set ("lastSelected", lastSelected);
	return true;
}

//************************************************************************************************
// UserOptionManager
//************************************************************************************************

CCL_KERNEL_TERM (UserOptionManager)
{
	if(UserOptionManager::peekInstance () != nullptr)
	{
		UserOptionManager::instance ().store ();
		UserOptionManager::instance ().removeAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SINGLETON (UserOptionManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionManager::UserOptionManager ()
{
	#if CCL_PLATFORM_MAC
	optionList.setTitle (XSTR (Preferences));
	#else
	optionList.setTitle (XSTR (Options));
	#endif
	optionLists.add (&optionList);

	restore ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UserOptionManager::getTitle () const
{
	return optionList.getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::store ()
{
	Attributes& a = Settings::instance ().getAttributes ("UserOptions");
	optionList.save (Storage (a));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::restore ()
{
	Attributes& a = Settings::instance ().getAttributes ("UserOptions");
	optionList.load (Storage (a));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::add (UserOption* option)
{
	optionList.addOption (option);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::removeAll ()
{
	optionList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::addPlugIns ()
{
	ForEachPlugInClass (PLUG_CATEGORY_USEROPTION, description)
		IUserOption* option = ccl_new<IUserOption> (description.getClassID ());
		ASSERT (option != nullptr)
		if(option)
			optionList.getPlugInList ().add (option);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::removePlugIns ()
{
	VectorForEach (optionList.getPlugInList (), IUserOption*, option)
		ccl_release (option);
	EndFor
	optionList.getPlugInList ().removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::addList (UserOptionList* list)
{
	optionLists.add (list);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::removeList (UserOptionList* list)
{
	optionLists.remove (list);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserOption* UserOptionManager::findOptionByName (StringRef name, UserOptionList** outList) const
{
	if(outList)
		*outList = nullptr;
	ForEach (optionLists, UserOptionList, list)
		if(IUserOption* option = list->findOptionByName (name))
		{
			if(outList)
				*outList = list;
			return option;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionManager::runDialog (UserOptionList* selectedList, IUserOption* selectedOption)
{
	if(selectedList == nullptr)
		selectedList = &optionList;

	AutoPtr<IUserOptionDialog> dialog = ccl_new<IUserOptionDialog> (ClassID::UserOptionDialog);
	ASSERT (dialog != nullptr)
	if(dialog)
	{
		bool empty = true;
		Vector<IUserOptionList*> lists;
		ForEach (optionLists, UserOptionList, list)
			lists.add (list);
			if(list->countOptions () > 0)
				empty = false;
		EndFor

		if(!lists.contains (selectedList))
		{
			lists.add (selectedList);
			if(selectedList->countOptions () > 0)
				empty = false;
		}

		if(empty) // suppress dialog if empty
			return;

		int count = lists.count ();
		int index = lists.index (selectedList);

		if(selectedOption)
			selectedList->setLastSelected (selectedOption->getName ());

		dialog->run (lists, count, index);

		// auto-save settings
		Settings::autoSaveAll ();
	}
}

//************************************************************************************************
// ConfigurationComponent
//************************************************************************************************

namespace CCL {

class ConfigurationComponent: public Component,
                              public ComponentSingleton<ConfigurationComponent>
{
public:
	DECLARE_CLASS (ConfigurationComponent, Component)
	typedef ConfigurationPublisher::ApplyCallback ApplyCallback;

	ConfigurationComponent ();
	~ConfigurationComponent ();

	bool addElement (StringID section, StringID key, IParameter* param, ApplyCallback applyCallback);
	bool addToggleCommand (StringID section, StringID key, StringID commandCategory, StringID commandName);

	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	typedef PointerHashMap<ApplyCallback> CallbackTable;
	CallbackTable callbackTable;

	bool composeParamName (MutableCString& paramName, StringID section, StringID key) const;
	bool decomposeParamName (MutableCString& section, MutableCString& key, StringID paramName) const;

	class ToggleCommandHandler;
};

} // namespace CCL

//************************************************************************************************
// ConfigurationComponent::ToggleCommandHandler
//************************************************************************************************

class ConfigurationComponent::ToggleCommandHandler: public Component
{
public:
	ToggleCommandHandler (StringRef name)
	: Component (name)
	{}

	PROPERTY_MUTABLE_CSTRING (commandCategory, CommandCategory)
	PROPERTY_MUTABLE_CSTRING (commandName, CommandName)
	PROPERTY_SHARED_AUTO (IParameter, parameter, Parameter)

	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		return category == getCommandCategory ();
	}

	tbool CCL_API interpretCommand (const CommandMsg& msg) override
	{
		if(msg.category == getCommandCategory () && msg.name == getCommandName () && parameter)
		{
			if(!msg.checkOnly ())
			{
				bool state = !parameter->getValue ().asBool (); // toggle by default
				CommandAutomator::Arguments (msg).getBool ("State", state);

				parameter->setValue (state, true);
			}
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// ConfigurationComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConfigurationComponent, Component)
DEFINE_COMPONENT_SINGLETON (ConfigurationComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigurationComponent::ConfigurationComponent ()
: Component ("Configuration")
{
	Configuration::Registry::instance ().addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigurationComponent::~ConfigurationComponent ()
{
	Configuration::Registry::instance ().removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationComponent::addElement (StringID section, StringID key, IParameter* param, ApplyCallback applyCallback)
{
	MutableCString paramName;
	composeParamName (paramName, section, key);

	ASSERT (paramList.findParameter (paramName) == nullptr)

	param->setName (paramName);
	Variant var;
	if(Configuration::Registry::instance ().getValue (var, section, key) != 0)
		param->setValue (var, false);

	paramList.add (param);
	if(applyCallback)
		callbackTable.add (param, applyCallback);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConfigurationComponent::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &Configuration::Registry::instance () && msg == kChanged)
	{
		MutableCString section (msg.getArg (0).asString ());
		MutableCString key (msg.getArg (1).asString ());
		MutableCString paramName;
		if(composeParamName (paramName, section, key))
		{
			if(IParameter* param = paramList.findParameter (paramName))
			{
				Variant var;
				if(Configuration::Registry::instance ().getValue (var, section, key) != 0)
					param->setValue (var, false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConfigurationComponent::paramChanged (IParameter* param)
{
	MutableCString section, key;
	if(decomposeParamName (section, key, param->getName ()))
	{
		Configuration::Registry::instance ().removeObserver (this);
		Configuration::Registry::instance ().setValue (section, key, param->getValue ());
		Configuration::Registry::instance ().addObserver (this);

		if(ApplyCallback applyCallback = callbackTable.lookup (param))
			applyCallback ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationComponent::composeParamName (MutableCString& paramName, StringID section, StringID key) const
{
	if(section.isEmpty () || key.isEmpty ())
		return false;
	paramName = section;
	paramName += ".";
	paramName += key;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationComponent::decomposeParamName (MutableCString& section, MutableCString& key, StringID paramName) const
{
	int pointIdx = paramName.lastIndex ('.');
	if(pointIdx < 0)
		return false;

	section = paramName.subString (0, pointIdx);
	key = paramName.subString (pointIdx + 1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationComponent::addToggleCommand (StringID section, StringID key, StringID commandCategory, StringID commandName)
{
	MutableCString paramName;
	if(composeParamName (paramName, section, key))
	{
		IParameter* param = paramList.findParameter (paramName);
		ASSERT (param)
		if(param)
		{
			String name (String () << "ToggleHandler" << (countChildren () + 1));
			ToggleCommandHandler* commandHandler = NEW ToggleCommandHandler (name);
			commandHandler->setCommandCategory (commandCategory);
			commandHandler->setCommandName (commandName);
			commandHandler->setParameter (param);
			addComponent (commandHandler);

			CommandRegistry::addToCommandTable (commandCategory, commandName, 0, "State");
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// ConfigurationPublisher
//************************************************************************************************

bool ConfigurationPublisher::addParam (StringID section, StringID key, IParameter* param, ApplyCallback applyCallback)
{
	return ConfigurationComponent::instance ().addElement (section, key, param, applyCallback);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationPublisher::addBoolParam (StringID section, StringID key, ApplyCallback applyCallback)
{
	return addParam (section, key, NEW Parameter, applyCallback);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationPublisher::addToggleCommand (StringID section, StringID key, StringID commandCategory, StringID commandName)
{
	return ConfigurationComponent::instance ().addToggleCommand (section, key, commandCategory, commandName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IController* ConfigurationPublisher::getSharedInstance ()
{
	if(System::IsInMainAppModule ())
		return &ConfigurationComponent::instance ();
	else
		return UnknownPtr<IController> (System::GetObjectTable ().getObjectByUrl (Url ("object://hostapp/Configuration")));
}
