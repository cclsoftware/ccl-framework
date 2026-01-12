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
// Filename    : ccl/app/editing/editextension.cpp
// Description : Editing Extension
//
//************************************************************************************************

#include "ccl/app/editing/editextension.h"
#include "ccl/app/editing/editview.h"

#include "ccl/public/base/variant.h"

using namespace CCL;

//************************************************************************************************
// EditExtension
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditExtension, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditExtension::extendModel (EditModel& model, EditorComponent& editor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditExtension::createEditHandler (Object* object, EditView& editView, const MouseEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* EditExtension::createDragHandler (UserControl& control, const DragEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditExtension::collectTaskCategories (StringList& taskCategories)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditExtension::canPerformTask (EditView& editView, const EditTaskDescription& task)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditExtension::collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task)
{
	return false;
}

//************************************************************************************************
// EditExtensionRegistry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditExtensionRegistry, EditExtension)
DEFINE_SINGLETON (EditExtensionRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditExtensionRegistry::EditExtensionRegistry ()
{
	extensions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditExtensionRegistry::addExtension (EditExtension* extension)
{
	extensions.add (extension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditExtensionRegistry::extendModel (EditModel& model, EditorComponent& editor)
{
	ForEach (extensions, EditExtension, extension)
		extension->extendModel (model, editor);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditExtensionRegistry::createEditHandler (Object* object, EditView& editView, const MouseEvent& event)
{
	ForEach (extensions, EditExtension, extension)
		EditHandler* handler = extension->createEditHandler (object, editView, event);
		if(handler)
			return handler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* EditExtensionRegistry::createDragHandler (UserControl& control, const DragEvent& event)
{
	ForEach (extensions, EditExtension, extension)
		IDragHandler* handler = extension->createDragHandler (control, event);
		if(handler)
			return handler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditExtensionRegistry::collectTaskCategories (StringList& taskCategories)
{
	ForEach (extensions, EditExtension, extension)
		extension->collectTaskCategories (taskCategories);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditExtensionRegistry::canPerformTask (EditView& editView, const EditTaskDescription& task)
{
	ForEach (extensions, EditExtension, extension)
		if(extension->canPerformTask (editView, task))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditExtensionRegistry::collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task)
{
	bool result = false;
	ForEach (extensions, EditExtension, extension)
		if(extension->collectTaskCandidates (resultList, editView, task))
			result = true;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditExtension* EditExtensionRegistry::getExtension (StringID name) const
{
	ForEach (extensions, EditExtension, extension)
		if(extension->getName () == name)
			return extension;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditExtensionRegistry::getProperty (Variant& var, MemberID propertyId) const
{
	if(EditExtension* ext = getExtension (propertyId))
	{
		var = ext->asUnknown ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
