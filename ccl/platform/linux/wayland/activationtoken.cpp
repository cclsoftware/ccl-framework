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
// Filename    : ccl/platform/linux/wayland/activationtoken.h
// Description : Wayland Activation Token
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/activationtoken.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/gui/windows/desktop.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// ActivationToken
//************************************************************************************************

#if WAYLAND_USE_XDG_ACTIVATION

ActivationToken::ActivationToken ()
: done (false),
  token (nullptr),
  customListener (nullptr),
  customData (nullptr)
{
	xdg_activation_token_v1_listener::done = onActivationDone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActivationToken::~ActivationToken ()
{
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivationToken::onActivationDone (void* data, xdg_activation_token_v1* token, CStringPtr tokenString)
{
	ActivationToken* This = static_cast<ActivationToken*> (data);
	This->tokenString = tokenString;
	This->done = true;
	if(This->customListener)
		This->customListener->done (This->customData, token, tokenString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActivationToken::request (void* data, xdg_activation_token_v1_listener* listener)
{
	WaylandClient& waylandClient = WaylandClient::instance ();

	xdg_activation_v1* activation = waylandClient.getActivation ();
	if(activation == nullptr)
		return false;

	reset ();
	token = xdg_activation_v1_get_activation_token (activation);
	if(token == nullptr)
		return false;

	xdg_activation_token_v1_set_app_id (token, waylandClient.getApplicationId ());
	xdg_activation_token_v1_set_serial (token, waylandClient.getSerial (), waylandClient.getSeat ());
	Window* window = Desktop.getActiveWindow ();
	LinuxWindow* activeWindow = LinuxWindow::cast (Desktop.getActiveWindow ());

	wl_surface* activeSurface = activeWindow ? activeWindow->getWaylandSurface () : nullptr;

	while(activeSurface == nullptr && activeWindow != nullptr)
	{
		activeWindow = LinuxWindow::cast (ccl_cast<Window> (activeWindow->getParentWindow ()));
		if(activeWindow)
			activeSurface = activeWindow->getWaylandSurface ();
	}
	if(activeSurface == nullptr)
		return false;
	
	customData = data;
	customListener = listener;

	xdg_activation_token_v1_set_surface (token, activeSurface);
	xdg_activation_token_v1_add_listener (token, this, this);
	xdg_activation_token_v1_commit (token);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivationToken::reset ()
{
	if(token)
		xdg_activation_token_v1_destroy (token);
	token = nullptr;
	customListener = nullptr;
}

#endif // WAYLAND_USE_XDG_ACTIVATION
