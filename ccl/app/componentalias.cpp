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
// Filename    : ccl/app/componentalias.cpp
// Description : Component Alias
//
//************************************************************************************************

#include "ccl/app/componentalias.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ComponentAlias
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ComponentAlias, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ComponentAlias::ComponentAlias ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComponentAlias::~ComponentAlias ()
{
	detachAlias ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ComponentAlias::initialize (IUnknown* context)
{
	tresult result = kResultOk;

	// initialize plug-in object
	{
		ScopedVar<IUnknown*> scope (this->context, context);
		UnknownPtr<IComponent> iComponent (unknownPtr);
		if(iComponent)
			result = iComponent->initialize (this->asUnknown ());
	}

	// initialize native components
	if(result == kResultOk)
		result = SuperClass::initialize (context);
	else
		SuperClass::terminate (); // something failed

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ComponentAlias::terminate ()
{
	// terminate native sub-objects
	SharedPtr<IUnknown> contextKeeper = this->context;
	tresult result = SuperClass::terminate ();

	// terminate plug-in object
	ScopedVar<IUnknown*> scope (this->context, contextKeeper);
	UnknownPtr<IComponent> iComponent (unknownPtr);
	if(iComponent)
		result = iComponent->terminate ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ComponentAlias::canTerminate () const
{
	// ask plug-in object
	UnknownPtr<IComponent> iComponent (unknownPtr);
	if(iComponent && !iComponent->canTerminate ())
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ComponentAlias::getExtension (StringID id)
{
	UnknownPtr<IExtensible> iExtensible (unknownPtr);
	return iExtensible ? iExtensible->getExtension (id) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ComponentAlias::findParameter (StringID name) const
{
	UnknownPtr<IController> iController (unknownPtr);
	return iController ? iController->findParameter (name) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ComponentAlias::countParameters () const
{
	UnknownPtr<IController> iController (unknownPtr);
	return iController ? iController->countParameters () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ComponentAlias::getParameterAt (int index) const
{
	UnknownPtr<IController> iController (unknownPtr);
	return iController ? iController->getParameterAt (index) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ComponentAlias::getParameterByTag (int tag) const
{
	UnknownPtr<IController> iController (unknownPtr);
	return iController ? iController->getParameterByTag (tag) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ComponentAlias::createView (StringID name, VariantRef data, const Rect& bounds)
{
	UnknownPtr<IViewFactory> iViewFactory (unknownPtr);
	return iViewFactory ? iViewFactory->createView (name, data, bounds) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ComponentAlias::checkCommandCategory (CStringRef category) const
{
	UnknownPtr<ICommandHandler> iCommandHandler (unknownPtr);
	if(iCommandHandler)
		return iCommandHandler->checkCommandCategory (category);
	return Component::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ComponentAlias::interpretCommand (const CommandMsg& msg)
{
	UnknownPtr<ICommandHandler> iCommandHandler (unknownPtr);
	if(iCommandHandler)
		return iCommandHandler->interpretCommand (msg);
	return Component::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ComponentAlias::appendContextMenu (IContextMenu& contextMenu)
{
	UnknownPtr<IContextMenuHandler> iContextMenuHandler (unknownPtr);
	if(iContextMenuHandler)
		return iContextMenuHandler->appendContextMenu (contextMenu);
	return Component::appendContextMenu (contextMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentAlias::assignAlias (IUnknown* object)
{
	unknownPtr = object;

	UnknownPtr<IObjectNode> iNode (object);
	if(iNode)
	{
		setName (iNode->getObjectID ());
		setObjectUID (iNode->getObjectUID ());
	}
	else
	{
		const IClassDescription* classDesc = getClassDescription ();
		if(classDesc)
			setName (classDesc->getName ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentAlias::detachAlias ()
{
	// In case we wrap a plug-in created via ccl_new, give plug-in manager a chance to cleanup
	IUnknown* unk = unknownPtr.detach ();
	if(unk)
		ccl_release (unk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentAlias::verifyAlias () const
{
	return unknownPtr.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ComponentAlias::getPlugInUnknown () const
{
	return unknownPtr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ComponentAlias::getHostContext ()
{
	return getContext (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* ComponentAlias::getClassDescription () const
{
	return ccl_classof (unknownPtr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ComponentAlias::getClassUID () const
{
	const IClassDescription* classDesc = getClassDescription ();
	if(classDesc)
		return classDesc->getClassID ();
	return Component::getClassUID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ComponentAlias::findChild (StringRef id) const
{
	if(id == "plugInUnknown")
	{
		UnknownPtr<IObjectNode> iNode (unknownPtr);
		return iNode;
	}
	else
		return Component::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ComponentAlias::getObject (StringID name, UIDRef classID)
{
	UnknownPtr<IController> iController (unknownPtr);
	if(iController)
		return iController->getObject (name, classID);
	
	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ComponentAlias::getProperty (Variant& var, MemberID propertyId) const
{
	UnknownPtr<IObject> iObject (unknownPtr);
	if(iObject && iObject->getProperty (var, propertyId))
		return true;

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentAlias::load (const Storage& storage)
{
	UnknownPtr<IPersistAttributes> p (unknownPtr);
	if(p)
		return p->restoreValues (storage.getAttributes ()) == kResultOk;
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentAlias::save (const Storage& storage) const
{
	UnknownPtr<IPersistAttributes> p (unknownPtr);
	if(p)
		return p->storeValues (storage.getAttributes ()) == kResultOk;
	else
		return true;
}
