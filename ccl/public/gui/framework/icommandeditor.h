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
// Filename    : ccl/public/gui/framework/icommandeditor.h
// Description : Key Command Editor Interface
//
//************************************************************************************************

#ifndef _ccl_icommandeditor_h
#define _ccl_icommandeditor_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

struct CommandDescription;
interface ICommandContainer;
interface IUnknownIterator;
interface IAsyncOperation;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (CommandSelector, 0xebd102b8, 0xb508, 0x4153, 0x81, 0x22, 0x18, 0x20, 0x6f, 0x75, 0x4f, 0xd7);
	DEFINE_CID (CommandEditor, 0x211bb2f0, 0xad36, 0x44a8, 0x9f, 0xf1, 0x42, 0xf2, 0x2e, 0x6, 0xbc, 0xcb);
	DEFINE_CID (CommandBarModel, 0x31074e2a, 0xf4b0, 0x4827, 0x87, 0x5, 0xb1, 0xce, 0x6d, 0xe, 0x2f, 0x82);
}

//************************************************************************************************
// ICommandSelector
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandSelector: IUnknown
{
	/** Run command selector dialog. */
	virtual tresult CCL_API run (CommandDescription& command) = 0;

	/** Run command selector asynchronously (dialog or popup). */
	virtual IAsyncOperation* CCL_API runAsync (const CommandDescription& command, tbool popupMode) = 0;

	/** Set available commands (optional, application commands are used by default) . */
	virtual tresult CCL_API setCommands (ICommandContainer* commands) = 0;

	/** Get description of the selected command. */
	virtual tresult CCL_API getSelectedCommand (CommandDescription& command) const = 0;

	// Signals
	DECLARE_STRINGID_MEMBER (kCommandSelected);	///< args[0]: ICommand
	DECLARE_STRINGID_MEMBER (kCommandFocused);	///< args[0]: ICommand

	DECLARE_IID (ICommandSelector)
};

DEFINE_IID (ICommandSelector, 0x62f429cf, 0xd105, 0x4878, 0x8a, 0xff, 0x2d, 0xfc, 0x6e, 0x78, 0x55, 0x0)
DEFINE_STRINGID_MEMBER (ICommandSelector, kCommandSelected, "commandSelected")
DEFINE_STRINGID_MEMBER (ICommandSelector, kCommandFocused, "commandFocused")

//************************************************************************************************
// ICommandEditor
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandEditor: IUnknown
{
	/** Run command editor modal dialog. */
	virtual tresult CCL_API run () = 0;

	/** Initialize editor state (non-modal usage). */
	virtual void CCL_API init (const CommandDescription& command) = 0;

	/** Apply editor state to command table (non-modal usage). */
	virtual void CCL_API apply () = 0;

	/** Load commands from file. */
	virtual tbool CCL_API load (UrlRef path) = 0;

	/** Save commands to file. */
	virtual tbool CCL_API save (UrlRef path) const = 0;

	/** Create iterator of ICommandCategory objects currently in editor. */
	virtual IUnknownIterator* CCL_API newCategoryIterator () const = 0;

	DECLARE_IID (ICommandEditor)
};

DEFINE_IID (ICommandEditor, 0xdae093ae, 0xba4e, 0x435b, 0xa1, 0xb6, 0xa3, 0x8e, 0x92, 0x64, 0x42, 0xf8)

//************************************************************************************************
// ICommandBarItem
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandBarItem: IUnknown
{
	/** Get type of item, e.g. "Button", "Group" or custom types. */
	virtual StringRef CCL_API getType () const = 0;

	/** Count child items. */
	virtual int CCL_API countChilds () const = 0;

	/** Get child item at index. */
	virtual ICommandBarItem* CCL_API getChildItem (int index) const = 0;

	/** Get index of child item. */
	virtual int CCL_API getChildIndex (ICommandBarItem* item) const = 0;

	/** Get property of item (IObject). */
	virtual tbool CCL_API getProperty (Variant& var, CStringRef propertyId) const = 0;

	DECLARE_IID (ICommandBarItem)
};

DEFINE_IID (ICommandBarItem, 0x873ca072, 0xb49, 0x45ba, 0xba, 0x63, 0x3a, 0xe5, 0x49, 0xfb, 0x94, 0xed)

//************************************************************************************************
// ICommandBarModel
/**
	\ingroup gui_command */
//************************************************************************************************

interface ICommandBarModel: IUnknown
{
	/** Get command bar item by id. */
	virtual ICommandBarItem* CCL_API getItemByID (StringRef id) const = 0;

	/** Get parent of given item. */
	virtual ICommandBarItem* CCL_API getParentItem (ICommandBarItem* item) const = 0;

	/** Add a command bar item. */
	virtual const ICommandBarItem* CCL_API addCommandItem (StringRef type, StringRef title, const ICommandBarItem* parentItem = nullptr, int index = -1) = 0;

	/** Remove a command bar item. */
	virtual tbool CCL_API removeCommandItem (ICommandBarItem* item) = 0;

	/** Set a property of an item. Suppoprted properties: title, icon, commandCategory, commandName. */
	virtual tbool CCL_API setItemProperty (const ICommandBarItem* item, CStringRef propertyId, const Variant& var) = 0;

	DECLARE_IID (ICommandBarModel)
};

DEFINE_IID (ICommandBarModel, 0x32187B78, 0x76F5, 0x416A, 0x9C, 0x56, 0xF7, 0x3D, 0x74, 0xC0, 0x82, 0x20)

} // namespace CCL

#endif // _ccl_icommandeditor_h
