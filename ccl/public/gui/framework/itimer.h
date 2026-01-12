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
// Filename    : ccl/public/gui/framework/itimer.h
// Description : Timer Interface
//
//************************************************************************************************

#ifndef _ccl_itimer_h
#define _ccl_itimer_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface ITimer;

//************************************************************************************************
// ITimerTask
/** 
	\ingroup gui */
//************************************************************************************************

interface ITimerTask: IUnknown
{
	virtual void CCL_API onTimer (ITimer* timer) = 0;

	DECLARE_IID (ITimerTask)
};

DEFINE_IID (ITimerTask, 0x6e55ff36, 0xfd5, 0x43a9, 0x9d, 0xa, 0xbc, 0xc3, 0x78, 0x21, 0x0, 0xa3)

//************************************************************************************************
// ITimer
/**
	\ingroup gui */
//************************************************************************************************

interface ITimer: IUnknown
{
	virtual void CCL_API task () = 0;

	/** Safe to be called inside task routine to stop timer. */
	virtual void CCL_API kill () = 0;

	virtual void CCL_API addTask (ITimerTask* task) = 0;
	
	virtual void CCL_API removeTask (ITimerTask* task) = 0;
	
	DECLARE_IID (ITimer)
};

DEFINE_IID (ITimer, 0x75fc3c28, 0x7885, 0x4522, 0xb2, 0xd8, 0xa6, 0xe8, 0xb0, 0xd, 0x45, 0x46)

} // namespace CCL

#endif // _ccl_itimer_h
