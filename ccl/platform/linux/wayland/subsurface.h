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
// Filename    : ccl/platform/linux/wayland/subsurface.h
// Description : Wayland Subsurface
//
//************************************************************************************************

#ifndef _ccl_linux_subsurface_h
#define _ccl_linux_subsurface_h

#include "ccl/platform/linux/wayland/surface.h"

#include "ccl/public/gui/graphics/point.h"

namespace CCL {
struct KeyEvent;
struct FocusEvent;

namespace Linux {

//************************************************************************************************
// SubSurface
//************************************************************************************************

template<class Base = Surface>
class SubSurface: public Base
{
public:
	SubSurface (Surface& parent);
	
	PROPERTY_POINTER (wl_subsurface, subSurface, SubSurface)
	
	void setPosition (PointRef position);
	PointRef getPosition () const { return position; }
	void setSynchronous (bool state);
	void placeBelow (const Surface& surface);
	void placeAbove (const Surface& surface);
	
	// Surface
	void createSurface () override;
	void destroySurface () override;
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;
	void enableInput (bool state = true) override;
	bool suppressInput () override;
	void handleKeyboardEvent (const KeyEvent& event) override;
	void handleFocus (const FocusEvent& event) override;
	void handlePointerEvent (const InputHandler::PointerEvent& event) override;
	
protected:
	Surface& parent;
	Point position;
};

//************************************************************************************************
// SubSurface implementation
//************************************************************************************************

template<class Base>
SubSurface<Base>::SubSurface (Surface& parent)
: parent (parent),
  subSurface (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::enableInput (bool state)
{
	if(Surface::getWaylandSurface () != nullptr)
	{
		if(state)
			wl_surface_set_input_region (Surface::getWaylandSurface (), nullptr);
		else
			Surface::clearInputRegion ();
	}
	Surface::enableInput (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
bool SubSurface<Base>::suppressInput ()
{
	return parent.suppressInput ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::handleKeyboardEvent (const KeyEvent& event)
{
	parent.handleKeyboardEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::handleFocus (const FocusEvent& event)
{
	parent.handleFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::handlePointerEvent (const InputHandler::PointerEvent& pointerEvent)
{
	if(pointerEvent.focus == Surface::getWaylandSurface () 
		|| pointerEvent.oldSurface == Surface::getWaylandSurface ()
		|| (pointerEvent.focus == nullptr && pointerEvent.oldSurface == nullptr))
	{
		InputHandler::PointerEvent event (pointerEvent);
		event.x += wl_fixed_from_int (position.x);
		event.y += wl_fixed_from_int (position.y);
		event.eventMask &= ~InputHandler::kPointerEnter;
		event.eventMask &= ~InputHandler::kPointerLeave;
		if(event.focus == Surface::getWaylandSurface ())
			event.focus = parent.getWaylandSurface ();
		if(event.oldSurface == Surface::getWaylandSurface ())
			event.oldSurface = parent.getWaylandSurface ();
		parent.handlePointerEvent (event);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::createSurface ()
{
	wl_subcompositor* subCompositor = WaylandClient::instance ().getSubCompositor ();
	if(subCompositor == nullptr)
		return;
	
	ASSERT (parent.getWaylandSurface () != nullptr)
	if(parent.getWaylandSurface () != nullptr)
	{
		Surface::createSurface ();
		ASSERT (Surface::getWaylandSurface () != nullptr)
		if(Surface::getWaylandSurface () != nullptr)
		{
			setSubSurface (wl_subcompositor_get_subsurface (subCompositor, Surface::getWaylandSurface (), parent.getWaylandSurface ()));
			setPosition (position);
			enableInput (false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::destroySurface ()
{
	if(getSubSurface () != nullptr && WaylandClient::instance ().isInitialized ())
		wl_subsurface_destroy (getSubSurface ());
	setSubSurface (nullptr);
	Surface::destroySurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::onCompositorDisconnected ()
{
	Base::onCompositorDisconnected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::onCompositorConnected ()
{
	Base::onCompositorConnected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::setPosition (PointRef _position)
{
	position = _position;
	if(getSubSurface () != nullptr)
		wl_subsurface_set_position (getSubSurface (), position.x, position.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::setSynchronous (bool state)
{
	if(getSubSurface () != nullptr)
	{
		if(state)
			wl_subsurface_set_sync (getSubSurface ());
		else
			wl_subsurface_set_desync (getSubSurface ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::placeBelow (const Surface& surface)
{
	if(getSubSurface () != nullptr && surface.getWaylandSurface () != nullptr)
		wl_subsurface_place_below (getSubSurface (), surface.getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
void SubSurface<Base>::placeAbove (const Surface& surface)
{
	if(getSubSurface () != nullptr && surface.getWaylandSurface () != nullptr)
		wl_subsurface_place_above (getSubSurface (), surface.getWaylandSurface ());
}

} // namespace Linux
} // namespace CCL
	
#endif // _ccl_linux_subsurface_h
