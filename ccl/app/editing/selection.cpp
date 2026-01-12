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
// Filename    : ccl/app/editing/selection.cpp
// Description : Selection
//
//************************************************************************************************

#include "ccl/app/editing/selection.h"

#include "ccl/base/message.h"
#include "ccl/base/kernel.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iarrayobject.h"

using namespace CCL;

//************************************************************************************************
// SelectionContainer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SelectionContainer, Object)

//************************************************************************************************
// SelectionList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SelectionList, SelectionContainer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectionList::SelectionList (bool sharedItems)
: sharedItems (sharedItems)
{
	if(sharedItems)
		items.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::isEmpty () const
{
	return items.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::isMultiple () const
{
	return items.isMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::isSelected (Object* object) const
{
	return items.contains (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::select (Object* object)
{
	if(sharedItems)
		object->retain ();

	return items.add (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::unselect (Object* object)
{
	bool removed = items.remove (object);

	if(sharedItems && removed)
		object->release ();

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::unselectAll ()
{
	if(items.isEmpty ())
		return false;
	items.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassRef SelectionList::getType () const
{
	return ccl_typeid<Object> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* SelectionList::newIterator (MetaClassRef) const
{
	return items.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SelectionList::getFirst () const
{
	return items.getFirst ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectionList::unselectType (MetaClassRef type)
{
	if(type == getType ())
		return unselectAll ();
	return false;
}

//************************************************************************************************
// Selection
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Selection, SelectionContainer)

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection::Selection ()
: showHideSuspended (false),
  editTag (0),
  lastEditTag (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection::~Selection ()
{
	signal (Message (kDestroyed));

	cancelSignals ();

	ASSERT (viewers.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int32 CCL_API Selection::getEditTag () const
{
	return editTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::contentChanged ()
{
	editTag++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::flushChanged ()
{
	if(editTag != lastEditTag)
	{
		lastEditTag = editTag;
		deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::ignoreChanges ()
{
	lastEditTag = editTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::addViewer (ISelectionViewer* viewer)
{
	viewers.append (viewer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::removeViewer (ISelectionViewer* viewer)
{
	viewers.remove (viewer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::hide (bool redraw)
{
	if(showHideSuspended)
		return;

	ListForEach (viewers, ISelectionViewer*, viewer)
		viewer->hideSelection (redraw);
	EndFor

	flushChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::show (bool redraw)
{
	if(showHideSuspended)
		return;

	ListForEach (viewers, ISelectionViewer*, viewer)
		viewer->showSelection (redraw);
	EndFor

	flushChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Selection::makeItemsVisible (bool relaxed)
{
	ListForEach (viewers, ISelectionViewer*, viewer)
		viewer->makeSelectedItemsVisible (relaxed);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* Selection::getFirst () const
{
	AutoPtr<Iterator> iter = newIterator (getType ());
	return iter ? iter->next () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Selection::countTypes () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Selection::canSelectType (MetaClassRef type) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Selection::canSelect (Object* object) const
{
	return object && canSelectType (object->myClass ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Selection::newIterator (int index) const
{
	ASSERT (index == 0)
	return newIterator (getType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API Selection::newIterator (StringID typeName) const
{
	const MetaClass* type = Kernel::instance ().getClassRegistry ().findType (typeName);
	ASSERT (type != nullptr)
	return type ? newIterator (*type) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Selection::isObjectSelected (IUnknown* _object) const
{
	Object* object = unknown_cast<Object> (_object);
	ASSERT (object != nullptr)
	return object && isSelected (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Selection::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "showHideSuspended")
	{
		var = isShowHideSuspended ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Selection::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "showHideSuspended")
	{
		setShowHideSuspended (var);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Selection)
	DEFINE_METHOD_NAME ("isSelected")               // args: Object, return: bool
	DEFINE_METHOD_NAME ("isEmpty")                  // return bool
	DEFINE_METHOD_NAME ("isMultiple")               // return bool
	DEFINE_METHOD_NAME ("newIterator")              // return Object
	//DEFINE_METHOD_NAME ("select")
	//DEFINE_METHOD_NAME ("unselect")
	DEFINE_METHOD_NAME ("unselectAll")
	DEFINE_METHOD_NAME ("unselectType")
	DEFINE_METHOD_NAME ("ignoreChanges")
END_METHOD_NAMES (Selection)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Selection::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "isSelected")
	{
		Object* obj = unknown_cast<Object> (msg[0]);
		returnValue = (obj && isSelected (obj));
	}
	else if(msg == "isEmpty")
	{
		returnValue = isEmpty ();
	}
	else if(msg == "isMultiple")
	{
		returnValue = isMultiple ();
	}
	else if(msg == "newIterator")
	{
		const MetaClass* type = nullptr;
		if(msg.getArgCount () >= 1)
		{
			MutableCString typeString (msg[0].asString ());
			type = Kernel::instance ().getClassRegistry ().findType (typeString);
		}
		else
			type = &getType ();

		if(type)
		{
			AutoPtr<IUnknown> iter (ccl_as_unknown (newIterator (*type)));
			returnValue = Variant (iter, true);
		}
	}
	/*else if(msg == "select")
	{
		hide (false);
		Object* obj = unknown_cast<Object> (msg[0]);
		returnValue = (obj && select (obj));
		show (true);
	}
	else if(msg == "unselect")
	{
		hide (false);
		Object* obj = unknown_cast<Object> (msg[0]);
		returnValue = (obj && unselect (obj));
		show (true);
	}*/
	else if(msg == "unselectAll")
	{
		hide (false);
		returnValue = (unselectAll ());
		show (true);
	}
	else if(msg == "unselectType")
	{
		if(msg.getArgCount () > 0)
		{
			MutableCString className (msg[0].asString ());
			if(const MetaClass* metaClass = Kernel::instance ().getClassRegistry ().findType (className))
			{
				hide (false);
				returnValue = (unselectType (*metaClass));
				show (true);
			}
		}
	}
	else if(msg == "ignoreChanges")
		ignoreChanges ();
	else
		return SuperClass::invokeMethod (returnValue, msg);
	return true;
}

//************************************************************************************************
// SimpleSelection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimpleSelection, Selection)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::isEmpty () const
{
	return list.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::isMultiple () const
{
	return list.isMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::isSelected (Object* object) const
{
	return list.isSelected (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::select (Object* object)
{
	bool result = list.select (object);
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::unselect (Object* object)
{
	bool result = list.unselect (object);
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::unselectAll ()
{
	bool result = list.unselectAll ();
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleSelection::unselectType (MetaClassRef type)
{
	bool result = list.unselectType (type);
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassRef SimpleSelection::getType () const
{
	return list.getType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* SimpleSelection::newIterator (MetaClassRef type) const
{
	return list.newIterator (type);
}

//************************************************************************************************
// MixedSelection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MixedSelection, Selection)

//////////////////////////////////////////////////////////////////////////////////////////////////

MixedSelection::MixedSelection ()
: defaultContainer (nullptr)
{
	containers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MixedSelection::countTypes () const
{
	return containers.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectionContainer* MixedSelection::getTypeAt (int index)
{
	return (SelectionContainer*)containers.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MixedSelection::addType (SelectionContainer* c, bool isDefault)
{
	containers.add (c);
	if(isDefault)
		defaultContainer = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectionContainer* MixedSelection::getContainer (Object* object) const
{
	ASSERT (object != nullptr)
	return object ? getContainer (object->myClass ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectionContainer* MixedSelection::getContainer (MetaClassRef type) const
{
	ArrayForEach (containers, SelectionContainer, c)
		if(type.canCast (c->getType ()))
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::isEmpty () const
{
	ArrayForEach (containers, SelectionContainer, c)
		if(!c->isEmpty ())
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::isMultiple () const
{
	int numSelected = 0;

	ArrayForEach (containers, SelectionContainer, c)
		if(c->isMultiple ())
			return true;
		else if(!c->isEmpty ())
		{
			if(++numSelected > 1)
				return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* MixedSelection::getFirst () const
{
	if(defaultContainer)
		if(Object* first = defaultContainer->getFirst ())
			return first;

	for(auto c : iterate_as<SelectionContainer> (containers))
		if(c != defaultContainer)
			if(Object* first = c->getFirst ())
				return first;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::isSelected (Object* object) const
{
	SelectionContainer* c = getContainer (object);
	SOFT_ASSERT (c != nullptr, "MixedSelection::isSelected: no container for object")
	return c ? c->isSelected (object) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::select (Object* object)
{
	SelectionContainer* c = getContainer (object);
	ASSERT (c != nullptr)
	if(c && c->select (object))
	{
		contentChanged ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::unselect (Object* object)
{
	SelectionContainer* c = getContainer (object);
	ASSERT (c != nullptr)
	if(c && c->unselect (object))
	{
		contentChanged ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::unselectAll ()
{
	bool result = false;
	ArrayForEach (containers, SelectionContainer, c)
		if(c->unselectAll ())
			result = true;
	EndFor
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::unselectType (MetaClassRef type)
{
	bool result = false;
	ArrayForEach (containers, SelectionContainer, c)
		if(c->unselectType (type))
			result = true;
	EndFor
	if(result)
		contentChanged ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassRef MixedSelection::getType () const
{
	ASSERT (defaultContainer != nullptr)
	if(defaultContainer)
		return defaultContainer->getType ();

	SelectionContainer* first = (SelectionContainer*)containers.at (0);
	if(first)
		return first->getType ();

	return ccl_typeid<Object> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MixedSelection::canSelectType (MetaClassRef type) const
{
	return getContainer (type) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* MixedSelection::newIterator (MetaClassRef type) const
{
	SelectionContainer* c = getContainer (type);
	//ASSERT (c != 0)
	return c ? c->newIterator (type) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* MixedSelection::newIterator (int index) const
{
	SelectionContainer* c = (SelectionContainer*)containers.at (index);
	ASSERT (c != nullptr)
	return c ? c->newIterator (c->getType ()) : nullptr;
}
