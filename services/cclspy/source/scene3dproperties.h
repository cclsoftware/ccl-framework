//************************************************************************************************
//
// CCL Spy
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
// Filename    : scene3dproperties.h
// Description : 3D Scene Properties
//
//************************************************************************************************

#ifndef _scene3dproperties_h
#define _scene3dproperties_h

#include "objectinfo.h"

#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/gui/graphics/3d/iscene3d.h"
#include "ccl/public/gui/graphics/3d/imodel3d.h"

using namespace CCL;

namespace Spy {

//************************************************************************************************
// Position3DPropertyHandler - currently not used
//************************************************************************************************

class Position3DPropertyHandler: public PropertyHandler
{
public:
	void toString (String& string, VariantRef value) override
	{
		PointF3D p;
		if(auto uiValue = IUIValue::toValue (value))
			uiValue->toPointF3D (p);

		string = "(";
		string.appendFloatValue (p.x, 3);
		string << ", ";
		string.appendFloatValue (p.y, 3);
		string << ", ";
		string.appendFloatValue (p.z, 3);
		string << ")";
	}

	int getEditCapability (VariantRef value) override { return kStringEdit; }
};

//************************************************************************************************
// Material3DPropertyHandler
//************************************************************************************************

struct Material3DPropertyHandler: public PropertyHandler
{
	// PropertyHandler
	void toString (String& string, VariantRef value) override { UnknownPtr<IObject> object (value.asUnknown ()); if(object) string = object->getTypeInfo ().getClassName (); }
	int getEditCapability (VariantRef value) override { return value.asUnknown () ? kObjectLink : kNoEdit; }
	bool edit (VariantRef value, EditContext& context) override { context.objectToInspect.share (value.asUnknown ()); return true; }
};

// to be continued...

} // namespace Spy

#endif // _scene3dproperties_h
