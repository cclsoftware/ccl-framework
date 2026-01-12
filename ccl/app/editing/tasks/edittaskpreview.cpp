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
// Filename    : ccl/app/editing/tasks/edittaskpreview.cpp
// Description : Edit Task with Preview
//
//************************************************************************************************

#include "ccl/app/editing/tasks/edittaskpreview.h"

#include "ccl/app/actions/action.h"
#include "ccl/app/actions/iactioncontext.h"
#include "ccl/app/actions/actionjournal.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"

using namespace CCL;

//************************************************************************************************
// EditTaskComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTaskComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskComponent::EditTaskComponent (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("EditTask") : name),
  outerTask (nullptr),
  applyNeeded (true),
  applyButton (nullptr),
  defaultButton (nullptr)
{
	defaultButton = paramList.addParam ("setDefault", 'dflt');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskComponent::storeValues (IAttributeList& values) const
{
	Attributes a;
	paramList.storeValues (a, true);
	values.copyFrom (a);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskComponent::restoreValues (const IAttributeList& values)
{
	Attributes a;
	a.copyFrom (values);
	paramList.restoreValues (a, true, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskComponent::runDialog ()
{
	ASSERT (!formName.isEmpty ())
	IView* view = getTheme ()->createView (formName, this->asUnknown ());
	ASSERT (view)
	return view ? DialogBox ()->runDialog (view) == DialogResult::kOkay : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskComponent::isApplyNeeded () const
{
	return applyNeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskComponent::setApplyNeeded (bool state)
{
	applyNeeded = state;
	if(applyButton)
		applyButton->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditTaskComponent::setDialogButton (IParameter* button, int which)
{
	if(which == DialogResult::kApply)
	{
		applyButton = button;
		if(applyButton)
			applyButton->enable (applyNeeded);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskComponent::onDialogButtonHit (int which)
{
	if(which == DialogResult::kApply)
	{
		bool done = false;
		ASSERT (outerTask)
		if(outerTask)
			done = outerTask->apply ();

		setApplyNeeded (!done);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskComponent::onDefaultButtonHit ()
{
	paramList.setDefaultValues (true, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskComponent::paramChanged (IParameter* param)
{
	if(param == defaultButton)
	{
		onDefaultButtonHit ();
		return true;
	}

	setApplyNeeded (true);
	return true;
}

//************************************************************************************************
// EditTaskWithPreview
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EditTaskWithPreview, EditTask)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskWithPreview::EditTaskWithPreview ()
: currentContext (nullptr),
  currentAction (nullptr),
  candidateCounter (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskWithPreview::prepareEdit (IObject& context)
{
	component = createComponent ();
	ASSERT (component)
	component->setOuterTask (this);
	candidateCounter = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskWithPreview::performEdit (IObject& context)
{
	IActionContext* actionContext = getActionContext (context);
	ActionJournal* journal = actionContext ? actionContext->getActionJournal () : nullptr;
	Action* multiAction = journal ? journal->peekMultiple () : nullptr;
	ASSERT (multiAction && component)
	if(!multiAction || !component)
		return kResultUnexpected;

	ASSERT (multiAction->hasSubActions () == false)

	candidateCounter++;
	ScopedVar<IObject*> contextScope (currentContext, &context);
	ScopedVar<Action*> actionScope (currentAction, multiAction);

	bool done = false;
	if(isSilentMode (context) || candidateCounter > 1)
	{
		done = apply ();
	}
	else
	{
		if(component->runDialog  ())
		{
			done = true;
			if(component->isApplyNeeded ())
				done = apply ();
		}
	}

	return done ? kResultOk : kResultAborted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskWithPreview::storeValues (IAttributeList& values) const
{
	ASSERT (component)
	if(!component)
		return kResultUnexpected;

	return component->storeValues (values);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskWithPreview::restoreValues (const IAttributeList& values)
{
	ASSERT (component)
	if(!component)
		return kResultUnexpected;

	return component->restoreValues (values);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskWithPreview::revert ()
{
	ASSERT (currentAction)
	if(currentAction && currentAction->hasSubActions ())
	{
		currentAction->undoAll ();
		currentAction->removeSubActions ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskWithPreview::apply ()
{
	ASSERT (component && currentContext)
	if(!component || !currentContext)
		return false;

	// revert to initial state
	revert ();

	// prepare inner task
	AutoPtr<EditTask> task = createInnerTask ();
	ASSERT (task)

	tresult tr = task->prepareEdit (*currentContext);
	ASSERT (tr == kResultOk)

	// transfer configuration from component
	UnknownPtr<IPersistAttributes> persistTask (static_cast<IEditTask*> (task));
	if(persistTask)
	{
		Attributes a;
		component->storeValues (a);
		persistTask->restoreValues (a);
	}

	// perform inner task
	tr = task->performEdit (*currentContext);
	bool done = tr == kResultOk;

	if(!done) // revert if canceled or error occurred
		revert ();
	
	return done;
}
