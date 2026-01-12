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
// Filename    : ccl/gui/dialogs/commandeditor.h
// Description : Key Command Editor
//
//************************************************************************************************

#ifndef _ccl_commandeditor_h
#define _ccl_commandeditor_h

#include "ccl/gui/dialogs/commandselector.h"

#include "ccl/public/gui/iviewfactory.h"

namespace CCL {

class CommandFile;

//************************************************************************************************
// CommandEditor
//************************************************************************************************

class CommandEditor: public CommandSelector,
					 public ICommandEditor,
					 public IViewFactory
{
public:
	DECLARE_CLASS (CommandEditor, CommandSelector)

	CommandEditor ();
	~CommandEditor ();

	static bool isValidCommandKey (const KeyEvent& key);

	void setState (CommandFile& file);
	void getState (CommandFile& file) const;

	// ICommandEditor
	tresult CCL_API run () override;
	tresult CCL_API run (CommandDescription& command) override; ///< ICommandSelector
	void CCL_API init (const CommandDescription& command) override;
	void CCL_API apply () override;
	tbool CCL_API load (UrlRef path) override;
	tbool CCL_API save (UrlRef path) const override;
	IUnknownIterator* CCL_API newCategoryIterator () const override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE2 (ICommandEditor, IViewFactory, CommandSelector)

private:
	class BindingsListModel;
	class KeyParam;
	class KeyEditBox;
	class InplaceKeyEditBox;
	friend class BindingsListModel;

	BindingsListModel* bindingsList;		///< items for key bindings of current focus command
	KeyParam* keyParam;
	String editSchemeName;
	bool editSchemeModified;

	KnownCommand* findCommand (const KeyEvent& key);
	void updateParamStates ();
	void assignKey ();
	void removeKey ();
	void showCommandForKey ();
	void onKeyEntered ();
	void onKeyItemFocused (int index);
	void setModified (bool state);
	void reset ();

	// CommandSelector
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool onEditKeyColumn (CommandItem& item, const IItemModel::EditInfo& info) override;
	void setFocusCommand (KnownCommand* command) override;
};

} // namespace CCL

#endif // _ccl_commandeditor_h
