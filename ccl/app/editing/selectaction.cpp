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
// Filename    : ccl/app/editing/selectaction.cpp
// Description : Select Action
//
//************************************************************************************************

#include "ccl/app/editing/selectaction.h"
#include "ccl/app/editing/selection.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editor.h"
#include "ccl/app/actions/iactioncontext.h"

#include "ccl/base/collections/arraybox.h"

#include "ccl/public/base/variant.h"

using namespace CCL;

//************************************************************************************************
// SetFocusAction
//************************************************************************************************

class SetFocusAction: public Action
{
public:
	DECLARE_CLASS_ABSTRACT (SetFocusAction, Action)

	SetFocusAction (Object* focusItem, EditorComponent& editor, EditView* editView);

	// Action
	bool execute () override;
	bool undo () override;
	bool redo () override;

protected:
	SharedPtr<Object> focusItem;
	ObservedPtr<EditModel> editModel;
	ObservedPtr<EditView> editView;
	MetaClassRef editorClass;

	bool setFocus ();
};

//************************************************************************************************
// UnfocusAction
//************************************************************************************************

class UnfocusAction: public SetFocusAction
{
public:
	DECLARE_CLASS_ABSTRACT (UnfocusAction, SetFocusAction)

	UnfocusAction (Object* focusItem, EditorComponent& editor, EditView* editView);

	// Action
	bool execute () override;
	bool undo () override;
	bool redo () override;
};

//************************************************************************************************
// SelectFunctions
//************************************************************************************************

