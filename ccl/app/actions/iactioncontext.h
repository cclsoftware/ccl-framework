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
// Filename    : ccl/app/actions/iactioncontext.h
// Description : Action Context
//
//************************************************************************************************

#ifndef _ccl_iactioncontext_h
#define _ccl_iactioncontext_h

#include "ccl/base/object.h"

namespace CCL {

class Action;
class ActionJournal;

//************************************************************************************************
// IActionContext
/** Abstract context to provide an action journal. */
//************************************************************************************************

interface IActionContext: IUnknown
{
	virtual ActionJournal* getActionJournal () const = 0;

	DECLARE_IID (IActionContext)
};

//************************************************************************************************
// MultiActionScope
//************************************************************************************************

struct MultiActionScope
{
	IActionContext* context;

	MultiActionScope (IActionContext* context, StringRef description);
	MultiActionScope (IActionContext* context, StringRef description, StringRef details);
	MultiActionScope (IActionContext* context, Action* multiAction);
	~MultiActionScope ();
	
	void cancel ();
};

} // namespace CCL

#endif // _ccl_iactioncontext_h
