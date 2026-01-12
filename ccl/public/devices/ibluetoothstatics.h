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
// Filename    : ccl/public/devices/ibluetoothstatics.h
// Description : Bluetooth Statics Interface
//
//************************************************************************************************

#ifndef _ccl_ibluetoothstatics_h
#define _ccl_ibluetoothstatics_h

#include "ccl/public/base/iunknown.h"

#include "core/public/devices/coregattcentral.h"
#include "core/public/devices/coregattperipheral.h"

namespace CCL {

// import Core Library definitions
namespace Bluetooth {
using namespace Core::Bluetooth; }

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (BluetoothStatics, 0xa7b62c37, 0xe5d0, 0x4411, 0x8e, 0x9d, 0x1c, 0x50, 0xd, 0x73, 0x48, 0xdb)
}

//************************************************************************************************
// IBluetoothStatics
//************************************************************************************************

interface IBluetoothStatics: IUnknown
{
	/** Get Bluetooth LE GATT Central factory. */
	virtual Bluetooth::IGattCentralFactory& CCL_API getGattCentralFactory () = 0;

	/** Get Bluetooth LE GATT Peripheral factory. */
	virtual Bluetooth::IGattPeripheralFactory& CCL_API getGattPeripheralFactory () = 0;

	DECLARE_IID (IBluetoothStatics)
};

DEFINE_IID (IBluetoothStatics, 0x3fae692a, 0xe9d, 0x4f2c, 0xa8, 0xfc, 0x34, 0xf, 0xc3, 0x6f, 0xc8, 0xf0)

} // namespace CCL

#endif // _ccl_ibluetoothstatics_h
