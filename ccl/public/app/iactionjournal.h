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
// Filename    : ccl/public/app/iactionjournal.h
// Description : Action Journal Interface
//
//************************************************************************************************

#ifndef _ccl_iactionjournal_h
#define _ccl_iactionjournal_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// IActionJournal
/** \ingroup app_inter */
//************************************************************************************************

interface IActionJournal: IUnknown
{
	virtual int64 CCL_API getLastEditTime () const = 0;
	
	virtual tbool CCL_API canUndoLastEdit () const = 0;
	
	virtual tbool CCL_API canRedoLastEdit () const = 0;

	virtual tbool CCL_API undoLastEdit () = 0;

	virtual tbool CCL_API redoLastEdit () = 0;

	virtual tbool CCL_API isPerformingAction () const = 0;

	DECLARE_IID (IActionJournal)
};

DEFINE_IID (IActionJournal, 0xd70f5dc5, 0xd9bd, 0x4230, 0xb9, 0x8, 0xf5, 0x57, 0xcc, 0x41, 0x79, 0x3f)

//************************************************************************************************
// IActionExecuter
/** \ingroup app_inter */
//************************************************************************************************

interface IActionExecuter: IUnknown
{
	virtual tbool CCL_API beginMultiAction (StringRef description, StringRef details = nullptr) = 0;

	virtual tbool CCL_API endMultiAction (tbool cancel = false) = 0;

	virtual void CCL_API setExecuteActionImmediately (tbool state) = 0;

	virtual tbool CCL_API isExecuteActionImmediately () const = 0;

	virtual void CCL_API setJournalEnabled (tbool enabled) = 0;
	
	virtual tbool CCL_API isJournalEnabled () const = 0;

	DECLARE_IID (IActionExecuter)
};

DEFINE_IID (IActionExecuter, 0x11a9fb1d, 0x5338, 0x4f70, 0xaa, 0xc9, 0x97, 0x39, 0x3e, 0xb6, 0x4a, 0xfd)

} // namespace CCL

#endif // _ccl_iactionjournal_h
