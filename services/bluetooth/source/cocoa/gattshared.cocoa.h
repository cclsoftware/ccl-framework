//************************************************************************************************
//
// Bluetooth Support
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
// Filename    : gattshared.cocoa.h
// Description : Bluetooth LE Gatt (Core Bluetooth)
//
//************************************************************************************************

#ifndef _gattshared_cocoa_h
#define _gattshared_cocoa_h

#include "core/public/coreuid.h"

@class CBUUID;

//////////////////////////////////////////////////////////////////////////////////////////////////

Core::UIDBytes fromCBUUID (CBUUID* cbuuid);

//////////////////////////////////////////////////////////////////////////////////////////////////

void toCBUUID (CBUUID*& cbuuid, Core::UIDRef uid);

#endif // _gattshared_cocoa_h
