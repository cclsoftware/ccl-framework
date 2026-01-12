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
// Filename    : ccl/public/app/iedittask.h
// Description : Edit Task Interface
//
//************************************************************************************************

#ifndef _ccl_iedittask_h
#define _ccl_iedittask_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IObject;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_EDITTASK_	"EditTask"
#define PLUG_CATEGORY_EDITTASK	CCLSTR (PLUG_CATEGORY_EDITTASK_)

#define MAKE_EDITTASK_CATEGORY(subCategory) PLUG_CATEGORY_EDITTASK_ ":" subCategory
#define MAKE_EDITTASK_FULLNAME(subCategory, className) MAKE_EDITTASK_CATEGORY (subCategory) ":" className

//************************************************************************************************
// IEditTask
/**	\ingroup app_inter */
//************************************************************************************************

interface IEditTask: IUnknown
{
	/** Show configuration dialog, etc. */
	virtual tresult CCL_API prepareEdit (IObject& context) = 0;

	/** Perform actual task, context contains objects relevant for editing. */
	virtual tresult CCL_API performEdit (IObject& context) = 0;

	// Additional properties (IObject)
	DECLARE_STRINGID_MEMBER (kFormName)
	DECLARE_STRINGID_MEMBER (kThemeID)

	DECLARE_IID (IEditTask)
};

DEFINE_IID (IEditTask, 0x1382e475, 0xeba4, 0x4151, 0x8a, 0xc2, 0x57, 0xd5, 0xa5, 0xef, 0x64, 0xb2)
DEFINE_STRINGID_MEMBER (IEditTask, kFormName, "formName")
DEFINE_STRINGID_MEMBER (IEditTask, kThemeID, "themeID")

} // namespace CCL

#endif // _ccl_iedittask_h
