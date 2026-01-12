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
// Filename    : ccl/app/utilities/paramaccessor.cpp
// Description : Parameter Accessor
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/utilities/paramaccessor.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/icontroller.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ParamAccessor
//************************************************************************************************

ParamAccessor::ParamAccessor (IUnknown* anchor, StringID paramPath)
: parameter (nullptr)
{
	resolve (anchor, paramPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamAccessor::ParamAccessor (StringID paramPath)
: parameter (nullptr)
{
	resolve (nullptr, paramPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamAccessor::resolve (IUnknown* anchor, StringID paramPath)
{
	if(!paramPath.isEmpty ())
	{
		UnknownPtr<IController> controller;

		// try to interpret the name as "controllerPath/paramName"
		int pos = paramPath.lastIndex ('/');
		if(pos >= 0)
		{
			MutableCString controllerPath (paramPath.subString (0, pos));
			MutableCString pName (paramPath.subString (pos + 1));
			controller = lookupController (anchor, controllerPath);
			if(controller)
				parameter = controller->findParameter (pName);
		}
		else
		{
			// interpret full paramPath as param name
			controller = anchor;
			if(controller)
				parameter = controller->findParameter (paramPath);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ParamAccessor::lookupController (IUnknown* anchor, StringID path)
{
	IUnknown* controller = nullptr;

	if(path.contains ("://")) // lookup from root
	{
		String _path (path);
		Url objectUrl (_path);
		controller = System::GetObjectTable ().getObjectByUrl (objectUrl);
	}
	else
	{
		// lookup relative to anchor controller
		UnknownPtr<IObjectNode> iNode (anchor);
		controller = iNode ? iNode->lookupChild (String (path)) : nullptr;
	}
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamAccessor::get (Variant& value) const
{
	if(!parameter)
		return false;

	value = parameter->getValue ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamAccessor::set (VariantRef value, tbool update)
{
	if(!parameter)
		return false;

	parameter->setValue (value, update);
	return true;
}