SelectFunctions* SelectFunctions::createInstance (EditorComponent& editor, IUnknown* unknown)
{
	UnknownPtr<IActionContext> actionContext (unknown);
	if(actionContext)
		return NEW SelectFunctions (editor, actionContext);

	ActionExecuter* executer = unknown_cast<ActionExecuter> (unknown);
	if(executer)
		return NEW SelectFunctions (editor, *executer);

	CCL_DEBUGGER ("Can't create SelectFunctions!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (SelectFunctions, ActionExecuter)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectFunctions::SelectFunctions (EditorComponent& editor, IActionContext* context, bool exclusive)
: ActionExecuter (context),
  editor (editor),
  selectAction (nullptr),
  focusItem (nullptr),
  focusView (nullptr),
  flags (0)
{
	makeItemsVisible (true);
	selectExclusive (exclusive);

	editor.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectFunctions::SelectFunctions (EditorComponent& editor, const ActionExecuter& otherExecuter, bool exclusive)
: ActionExecuter (otherExecuter.getActionContext ()),
  editor (editor),
  selectAction (nullptr),
  focusItem (nullptr),
  focusView (nullptr),
  flags (0)
{
	makeItemsVisible (true);
	selectExclusive (exclusive);

	editor.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectFunctions::~SelectFunctions ()
{
	ASSERT (!selectAction)
	flushSelectAction (); // only in case of a forgotten endMultiple

	editor.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectFunctions::flushSelectAction ()
{
	if(selectAction)
	{
		selectAction->selectExclusive (selectExclusive ());
		selectAction->makeItemsVisible (makeItemsVisible ());
		execute (selectAction);
		selectAction = nullptr;
	}

	if(focusItem)
	{
		execute (NEW SetFocusAction (focusItem, editor, focusView));
		focusItem = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* SelectFunctions::beginMultiple (StringRef description, StringRef details)
{
	ASSERT (!selectAction)
	if(!selectAction)
		selectAction = NEW SelectAction (editor);

	return ActionExecuter::beginMultiple (description, details);
}

///////////////////////////////////////////////////////////////°///////////////////////////////////

bool SelectFunctions::endMultiple (bool cancel)
{
	flushSelectAction ();

	return ActionExecuter::endMultiple (cancel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectFunctions::select (Object* item)
{
	SelectAction* action = selectAction;
	if(!action)
		action = NEW SelectAction (editor);

	action->addCandidate (item);

	if(focusFirstItem ())
	{
		ASSERT (!focusItem)
		focusItem = item;
		focusFirstItem (false);
	}

	if(!selectAction) // not in multiaction, execute our local action now
	{
		selectAction = action;
		flushSelectAction ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectFunctions::select (Container& items)
{
	SelectAction* action = selectAction;
	if(!action)
		action = NEW SelectAction (editor);

	ForEach (items, Object, item)
		action->addCandidate (item);
	EndFor

	if(focusFirstItem ())
	{
		ASSERT (!focusItem)
		focusItem = items.at (0);
		if(focusItem)
			focusFirstItem (false);
	}

	if(!selectAction) // not in multiaction, execute our local action now
	{
		selectAction = action;
		flushSelectAction ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectFunctions::saveSelection ()
{
	// allow restoring the current selection as part of a later undo step:
	// pretend that all currently selected items will be deselected
	UnselectAction* unselectAction = NEW UnselectAction (editor);
	unselectAction->addCandidatesFrom (editor.getModel ().getSelection ());
	unselectAction->selectExclusive (true);
	unselectAction->makeItemsVisible (makeItemsVisible ());
	unselectAction->setExecuted (true); // don't do it, they will be unselected by other means
	return execute (unselectAction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectFunctions::takeSnapshot ()
{
	// create a select action that would select the items that are currently selected (e.g. as part of a later redo)
	SelectAction* selectAction = NEW SelectAction (editor);
	selectAction->addCandidatesFrom (editor.getModel ().getSelection ());
	selectAction->selectExclusive (true);
	selectAction->makeItemsVisible (makeItemsVisible ());
	selectAction->setExecuted (true); // don't do it, they are already selected
	return execute (selectAction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectFunctions::saveFocusItem (Object* item, EditView* editView)
{
	return execute (NEW UnfocusAction (item, editor, editView));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectFunctions::setFocusItem (Object* item, EditView* view)
{
	focusItem = item;
	focusView = view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectFunctions::focusFirstSelected (EditView* view)
{
	focusView = view;
	focusFirstItem (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectFunctions::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "selectExclusive")
	{
		var = selectExclusive ();
		return true;
	}
	if(propertyId == "makeItemsVisible")
	{
		var = makeItemsVisible ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectFunctions::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "selectExclusive")
	{
		selectExclusive (var.asBool ());
		return true;
	}
	if(propertyId == "makeItemsVisible")
	{
		makeItemsVisible (var.asBool ());
		return true;
	}
	if(propertyId == "focusItem")
	{
		Object* item = unknown_cast<Object> (var);
		setFocusItem (item);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SelectFunctions)
	DEFINE_METHOD_NAME ("select")
	DEFINE_METHOD_NAME ("selectMultiple")
	DEFINE_METHOD_NAME ("saveSelection")
	DEFINE_METHOD_NAME ("takeSnapshot")
END_METHOD_NAMES (SelectFunctions)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectFunctions::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "select")
	{
		bool result = false;
		if(Object* item = unknown_cast<Object> (msg[0]))
			result = select (item);
		returnValue = result;
		return true;
	}
	else if(msg == "selectMultiple")
	{
		bool result = false;
		AutoPtr<Container> items = ArrayBox::convert (msg[0]);
		if(items)
			result = select (*items);
		returnValue = result;
		return true;
	}
	else if(msg == "saveSelection")
	{
		bool result = saveSelection ();
		returnValue = result;
		return true;
	}
	else if(msg == "takeSnapshot")
	{
		bool result = takeSnapshot ();
		returnValue = result;
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// SelectAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SelectAction, Action)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectAction::SelectAction (EditorComponent& editor)
: selection (&editor.getModel ().getSelection ()),
  editorClass (editor.myClass ()),
  flags (0)
{
	candidates.objectCleanup (true);

	makeItemsVisible (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection* SelectAction::getSelection ()
{
	// selection might be dead already, check for resurrection in another editor instance
	if(!selection)
		if(EditorComponent* editor = EditorRegistry::instance ().findEditor (editorClass))
		{
			Object* firstCandidate = candidates.getFirst ();
			if(!firstCandidate || editor->getModel ().canSelectItem (firstCandidate))
				selection = &editor->getModel ().getSelection ();
		}

	return selection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectAction::addCandidate (Object* object)
{
	ASSERT (object != nullptr)
	if(!object)
		return;

	object->retain ();
	candidates.add (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectAction::addCandidatesFrom (Selection& selection)
{
	for(int t = 0; t < selection.countTypes (); t++)
	{
		IterForEach (selection.newIterator (t), Object, object)
			addCandidate (object);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectAction::selectAll ()
{
	Selection* selection = getSelection ();
	if(!selection || candidates.isEmpty ())
		return false;

	selection->hide (false);
	if(selectExclusive ())
		selection->unselectAll ();

	ListForEachObject (candidates, Object, object)
		selection->select (object);
	EndFor

	selection->show (true);
	if(makeItemsVisible ())
		selection->makeItemsVisible (true); // relaxed
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectAction::unselectAll ()
{
	Selection* selection = getSelection ();
	if(!selection || candidates.isEmpty ())
		return false;

	selection->hide (false);
	ListForEachObject (candidates, Object, object)
		selection->unselect (object);
	EndFor
	selection->show (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectAction::execute ()
{
	return selectAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectAction::undo ()
{
	// Selection might be dead already, always return true here!
	unselectAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectAction::redo ()
{
	// Selection might be dead already, always return true here!
	selectAll ();
	return true;
}

//************************************************************************************************
// UnselectAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UnselectAction, SelectAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnselectAction::execute ()
{
	return unselectAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnselectAction::undo ()
{
	selectAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnselectAction::redo ()
{
	unselectAll ();
	return true;
}


//************************************************************************************************
// SetFocusAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SetFocusAction, Action)

//////////////////////////////////////////////////////////////////////////////////////////////////

SetFocusAction::SetFocusAction (Object* focusItem, EditorComponent& editor, EditView* editView)
: focusItem (focusItem),
  editModel (&editor.getModel ()),
  editorClass (editor.myClass ()),
  editView (editView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetFocusAction::setFocus ()
{
	ASSERT (focusItem)
	if(!focusItem)
		return false;

	if(!editModel)
		if(EditorComponent* editor = EditorRegistry::instance ().findEditor (editorClass))
			if(editor->getModel ().canSelectItem (focusItem))
				editModel = &editor->getModel ();

	if(editModel)
		editModel->setFocusItem (focusItem, editView);
	return true; // may succeed next time
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetFocusAction::execute ()
{
	return setFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetFocusAction::undo ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetFocusAction::redo ()
{
	setFocus ();
	return true;
}

//************************************************************************************************
// UnfocusAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UnfocusAction, Action)

//////////////////////////////////////////////////////////////////////////////////////////////////

UnfocusAction::UnfocusAction (Object* focusItem, EditorComponent& editor, EditView* editView)
: SetFocusAction (focusItem, editor, editView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnfocusAction::execute ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnfocusAction::undo ()
{
	setFocus ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnfocusAction::redo ()
{
	return true;
}
