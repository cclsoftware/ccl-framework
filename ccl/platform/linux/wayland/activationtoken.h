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

#ifndef _cclactivationtoken_h
#define _cclactivationtoken_h

#include "ccl/platform/linux/wayland/waylandclient.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// ActivationListener
//************************************************************************************************

#if WAYLAND_USE_XDG_ACTIVATION
struct ActivationToken: xdg_activation_token_v1_listener
{
	ActivationToken ();
	~ActivationToken ();

	tbool done;
	PROPERTY_VARIABLE (xdg_activation_token_v1*, token, Token)
	PROPERTY_MUTABLE_CSTRING (tokenString, TokenString)

	bool request (void* data = nullptr, xdg_activation_token_v1_listener* listener = nullptr);
	void reset ();

	static void onActivationDone (void* data, xdg_activation_token_v1* token, CStringPtr tokenString);

private:
	void* customData;
	xdg_activation_token_v1_listener* customListener;
};
#endif

} // namespace Linux
} // namespace CCL

#endif // _cclactivationtoken_h
