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
// Filename    : ccl/app/editing/selectaction.h
// Description : Select Action
//
//************************************************************************************************

#ifndef _ccl_selectaction_h
#define _ccl_selectaction_h

#include "ccl/app/actions/action.h"
#include "ccl/app/actions/actionexecuter.h"
#include "ccl/app/editing/selection.h"

namespace CCL {

class SelectAction;
class Selection;
class EditView;
class EditorComponent;

//************************************************************************************************
// SelectFunctions
//************************************************************************************************

class SelectFunctions: public CCL::ActionExecuter
{
public:
	DECLARE_CLASS_ABSTRACT (SelectFunctions, ActionExecuter)
	DECLARE_METHOD_NAMES (SelectFunctions)

	SelectFunctions (EditorComponent& editor, IActionContext* context, bool exclusive = false);
	SelectFunctions (EditorComponent& editor, const ActionExecuter& otherExecuter, bool exclusive = false);
	~SelectFunctions ();

	static SelectFunctions* createInstance (EditorComponent& editor, IUnknown* unknown);

	PROPERTY_FLAG (flags, 1<<0, selectExclusive)	///< select the candidates exclusively (unselects all before)
	PROPERTY_FLAG (flags, 1<<1, makeItemsVisible)	///< try to make the selected items visible (e.g. by scrolling); enabled by default

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Selection Functions
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool select (Object* item);
	bool select (Container& items);
	bool saveSelection ();				///< save the current selection state (for restoring on undo)
	bool takeSnapshot ();				///< recreate the current selection state (for restoring on redo)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Focus Functions
	//////////////////////////////////////////////////////////////////////////////////////////////

	void setFocusItem (Object* item, EditView* editView = nullptr);
	void focusFirstSelected (EditView* editView = nullptr);			///< the first selected item will become focusItem
	bool saveFocusItem (Object* item, EditView* editView = nullptr);	///< the given item will become focus item on undo

	// ActionExecuter
	Action* beginMultiple (StringRef description, StringRef details = nullptr) override;
	bool endMultiple (bool cancel = false) override;

private:
	EditorComponent& editor;
	SelectAction* selectAction;
	Object* focusItem;
	EditView* focusView;
	int flags;

	PROPERTY_FLAG (flags, 1<<2, focusFirstItem)

	void flushSelectAction ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// SelectAction
//************************************************************************************************

class SelectAction: public Action
{
public:
	DECLARE_CLASS_ABSTRACT (SelectAction, Action)

	SelectAction (EditorComponent& editor);

	PROPERTY_FLAG (flags, 1<<0, selectExclusive)	///< select the candidates exclusively (unselects all before)
	PROPERTY_FLAG (flags, 1<<1, makeItemsVisible)	///< try to make the selected items visible (e.g. by scrolling); enabled by default

	void addCandidate (Object* object);				///< shares object
	void addCandidatesFrom (Selection& selection);	///< adds all items from the given selection (of any type)

protected:
	MetaClassRef editorClass;
	ObservedPtr<Selection> selection;
	ObjectList candidates;
	int flags;

	Selection* getSelection ();

	bool selectAll ();
	bool unselectAll ();

	// Action
	bool execute () override;
	bool undo () override;
	bool redo () override;
};

//************************************************************************************************
// UnselectAction
//************************************************************************************************

class UnselectAction: public SelectAction
{
public:
	DECLARE_CLASS_ABSTRACT (UnselectAction, SelectAction)

	UnselectAction (EditorComponent& editor);

protected:
	// Action
	bool execute () override;
	bool undo () override;
	bool redo () override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline UnselectAction::UnselectAction (EditorComponent& editor)
: SelectAction (editor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_selectaction_h
