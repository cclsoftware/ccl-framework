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
// Filename    : ccl/gui/touch/touchcollection.h
// Description : Touch Collection
//
//************************************************************************************************

#ifndef _ccl_touchcollection_h
#define _ccl_touchcollection_h

#include "ccl/public/gui/framework/imultitouch.h"
#include "ccl/public/collections/vector.h"

#include "ccl/base/object.h"

namespace CCL {

//************************************************************************************************
// TouchCollection
//************************************************************************************************

class TouchCollection: public Object,
					   public ITouchCollection,
					   public Vector<TouchInfo>
{
public:
	void copyFrom (const ITouchCollection& other);
	
	// ITouchCollection
	int CCL_API getTouchCount () const override;
	const TouchInfo& CCL_API getTouchInfo (int index) const override;
	const TouchInfo* CCL_API getTouchInfoByID (TouchID id) const override;

	CLASS_INTERFACE (ITouchCollection, Object)
};

} // namespace CCL

#endif // _ccl_touchcollection_h
