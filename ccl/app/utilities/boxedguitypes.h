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
// Filename    : ccl/app/utilities/boxedguitypes.h
// Description : GUI "boxed" types
//
//************************************************************************************************

#ifndef _ccl_boxedguitypes_h
#define _ccl_boxedguitypes_h

#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {
namespace Boxed {

//************************************************************************************************
// Boxed::Rect
//************************************************************************************************

class Rect: public Object,
		    public CCL::Rect
{
public:
	DECLARE_CLASS (Rect, Object)
	DECLARE_METHOD_NAMES (Rect)

	Rect ();
	Rect (CCL::RectRef rect);

	typedef ValueHelper<Boxed::Rect, CCL::Rect> Value;

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage&) override;
	bool save (const Storage&) const override;

protected:
	// IObject
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
	tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
};

//************************************************************************************************
// Boxed::Point
//************************************************************************************************

class Point: public Object,
		     public CCL::Point
{
public:
	DECLARE_CLASS (Point, Object)
	DECLARE_METHOD_NAMES (Point)

	Point ();
	Point (CCL::PointRef rect);

	typedef ValueHelper<Boxed::Point, CCL::Point> Value;

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage&) override;
	bool save (const Storage&) const override;

protected:
	// IObject
	tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Boxed::MouseEvent
//************************************************************************************************

class MouseEvent: public Object,
	              public CCL::MouseEvent
{
public:
	DECLARE_CLASS (MouseEvent, Object)

	MouseEvent ();
	MouseEvent (const CCL::MouseEvent& mouseEvent);
	~MouseEvent ();

	typedef ValueHelper<Boxed::MouseEvent, CCL::MouseEvent> Value;

	MouseEvent& operator = (const CCL::MouseEvent& me) { return assign (me); }

protected:
	mutable Boxed::Point* mouseLoc;

	MouseEvent& assign (const CCL::MouseEvent& mouseEvent);

	// IObject
	tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
};

} // namespace Boxed
} // namespace CCL

#endif // _ccl_boxedtypes_h
