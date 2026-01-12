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
// Filename    : ccl/public/system/imultiworker.h
// Description : Multiworker Interface
//
//************************************************************************************************

#ifndef _ccl_imultiworker_h
#define _ccl_imultiworker_h

#include "ccl/public/system/threadsync.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// Threading::Work
/**	\ingroup ccl_system */
//************************************************************************************************

struct Work: IAtomicStack::Element
{
	virtual void work () = 0;
};

//************************************************************************************************
// IMultiWorker
/** \ingroup ccl_system */
//************************************************************************************************

interface IMultiWorker: IUnknown
{
	virtual void CCL_API terminate () = 0;
	
	virtual void CCL_API firstRun () = 0;
	
	virtual tbool CCL_API isDone () = 0;
		
	virtual int CCL_API work () = 0;
	
	virtual void CCL_API push (Work* work) = 0;
	
	virtual tbool CCL_API pushAndSignal (Work* work, tbool failWhenAllBusy = false) = 0;

	virtual int CCL_API getThreadErrors () const = 0;

	DECLARE_IID (IMultiWorker)
};

DEFINE_IID (IMultiWorker, 0xd80b2eba, 0xdf62, 0x4765, 0x97, 0x9, 0x8e, 0x5e, 0x2b, 0x9d, 0xbc, 0xde)

} // namespace Threading
} // namespace CCL

#endif // _ccl_imultiworker_h
