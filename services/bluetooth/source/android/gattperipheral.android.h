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
// Filename    : gattperipheral.android.h
// Description : Android Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#ifndef _gattperipheral_android_h
#define _gattperipheral_android_h

#include "core/public/devices/coregattperipheral.h"
#include "core/public/coreobserver.h"

#include "ccl/base/object.h"

#include "ccl/public/plugins/icoreplugin.h"

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

//************************************************************************************************
// AndroidGattPeripheral
//************************************************************************************************

class AndroidGattPeripheral: public CorePropertyHandler<IGattPeripheral, Object, IObject>
{
public:
	// IGattPeripheral
	void startup () override;
	Core::ErrorCode createServiceAsync (Core::UIDRef uuid) override;
	void shutdown () override;
	IGattPeripheralService* getService (int index) const override;
	int getNumServices () const override;

	DEFINE_OBSERVER_OVERRIDE (IGattPeripheralObserver);
};

} // namespace Bluetooth
} // namespace Core

#endif // _gattperipheral_android_h
