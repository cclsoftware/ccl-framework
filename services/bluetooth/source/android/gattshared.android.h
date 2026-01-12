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
// Filename    : gattshared.android.h
// Description : Bluetooth LE Gatt Central/Peripheral Shared
//
//************************************************************************************************

#ifndef _gattshared_android_h
#define _gattshared_android_h

#include "ccl/public/base/uid.h"

#include "ccl/platform/android/cclandroidjni.h"

#include "core/public/devices/coregattshared.h"

namespace CCL {
interface IBuffer;

namespace Bluetooth {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Java type conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes uidFromJavaUuid (jobject javaUuid);
IBuffer* createBufferFromJavaArray (const Core::Java::JniByteArray& javaArray);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Error Conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode toErrorCode (int status);

} // namespace Bluetooth
} // namespace CCL

#endif // _gattshared_android_h
