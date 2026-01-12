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
// Filename    : gattshared.android.cpp
// Description : Bluetooth LE Gatt Central/Peripheral Shared
//
//************************************************************************************************

#include "gattshared.android.h"

#include "ccl/public/base/primitives.h"

#include "ccl/public/base/buffer.h"

using namespace Core;
using namespace Core::Java;

namespace CCL {
namespace Bluetooth {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

// GATT status constants from android.bluetooth.BluetoothGatt
// https://developer.android.com/reference/android/bluetooth/BluetoothGatt

constexpr int GATT_SUCCESS = 0;
constexpr int GATT_READ_NOT_PERMITTED = 2;
constexpr int GATT_WRITE_NOT_PERMITTED = 3;
constexpr int GATT_INSUFFICIENT_AUTHENTICATION = 5;
constexpr int GATT_REQUEST_NOT_SUPPORTED = 6;
constexpr int GATT_INVALID_OFFSET = 7;
constexpr int GATT_INSUFFICIENT_AUTHORIZATION = 8;
constexpr int GATT_INVALID_ATTRIBUTE_LENGTH = 13;
constexpr int GATT_INSUFFICIENT_ENCRYPTION = 15;
constexpr int GATT_CONNECTION_CONGESTED = 143;
constexpr int GATT_FAILURE = 257;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Java type conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes uidFromJavaUuid (jobject javaUuid)
{
	JniAccessor jni;
	LocalStringRef javaUuidString (jni, Java::UUID.toString (javaUuid));

	UID uid;
	uid.fromString (String ("{").append (fromJavaString (javaUuidString)).append ("}"));
	return uid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBuffer* createBufferFromJavaArray (const JniByteArray& javaArray)
{
	Buffer* buffer = NEW Buffer (javaArray.getLength ());
	javaArray.getData (buffer->getAddress (), buffer->getSize ());

	return static_cast<IBuffer*> (buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Error conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode toErrorCode (int status)
{
	switch(status)
	{
	case GATT_SUCCESS:
		return Core::Errors::kError_NoError;
	case GATT_CONNECTION_CONGESTED:
		return Core::Bluetooth::kError_BluetoothBusy;
	default:
		return Core::Errors::kError_Failed;
	}
}

} // namespace Bluetooth
} // namespace CCL
