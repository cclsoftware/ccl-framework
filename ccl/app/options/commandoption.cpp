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
// Filename    : ccl/app/options/commandoption.cpp
// Description : Command Option
//
//************************************************************************************************

#include "ccl/app/options/commandoption.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/app/params.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/itextbuilder.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/icommandeditor.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UserOption")
	XSTRING (Import, "Import")
	XSTRING (Export, "Export")
	XSTRING (ExportText, "Export as Text")
	XSTRING (KeyboardShortcuts, "Keyboard Shortcuts")
	XSTRING (AskRevertCommandScheme, "Do you want to revert the keyboard mapping scheme to \"%(1)\"?")
END_XSTRINGS

static String getAppShortcutsTitle ()
{
	String title;
	title << RootComponent::instance ().getApplicationTitle () << " " << XSTR (KeyboardShortcuts);
	return title;
}

//************************************************************************************************
// CommandSaver
//************************************************************************************************

void CommandSaver::getLocation (Url& path)
{
	System::GetSystem ().getLocation (path, System::kAppSettingsFolder);
	String fileName ("user");
	fileName << "." << System::GetCommandTable ().getCommandFileType ().getExtension ();
	path.descend (fileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandSaver::store ()
{
	Url path;
	getLocation (path);
	return System::GetCommandTable ().saveCommands (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandSaver::restore ()
{
	Url path;
	getLocation (path);
	if(!System::GetFileSystem ().fileExists (path))
		return true;

	return System::GetCommandTable ().loadCommands (path, ICommandTable::kOverwriteExisting) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandSaver::exportText (UrlRef path, IUnknownIterator* categories)
{
	CommandSaver saver;
	String title = getAppShortcutsTitle ();
	return TextUtils::saveTextBlock (path, title, categories, saver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSaver::createText (TextBlock& block, StringRef title, VariantRef data) const
{
	UnknownPtr<IUnknownIterator> categories (data.asUnknown ());
	ASSERT (categories.isValid ())

	block << Text::Heading (Text::kH1, title);

	IterForEachUnknown (return_shared (static_cast<IUnknownIterator*> (categories)), unk)
		UnknownPtr<ICommandCategory> category (unk);
		ASSERT (category)
		if(!category)
			continue;

		block << Text::Heading (Text::kH2, String (category->getDisplayCategory ()));
		
		AutoPtr<ITextTable> table = block->createTable ();
		
		int rowCount = IterCountData (category->newCommandIterator ());

		table->construct (rowCount, 2);

		int rowIndex = 0;
		IterForEachUnknown (category->newCommandIterator (), unk)
			UnknownPtr<ICommand> command (unk);
			ASSERT (command)
			if(!command)
				continue;

			CommandDescription description;
			command->getDescription (description);

			ITextTable& t = *table;
			t[rowIndex][0].setContent (Text::Plain (description.displayName));
			
			bool first = true;
			TextBlock keyBlock (block.getBuilder ());
			IterForEachUnknown (command->newBindingIterator (), unk)
				UnknownPtr<IKeyBinding> binding (unk);
				ASSERT (binding)
				if(binding)
				{
					KeyEvent key;
					binding->copyTo (key);
					String keyString;
					key.toString (keyString, true);

					if(!first)
						keyBlock << Text::Break ();

					keyBlock << Text::Plain (keyString);
					first = false;
				}
			EndFor

			t[rowIndex][1].setContent (Text::SubBlock (keyBlock));

			rowIndex++;
		EndFor

		block << Text::Table (table);
	EndFor
}

//************************************************************************************************
// CommandEditorOption
//************************************************************************************************

bool CommandEditorOption::showCurrentCommandsText ()
{
	Url path;
	System::GetSystem ().getLocation (path, System::kUserContentFolder);
	path.descend (getAppShortcutsTitle ());
	path.setFileType (FileTypes::Html (), true);

	AutoPtr<ICommandEditor> editor = ccl_new<ICommandEditor> (ClassID::CommandEditor);
	ASSERT (editor != nullptr)

	AutoPtr<IUnknownIterator> iter = editor->newCategoryIterator ();
	if(!CommandSaver::exportText (path, iter))
		return false;

	System::GetSystemShell ().openUrl (path);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CommandEditorOption::Name ()
{
	return CCLSTR ("CommandEditorOption");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (CommandEditorOption, CommandOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandEditorOption::CommandEditorOption ()
: CommandOption (Name ()),
  editor (nullptr)
{
	setTitle (String () << General () << strSeparator << XSTR (KeyboardShortcuts));
	setFormName ("CCL/CommandEditorOption");

	paramList.byTag (kCommandScheme)->setSignalAlways (true);

	// restore commands
	CommandSaver::restore ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandEditorOption::~CommandEditorOption ()
{
	ASSERT (editor == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditorOption::setInitialCommand (StringID commandCategory, StringID commandName)
{
	initialCommandCategory = commandCategory;
	initialCommandName = commandName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditorOption::initScheme ()
{
	paramList.byTag (kCommandScheme)->setValue (0);
	String activeSchemeName = System::GetCommandTable ().getActiveSchemeName ();
	if(!activeSchemeName.isEmpty ())
		paramList.byTag (kCommandScheme)->fromString (activeSchemeName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditorOption::extendSchemeMenu (IMenu& menu)
{
	menu.addSeparatorItem ();
	menu.addCommandItem (CommandWithTitle ("Commands", "Import", XSTR (Import)), this, true);
	menu.addCommandItem (CommandWithTitle ("Commands", "Export", XSTR (Export)), this, true);
	menu.addCommandItem (CommandWithTitle ("Commands", "Export Text", XSTR (ExportText)), this, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandEditorOption::loadScheme (UrlRef path, StringRef title)
{
	bool result = false;
	ASSERT (editor)
	if(editor)
	{
		String text;
		text.appendFormat (XSTR (AskRevertCommandScheme), title);
		if(Alert::ask (text) == Alert::kYes)
			result = editor->load (path) != 0;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditorOption::opened ()
{
	ASSERT (editor == nullptr)
	editor = ccl_new<ICommandEditor> (ClassID::CommandEditor);
	ASSERT (editor != nullptr)
	CommandDescription commandDescription;
	commandDescription.category = initialCommandCategory;
	commandDescription.name = initialCommandName;
	editor->init (commandDescription);
	ISubject::addObserver (editor, this);

	initScheme ();

	SuperClass::opened ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditorOption::closed ()
{
	ASSERT (editor != nullptr)
	ISubject::removeObserver (editor, this);
	safe_release (editor);

	SuperClass::closed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API CommandEditorOption::findChild (StringRef id) const
{
	if(id == "CommandEditor")
		return UnknownPtr<IObjectNode> (editor);
	else
		return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditorOption::paramChanged (IParameter* param)
{
	if(param->getTag () == kCommandScheme)
	{
		ListParam* listParam = unknown_cast<ListParam> (param);
		ASSERT (listParam != nullptr)

		bool done = false;
		int index = listParam->getValue ();
		UrlWithTitle* scheme = listParam->getObject<UrlWithTitle> (index);
		ASSERT (scheme)
		if(scheme)
			done = loadScheme (*scheme, scheme->getTitle ());

		if(!done)
			initScheme ();
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditorOption::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Commands")
	{
		if(msg.name == "Import" || msg.name == "Export")
		{
			if(!msg.checkOnly ())
			{
				bool import = msg.name == "Import";

				AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
				fs->addFilter (System::GetCommandTable ().getCommandFileType ());
				if(!import)
					fs->setFileName (RootComponent::instance ().getApplicationTitle ());

				if(fs->run (import ? IFileSelector::kOpenFile : IFileSelector::kSaveFile))
				{
					const IUrl* path = fs->getPath ();
					if(import)
					{
						String title;
						path->getName (title, false);
						title.capitalize ();
						loadScheme (*path, title);
					}
					else
					{
						ASSERT (editor)
						if(editor)
							editor->save (*path);
					}
				}
			}
			return true;
		}
		else if(msg.name == "Export Text")
		{
			if(!msg.checkOnly ())
			{
				AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
				fs->addFilter (FileTypes::Html ());
				#if DEBUG
				fs->addFilter (FileTypes::Rtf ());
				#endif
				fs->setFileName (getAppShortcutsTitle ());

				if(fs->run (IFileSelector::kSaveFile))
				{
					const IUrl* path = fs->getPath ();
					
					ASSERT (editor)
					AutoPtr<IUnknownIterator> iter = editor ? editor->newCategoryIterator () : nullptr;
					if(CommandSaver::exportText (*path, iter))
						System::GetSystemShell ().openUrl (*path);
				}
			}
			return true;
		}
	}
	return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditorOption::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && editor && isEqualUnknown (editor, subject))
	{
		applyPending = true;
		signal (Message (kChanged));
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditorOption::apply ()
{
	ASSERT (editor)
	if(editor)
	{
		editor->apply ();

		// save commands
		CommandSaver::store ();
	}
	return SuperClass::apply ();
}

//************************************************************************************************
// CommandSchemeOption
//************************************************************************************************

StringRef CommandSchemeOption::Name ()
{
	return CCLSTR ("CommandSchemeOption");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (CommandSchemeOption, CommandOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandSchemeOption::CommandSchemeOption ()
: CommandOption (Name ()),
  currentSchemeIndex (0)
{
	setTitle (General ());
	setFormName ("CCL/CommandSchemeOption");

	// restore scheme
	Attributes& a = Settings::instance ().getAttributes ("Commands");
	String schemeId = a.getString ("scheme");
	if(!schemeId.isEmpty ())
	{
		ListParam* listParam = paramList.byTag<ListParam> (kCommandScheme);
		ASSERT (listParam)

		int count = listParam->getMax ().asInt () + 1;
		for(int i = 0; i < count; i++)
		{
			UrlWithTitle* scheme = listParam->getObject<UrlWithTitle> (i);
			if(scheme && scheme->getTitle () == schemeId)
			{
				loadScheme (i);
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSchemeOption::loadScheme (int index)
{
	if(index == currentSchemeIndex)
		return;

	ListParam* listParam = paramList.byTag<ListParam> (kCommandScheme);
	ASSERT (listParam != nullptr)

	UrlWithTitle* scheme = listParam->getObject<UrlWithTitle> (index);
	ASSERT (scheme != nullptr)
	if(scheme == nullptr)
		return;

	CCL_PRINT ("Loading Command Scheme ")
	CCL_PRINTLN (scheme->getTitle ())
	System::GetCommandTable ().loadCommands (*scheme, ICommandTable::kOverwriteExisting);
	Settings::instance ().getAttributes ("Commands").set ("scheme", scheme->getTitle ());

	listParam->setValue (index);

	currentSchemeIndex = index;
	applyPending = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandSchemeOption::opened ()
{
	SuperClass::opened ();

	paramList.byTag (kCommandScheme)->setValue (currentSchemeIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSchemeOption::apply ()
{
	loadScheme (paramList.byTag (kCommandScheme)->getValue ());
	
	return SuperClass::apply ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSchemeOption::paramChanged (IParameter* param)
{
	if(param->getTag () == kCommandScheme)
	{
		applyPending = param->getValue ().asInt () != currentSchemeIndex;
		signal (Message (kChanged));
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//************************************************************************************************
// CommandOption
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CommandOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandOption::CommandOption (StringRef name)
: UserOption (name),
  localeSink (*NEW SignalSink (Signals::kLocales))
{
	// check for locale changes
	localeSink.setObserver (this);
	localeSink.enable (true);

	IParameter* langParam = paramList.addString (CSTR ("language"), kInputLanguage);
	langParam->fromString (String (System::GetLocaleManager ().getInputLanguage ()));

	// add built-in commands
	ListParam* listParam = NEW MenuParam (CSTR ("scheme"));
	paramList.add (listParam, kCommandScheme);
	listParam->appendObject (NEW UrlWithTitle (Url (CCLSTR ("resource:///commands.xml")), RootComponent::instance ().getApplicationTitle ()));

	// collect additional schemes
	StringDictionary dict;
	StorableObject::loadFromFile (dict, Url ("resource:///commandschemes.xml"));
	for(int i = 0; i < dict.countEntries (); i++)
	{		
		Url url;
		url.setProtocol (ResourceUrl::Protocol);
		url.setPath (dict.getValueAt (i));

		tbool exists = System::GetFileSystem ().fileExists (url);
		ASSERT (exists)
		if(!exists)
			continue;

		String title (dict.getKeyAt (i));
		listParam->appendObject (NEW UrlWithTitle (url, title));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandOption::~CommandOption ()
{
	localeSink.enable (false);
	delete &localeSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandOption::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kInputLanguageChanged)
	{
		IParameter* langParam = paramList.byTag (kInputLanguage);
		langParam->fromString (String (System::GetLocaleManager ().getInputLanguage ()));
	}
	else if(msg == IParameter::kExtendMenu)
	{
		if(isEqualUnknown (subject, paramList.byTag (kCommandScheme)))
		{
			UnknownPtr<IMenu> menu (msg.getArg (0));
			ASSERT (menu)
			extendSchemeMenu (*menu);
		}
	}
	else
		SuperClass::notify (subject, msg);
}
