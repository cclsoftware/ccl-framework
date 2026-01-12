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
// Filename    : ccl/app/editing/addins/editaddin.h
// Description : Edit Add-in
//
//************************************************************************************************

#ifndef _ccl_editaddin_h
#define _ccl_editaddin_h

#include "ccl/app/component.h"

#include "ccl/public/app/iedittask.h"
#include "ccl/public/app/ieditenvironment.h"
#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/plugins/pluginst.h"

namespace CCL {

class Selection;
class EditorComponent;
class ActionJournal;

//************************************************************************************************
// EditAddIn
/** Base class for native edit add-ins. */
//************************************************************************************************

class EditAddIn: public Component,
				 public IPersistAttributes,
				 public PluginInstance
{
public:
	DECLARE_CLASS (EditAddIn, Component)

	EditAddIn (StringRef name = nullptr);

	class UndoKeeper;

	/// getWindowClassID for this add-in
	MutableCString getWindowClassID () const;

	/// getWindowClassID for a specific add-in class
	static MutableCString getWindowClassID (MetaClassRef panelClass);
	template<class T> static MutableCString getWindowClassID ();
	
	bool isOpenDetached () const;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API paramChanged (IParameter* param) override;

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;

	CLASS_INTERFACE2 (IPersistAttributes, IPluginInstance, Component)

protected:
	UnknownPtr<IEditEnvironment> environment;

	IParameter* applyButton;
	IParameter* defaultButton;
	UndoKeeper* undoKeeper;

	void initUndoKeeper ();
	void setApplyEnabled (bool state);
	virtual void onApplyButtonHit () {}
	virtual void onDefaultButtonHit ();

	// access to environment
	EditorComponent* getMainEditor () const;
	EditorComponent* getActiveEditor () const;
	Selection* getActiveSelection () const;
	Object* getFocusItem () const;
	Component* getFocusItemPropertyEditor () const;
	ActionJournal* getActionJournal () const;
	bool canRunEditTask (UIDRef cid) const;
	bool runEditTask (UIDRef cid);

	// environment notifications
	virtual void onActiveEditorChanged (MessageRef msg);
	virtual void onSelectionChanged (MessageRef msg);
	virtual void onFocusItemChanged (MessageRef msg);
};

//************************************************************************************************
// EditAddIn::UndoKeeper
//************************************************************************************************

class EditAddIn::UndoKeeper: public Component
{
public:
	DECLARE_CLASS (UndoKeeper, Component)

	UndoKeeper ();

	PROPERTY_VARIABLE (int64, lastApplyTime, LastApplyTime)
	PROPERTY_VARIABLE (int32, lastSelectionTag, LastSelectionTag)

	void beforeApply ();
	void afterApply (bool succeeded);
	
	void onSelectionChanged (MessageRef msg); // called by EditAddIn

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	IParameter* undoIndicator;
	IParameter* undoButton;

	EditAddIn* getAddIn () const;
	void getCurrentState (int64& lastEditTime, int32& selectionTag) const;

	bool canUndo () const;
	void setUndoIndicator (bool state);

	void onActionJournalChanged (MessageRef msg);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline MutableCString EditAddIn::getWindowClassID () { return getWindowClassID (ccl_typeid<T> ()); }

} // namespace CCL

#endif // _ccl_editaddin_h
