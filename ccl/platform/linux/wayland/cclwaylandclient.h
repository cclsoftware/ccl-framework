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
// Filename    : ccl/platform/linux/wayland/cclwaylandclient.h
// Description : Wayland Client Includes
//
//************************************************************************************************

#ifndef _cclwaylandclient_h
#define _cclwaylandclient_h

#include "ccl/public/base/ccldefpush.h"

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-dialog-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v2-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "linux-dmabuf-v1-client-protocol.h"
#ifndef ZWP_LINUX_DMABUF_V1_DESTROY_SINCE_VERSION
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#endif

#ifdef XDG_ACTIVATION_V1_DESTROY_SINCE_VERSION
#define WAYLAND_USE_XDG_ACTIVATION 1
#else
#define WAYLAND_USE_XDG_ACTIVATION 0
struct xdg_activation_v1;
#endif

#ifdef XDG_DIALOG_V1_DESTROY_SINCE_VERSION
#define WAYLAND_USE_XDG_DIALOG 1
#else
#define WAYLAND_USE_XDG_DIALOG 0
struct xdg_wm_dialog_v1;
#endif

#ifdef WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
#define WAYLAND_COMPOSITOR_VERSION WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
#elif defined (WL_SURFACE_OFFSET_SINCE_VERSION)
#define WAYLAND_COMPOSITOR_VERSION WL_SURFACE_OFFSET_SINCE_VERSION
#else
#define WAYLAND_COMPOSITOR_VERSION WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION
#endif

#ifdef WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION
#define WAYLAND_SEAT_VERSION WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION
#elif defined (WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION)
#define WAYLAND_SEAT_VERSION WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
#elif defined (WL_POINTER_AXIS_VALUE120_SINCE_VERSION)
#define WAYLAND_SEAT_VERSION WL_POINTER_AXIS_VALUE120_SINCE_VERSION
#else
#define WAYLAND_SEAT_VERSION 7
#endif

#include "ccl/public/base/ccldefpop.h"

#endif // _cclwaylandclient_h
