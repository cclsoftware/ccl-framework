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
// Filename    : ccl/public/system/ilogger.h
// Description : Logger Interface
//
//************************************************************************************************

#ifndef _ccl_ilogger_h
#define _ccl_ilogger_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {
namespace System {

//************************************************************************************************
// System::ILogger
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILogger: Alert::IReporter
{
	virtual void CCL_API addOutput (Alert::IReporter* output) = 0;
	
	virtual void CCL_API removeOutput (Alert::IReporter* output) = 0;

	DECLARE_IID (ILogger)
};

DEFINE_IID (ILogger, 0x5166d278, 0x4af9, 0x4543, 0x9a, 0xf5, 0xe2, 0xb6, 0x67, 0x3c, 0x3c, 0x4c)

} // namespace System
} // namespace CCL

#endif // _ccl_ilogger_h
