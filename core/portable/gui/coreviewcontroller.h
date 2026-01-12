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
// Filename    : core/portable/gui/coreviewcontroller.h
// Description : View Controller Interface
//
//************************************************************************************************

#ifndef _coreviewcontroller_h
#define _coreviewcontroller_h

#include "core/public/corestringbuffer.h"
#include "core/portable/coretypeinfo.h"
#include "core/portable/corecontrollershared.h"

namespace Core {
namespace Portable {

class View;

//************************************************************************************************
// ViewController
/** Application-specific callback interface for building views.
	\ingroup core_gui */
//************************************************************************************************

class ViewController: public ITypedObject
{
public:
	DECLARE_CORE_CLASS_ ('VCtl', ViewController)

	virtual ~ViewController () {}

	virtual View* createView (CStringPtr type) = 0;

	virtual void* getObjectForView (CStringPtr name, CStringPtr type) = 0;
};

static const CStringPtr kParamType = "Param";
static const CStringPtr kControllerType = "Controller";

//************************************************************************************************
// ComponentViewController
/** Mix-in template to add ViewController interface to Component class.
	\ingroup core_gui */
//************************************************************************************************

template <class T>
class ComponentViewController: public ViewController
{
public:
	View* createView (CStringPtr type)
	{
		ParamPath64 path (type);
		if(!path.childName.isEmpty ())
		{
			if(ViewController* controller = getSubController (path.childName))
				return controller->createView (path.paramName);
		}
		return 0;
	}

	void* getObjectForView (CStringPtr name, CStringPtr _type)
	{
		ConstString type (_type);
		if(type == kParamType)
			return static_cast<T*> (this)->lookupParameter (name);
		else if(type == kControllerType)
			return getSubController (name);
		else
		{
			ParamPath64 path (name);
			if(!path.childName.isEmpty ())
			{
				if(ViewController* controller = getSubController (path.childName))
					return controller->getObjectForView (path.paramName, type);
			}
		}
		return 0;
	}

protected:
	ViewController* getSubController (CStringPtr path) const
	{
		return core_cast<ViewController> (static_cast<const T*> (this)->lookupChild (path));
	}
};

} // namespace Portable
} // namespace Core

#endif // _coreviewcontroller_h
