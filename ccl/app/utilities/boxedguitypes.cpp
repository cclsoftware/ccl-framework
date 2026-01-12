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
// Filename    : ccl/app/utilities/boxedguitypes.cpp
// Description : GUI "boxed" types
//
//************************************************************************************************

#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/base/storage/storage.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Method definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (Boxed::Point, Object, "Point")
DEFINE_CLASS_PERSISTENT (Boxed::Rect, Object, "Rect")

namespace CCL
{
	namespace Boxed
	{
		BEGIN_METHOD_NAMES (Point)
			DEFINE_METHOD_NAME ("equals")
		END_METHOD_NAMES (Point)

		BEGIN_METHOD_NAMES (Rect)
			DEFINE_METHOD_NAME ("equals")
			DEFINE_METHOD_NAME ("pointInside")
			DEFINE_METHOD_NAME ("clone")
		END_METHOD_NAMES (Rect)
	}
}

//************************************************************************************************
// Boxed::Point
//************************************************************************************************

Boxed::Point::Point ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::Point::Point (PointRef point)
: CCL::Point (point)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Point::equals (const Object& obj) const
{
	const Boxed::Point* point = ccl_cast<Boxed::Point> (&obj);
	if(point)
		return *this == *point;
	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Point::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	x = a.getInt ("x");
	y = a.getInt ("y");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Point::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("x", x);
	a.set ("y", y);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Point::getProperty (CCL::Variant& var, MemberID propertyId) const
{
	if(propertyId == "x")
		var = x;
	else if(propertyId == "y")
		var = y;
	else
		return SuperClass::getProperty (var, propertyId);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Point::setProperty (MemberID propertyId, const CCL::Variant& var)
{
	if(propertyId == "x")
		x = var.asInt ();
	else if(propertyId == "y")
		y = var.asInt ();
	else
		return SuperClass::setProperty (propertyId, var);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Point::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "equals")
	{
		const CCL::Variant& arg = msg.getArg (0);
		if(arg.isObject ())
		{
			Object* obj = unknown_cast<Object> (arg);
			returnValue = (obj != nullptr && equals (*obj));
			return true;
		}
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Boxed::Rect
//************************************************************************************************

Boxed::Rect::Rect ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::Rect::Rect (RectRef rect)
: CCL::Rect (rect)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Rect::equals (const Object& obj) const
{
	const Boxed::Rect* rect = ccl_cast<Boxed::Rect> (&obj);
	if(rect)
		return *this == *rect;
	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Rect::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	left = a.getInt ("left");
	top = a.getInt ("top");
	right = a.getInt ("right");
	bottom = a.getInt ("bottom");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Rect::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("left", left);
	a.set ("top", top);
	a.set ("right", right);
	a.set ("bottom", bottom);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Rect::getProperty (CCL::Variant& var, MemberID propertyId) const
{
	if(propertyId == "left")
		var = left;
	else if(propertyId == "top")
		var = top;
	else if(propertyId == "right")
		var = right;
	else if(propertyId == "bottom")
		var = bottom;
	else
		return SuperClass::getProperty (var, propertyId);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Rect::setProperty (MemberID propertyId, const CCL::Variant& var)
{
	if(propertyId == "left")
		left = var.asInt ();
	else if(propertyId == "top")
		top = var.asInt ();
	else if(propertyId == "right")
		right = var.asInt ();
	else if(propertyId == "bottom")
		bottom = var.asInt ();
	else
		return SuperClass::setProperty (propertyId, var);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Rect::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "equals")
	{
		Object* obj = unknown_cast<Object> (msg[0]);
		returnValue = (obj != nullptr && equals (*obj));
		return true;
	}
	else if(msg == "pointInside")
	{
		Boxed::Point* point = unknown_cast<Boxed::Point> (msg[0]);
		returnValue = (point != nullptr && pointInside (*point));
		return true;
	}
	else if(msg == "clone")
	{
		AutoPtr<Boxed::Rect> rect (NEW Boxed::Rect (*this));
		returnValue.takeShared (ccl_as_unknown (rect));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Boxed::MouseEvent
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::MouseEvent, Object, "MouseEvent")

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MouseEvent::MouseEvent ()
: mouseLoc (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MouseEvent::~MouseEvent ()
{
	if(mouseLoc)
		mouseLoc->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MouseEvent::MouseEvent (const CCL::MouseEvent& mouseEvent)
: mouseLoc (nullptr)
{
	assign (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MouseEvent& Boxed::MouseEvent::assign (const CCL::MouseEvent& mouseEvent)
{
	this->eventClass = mouseEvent.eventClass;
	this->eventType = mouseEvent.eventType;
	this->where = mouseEvent.where;
	this->keys = mouseEvent.keys;
	this->inputDevice  = mouseEvent.inputDevice;
	this->penInfo = mouseEvent.penInfo;
	this->doubleClicked = mouseEvent.doubleClicked;
	this->dragged = mouseEvent.dragged;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::MouseEvent::getProperty (CCL::Variant& var, MemberID propertyId) const
{
	if(propertyId == "mouseLoc")
	{
		if(mouseLoc == nullptr)
			mouseLoc = NEW Boxed::Point (where);
		else
			mouseLoc->x = where.x, mouseLoc->y = where.y;

		var.takeShared (ccl_as_unknown (mouseLoc));

		//AutoPtr<Boxed::Point> point = NEW Boxed::Point (where);
		//var.takeShared (*point);
	}
	else if(propertyId == "modifier")
		var = this->keys.getModifiers ();
	else
		return SuperClass::getProperty (var, propertyId);
	return true;
}
