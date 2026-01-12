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
// Filename    : ccl/app/editing/tasks/edittask.cpp
// Description : Edit Task
//
//************************************************************************************************

#include "ccl/app/editing/tasks/edittask.h"

#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editor.h"

#include "ccl/app/actions/actionexecuter.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/container.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/itheme.h"

using namespace CCL;

//************************************************************************************************
// EditTask
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTask, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTask::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IEditTask)
	QUERY_INTERFACE (IParamObserver)

	// script edit tasks provide parameters as property members,
	// forward to parameter list provided by context for native edit tasks
	if(paramList && iid == ccl_iid<IController> ())
		return paramList->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTask::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kFormName)
	{
		var = Variant (String (formName), true);
		return true;
	}
	else if(propertyId == kThemeID)
	{
		var = Variant (String (getThemeID ()), true);
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditTask::getTitle () const
{
	String title;
	IUnknown* This = const_cast<EditTask*> (this)->asUnknown ();
	if(const IClassDescription* description = ccl_classof (This))
		description->getLocalizedName (title);
	else
		title = TRANSLATE (String (myClass ().getPersistentName ()));
	ASSERT (!title.isEmpty ())
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString EditTask::getThemeID () const
{
	return RootComponent::instance ().getTheme ()->getThemeID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTask::prepareEdit (IObject& context)
{
	// remember parameter list provided by context
	Variant v;
	context.getProperty (v, "parameters");
	paramList = unknown_cast<ParamContainer> (v);
	ASSERT (paramList.isValid ())
	if(!paramList.isValid ())
		return kResultInvalidArgument;

	construct (context);

	if(!formName.isEmpty ())
		return runDialog (context, formName, getThemeID ());
	else
		return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTask::performEdit (IObject& context)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTask::construct (IObject& context)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTask::paramChanged (IParameter* param)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditTask::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTask::isSilentMode (IObject& context) const
{
	Variant v;
	tbool result = context.invokeMethod (v, Message ("isSilentMode"));
	ASSERT (result)
	return v.asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* EditTask::getArguments (IObject& context) const
{
	Variant v;
	tbool result = context.invokeMethod (v, Message ("getArguments"));
	ASSERT (result)
	UnknownPtr<IAttributeList> arguments (v.asUnknown ());
	return arguments;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTask::restore (IObject& context)
{
	Variant v;
	tbool result = context.invokeMethod (v, Message ("restore"));
	ASSERT (result)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTask::runDialog (IObject& context, StringID formName, StringID _themeID)
{
	Variant v;
	Variant optionalThemeID;
	if(!_themeID.isEmpty ())
	{
		String themeID (_themeID);
		optionalThemeID = themeID;
		optionalThemeID.share ();
	}

	tbool result = context.invokeMethod (v, Message ("runDialog", String (formName), optionalThemeID));
	ASSERT (result)
	return v.asResult ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView* EditTask::getEditView (IObject& context) const
{
	Variant v;
	context.getProperty (v, "editor");
	return unknown_cast<EditView> (v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel* EditTask::getEditModel (IObject& context) const
{
	EditView* editView = getEditView (context);
	return editView ? &editView->getModel () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditTask::getEditor (IObject& context) const
{
	EditView* editView = getEditView (context);
	return editView ? unknown_cast<EditorComponent> (editView->getController ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditEnvironment* EditTask::getEditEnvironment (IObject& context) const
{
	EditorComponent* editor = getEditor (context);
	return editor ? editor->getEditEnvironment () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* EditTask::getIterator (IObject& context) const
{
	Variant v;
	context.getProperty (v, "iterator");
	return unknown_cast<Iterator> (v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionExecuter* EditTask::getFunctions (IObject& context) const
{
	Variant v;
	context.getProperty (v, "functions");
	return unknown_cast<ActionExecuter> (v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IActionContext* EditTask::getActionContext (IObject& context) const
{
	ActionExecuter* executer = getFunctions (context);
	return executer ? executer->getActionContext () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTask::collectEditItems (Container& items, IObject& context, MetaClassRef type, IObjectFilter* filter) const
{
	Iterator* iter = getIterator (context);
	ASSERT (iter != nullptr)
	if(iter)
	{
		iter->first (); // can be called multiple times
		IterForEach (return_shared (iter), Object, object)
			if(object->canCast (type) && (!filter || filter->matches (object->asUnknown ())))
				items.add (object);
		EndFor
	}
	return !items.isEmpty ();
}
