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
// Filename    : ccl/app/options/commandoption.h
// Description : Command Option
//
//************************************************************************************************

#ifndef _ccl_commandoption_h
#define _ccl_commandoption_h

#include "ccl/app/options/useroption.h"

#include "ccl/base/storage/textfile.h"

namespace CCL {

interface IMenu;
interface ICommandEditor;
class SignalSink;

//************************************************************************************************
// CommandSaver
//************************************************************************************************

class CommandSaver: private ITextPromise
{
public:
	static void getLocation (Url& path);	///< file location for user commands
	static bool store ();					///< store user commands
	static bool restore ();					///< restore user commands
	
	static bool exportText (UrlRef path, IUnknownIterator* categories);

protected:
	// ITextPromise
	void createText (TextBlock& block, StringRef title, VariantRef data) const override;
};

//************************************************************************************************
// CommandOption
//************************************************************************************************

class CommandOption: public UserOption
{
public:
	DECLARE_CLASS (CommandOption, UserOption)

	CommandOption (StringRef name = nullptr);
	~CommandOption ();

	// UserOption
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	enum Tags
	{
		kCommandScheme = 100,
		kInputLanguage 
	};

	SignalSink& localeSink;

	virtual void extendSchemeMenu (IMenu& menu) {}
};

//************************************************************************************************
// CommandSchemeOption
//************************************************************************************************

class CommandSchemeOption: public CommandOption
{
public:
	DECLARE_CLASS (CommandSchemeOption, CommandOption)

	CommandSchemeOption ();

	static StringRef Name ();

	// CommandOption
	void CCL_API opened () override;
	tbool CCL_API apply () override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	int currentSchemeIndex;

	void loadScheme (int index);
};

//************************************************************************************************
// CommandEditorOption
//************************************************************************************************

class CommandEditorOption: public CommandOption
{
public:
	DECLARE_CLASS (CommandEditorOption, CommandOption)

	CommandEditorOption ();
	~CommandEditorOption ();

	static StringRef Name ();
	static bool showCurrentCommandsText ();	///< show current commands external viewer

	void setInitialCommand (StringID commandCategory, StringID commandName);

	// CommandOption
	void CCL_API opened () override;
	void CCL_API closed () override;
	tbool CCL_API apply () override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	ICommandEditor* editor;
	MutableCString initialCommandCategory;
	MutableCString initialCommandName;

	void initScheme ();
	bool loadScheme (UrlRef path, StringRef title);

	// CommandOption
	void extendSchemeMenu (IMenu& menu) override;
};

} // namespace CCL

#endif // _ccl_commandoption_h
