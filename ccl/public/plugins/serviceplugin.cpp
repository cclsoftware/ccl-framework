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
// Filename    : ccl/public/plugins/serviceplugin.cpp
// Description : Service Plugin
//
//************************************************************************************************

#include "ccl/public/plugins/serviceplugin.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "ccl/public/base/iextensible.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ServicePlugin
//************************************************************************************************

ServicePlugin::ServicePlugin ()
: classFactory (nullptr),
  context (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServicePlugin::~ServicePlugin ()
{
	if(classFactory)
		classFactory->release ();

	ASSERT (context == nullptr)
	if(context)
		context->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory& ServicePlugin::getClassFactory ()
{
	if(!classFactory)
		classFactory = NEW ClassFactory;
	return *classFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServicePlugin::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IPluginInstance)
	QUERY_INTERFACE (IComponent)

	if(classFactory)
		return classFactory->queryInterface (iid, ptr);

	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServicePlugin::initialize (IUnknown* _context)
{
	ASSERT (context == nullptr)
	take_shared<IUnknown> (context, _context);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServicePlugin::terminate ()
{
	if(classFactory)
		classFactory->release ();
	classFactory = nullptr;

	take_shared<IUnknown> (context, nullptr);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServicePlugin::canTerminate () const
{
	return true;
}

//************************************************************************************************
// CoreServicePlugin
//************************************************************************************************

CoreServicePlugin::CoreServicePlugin ()
: coreClassFactory (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreServicePlugin::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IClassFactory> () && coreClassFactory)
		return coreClassFactory->queryInterface (iid, ptr);

	return ServicePlugin::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreServicePlugin::initFactory (const Core::Plugins::ClassInfoBundle* classBundle)
{
	ICoreCodeLoader* loader = IExtensible::getExtensionI<ICoreCodeLoader> (&System::GetPlugInManager ());
	ASSERT (classBundle != nullptr && loader != nullptr)
	if(classBundle == nullptr || loader == nullptr)
		return false;

	coreClassFactory = loader->createClassFactory (*classBundle);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreServicePlugin::terminate ()
{
	safe_release (coreClassFactory);

	return ServicePlugin::terminate ();
}
