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
// Filename    : ccl/platform/linux/wayland/waylandobject.h
// Description : Wayland Object Base Class
//
//************************************************************************************************

#ifndef _ccl_linux_waylandobject_h
#define _ccl_linux_waylandobject_h

namespace CCL {
namespace Linux {

//************************************************************************************************
// WaylandObject
//************************************************************************************************

class WaylandObject
{
public:
	virtual void onCompositorDisconnected () {}
	virtual void onCompositorConnected () {}
};
    
} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_waylandobject_h
