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
// Filename    : ccl/app/editing/addins/editaddin.cpp
// Description : Edit Add-in
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/addins/editaddin.h"

#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/selection.h"
#include "ccl/app/actions/actionjournal.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// EditAddIn
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditAddIn, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddIn::EditAddIn (StringRef name)
: Component (name),
  applyButton (nullptr),
  defaultButton (nullptr),
  undoKeeper (nullptr)
{
	applyButton = paramList.addParam ("apply", 'aply');
	applyButton->enable (false);

	defaultButton = paramList.addParam ("setDefault", 'dflt');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::initUndoKeeper ()
{
	ASSERT (undoKeeper == nullptr)
	addComponent (undoKeeper = NEW UndoKeeper);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::initialize (IUnknown* context)
{
	environment = context;

	UnknownPtr<ISubject> subject (environment);
	ASSERT (subject.isValid ())
	signalSlots.advise (subject, IEditEnvironment::kActiveEditorChanged, this, &EditAddIn::onActiveEditorChanged);
	signalSlots.advise (subject, IEditEnvironment::kSelectionChanged, this, &EditAddIn::onSelectionChanged);
	signalSlots.advise (subject, IEditEnvironment::kFocusItemChanged, this, &EditAddIn::onFocusItemChanged);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::terminate ()
{
	UnknownPtr<ISubject> subject (environment);
	ASSERT (subject.isValid ())
	signalSlots.unadvise (subject);

	tresult tr = SuperClass::terminate ();
	environment.release ();
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString EditAddIn::getWindowClassID (MetaClassRef panelClass)
{
	MutableCString windowClassId;
	UID (panelClass.getClassID ()).toCString (windowClassId);
	return windowClassId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString EditAddIn::getWindowClassID () const
{
	return getWindowClassID (myClass ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddIn::isOpenDetached () const
{
	// in the detached case, we are the controller of our own window
	return System::GetWindowManager ().isWindowOpen (getWindowClassID ())
		&& System::GetDesktop ().getWindowByOwner (ccl_as_unknown (this)) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddIn::paramChanged (IParameter* param)
{
	if(param == applyButton)
		onApplyButtonHit ();
	else if(param == defaultButton)
		onDefaultButtonHit ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::setApplyEnabled (bool state)
{
	applyButton->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::onDefaultButtonHit ()
{
	paramList.setDefaultValues (true, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::storeValues (IAttributeList& values) const
{
	Attributes a;
	paramList.storeValues (a, true);
	values.copyFrom (a);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::restoreValues (const IAttributeList& values)
{
	Attributes a;
	a.copyFrom (values);
	paramList.restoreValues (a, true, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditAddIn::getMainEditor () const
{
	return environment ? unknown_cast<EditorComponent> (environment->getMainEditor ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditAddIn::getActiveEditor () const
{
	return environment ? unknown_cast<EditorComponent> (environment->getActiveEditor ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection* EditAddIn::getActiveSelection () const
{
	return environment ? unknown_cast<Selection> (environment->getActiveSelection ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditAddIn::getFocusItem () const
{
	return environment ? unknown_cast<Object> (environment->getFocusItem ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* EditAddIn::getFocusItemPropertyEditor () const
{
	return environment ? unknown_cast<Component> (environment->getFocusItemPropertyEditor ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* EditAddIn::getActionJournal () const
{
	return environment ? unknown_cast<ActionJournal> (environment->getActionJournal ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddIn::canRunEditTask (UIDRef cid) const
{
	return environment && environment->canRunEditTask (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddIn::runEditTask (UIDRef cid)
{
	return environment && environment->runEditTask (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::onActiveEditorChanged (MessageRef msg)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::onSelectionChanged (MessageRef msg)
{
	if(undoKeeper)
		undoKeeper->onSelectionChanged (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::onFocusItemChanged (MessageRef msg)
{}

//************************************************************************************************
// EditAddIn::UndoKeeper
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditAddIn::UndoKeeper, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddIn::UndoKeeper::UndoKeeper ()
: Component (String ("UndoKeeper")),
  lastApplyTime (0),
  lastSelectionTag (0),
  undoIndicator (nullptr),
  undoButton (nullptr)
{
	undoIndicator = paramList.addParam ("canUndo", 'canU');
	undoIndicator->enable (false);
	undoButton = paramList.addParam ("undo", 'undo');
	undoButton->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::beforeApply ()
{
	if(canUndo ())
		getAddIn ()->getActionJournal ()->undo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::afterApply (bool succeeded)
{
	if(succeeded)
		getCurrentState (lastApplyTime, lastSelectionTag);
	setUndoIndicator (succeeded);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddIn* EditAddIn::UndoKeeper::getAddIn () const
{
	EditAddIn* addIn = getParentNode<EditAddIn> ();
	ASSERT (addIn != nullptr)
	return addIn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::getCurrentState (int64& lastEditTime, int32& selectionTag) const
{
	EditAddIn* addIn = getAddIn ();
	lastEditTime = addIn->getActionJournal ()->getLastEditTime ();
	selectionTag = 0;
	if(Selection* selection = addIn->getActiveSelection ())
		selectionTag = selection->getEditTag ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddIn::UndoKeeper::canUndo () const
{
	if(lastApplyTime == 0) // not yet applied
		return false;

	int64 lastEditTime = 0;
	int32 selectionTag = 0;
	getCurrentState (lastEditTime, selectionTag);

	return lastEditTime == lastApplyTime && selectionTag == lastSelectionTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::setUndoIndicator (bool state)
{
	undoIndicator->setValue (state);
	undoButton->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::UndoKeeper::initialize (IUnknown* context)
{
	signalSlots.advise (getAddIn ()->getActionJournal (), nullptr, this, &UndoKeeper::onActionJournalChanged);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddIn::UndoKeeper::terminate ()
{
	signalSlots.unadvise (getAddIn ()->getActionJournal ());

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddIn::UndoKeeper::paramChanged (IParameter* param)
{
	if(param == undoButton)
	{
		beforeApply ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::onActionJournalChanged (MessageRef msg)
{
	setUndoIndicator (canUndo ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddIn::UndoKeeper::onSelectionChanged (MessageRef msg)
{
	setUndoIndicator (canUndo ());
}
