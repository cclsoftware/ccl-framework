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
// Filename    : ccl/public/app/ieditenvironment.h
// Description : Edit Environment Interface
//
//************************************************************************************************

#ifndef _ccl_ieditenvironment_h
#define _ccl_ieditenvironment_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IObject;
interface ISelection;
interface IActionJournal;
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_EDITADDIN_ "EditAddIn"
#define PLUG_CATEGORY_EDITADDIN	CCLSTR (PLUG_CATEGORY_EDITADDIN_)

#define MAKE_EDITADDIN_CATEGORY(subCategory) PLUG_CATEGORY_EDITADDIN_ ":" subCategory

//************************************************************************************************
// IEditEnvironment
/**	\ingroup app_inter */
//************************************************************************************************

interface IEditEnvironment: IUnknown
{
	virtual IObject* CCL_API getMainEditor () = 0;

	virtual IObject* CCL_API getActiveEditor () = 0;
	
	virtual ISelection* CCL_API getActiveSelection () = 0;
	
	virtual IUnknown* CCL_API getFocusItem () = 0;
	
	virtual IObject* CCL_API getFocusItemPropertyEditor () = 0;

	virtual IUnknown* CCL_API getAddInInstance (StringRef name) = 0;
	
	virtual IActionJournal* CCL_API getActionJournal () = 0;

	virtual tbool CCL_API canRunEditTask (UIDRef cid) = 0;

	virtual tbool CCL_API runEditTask (UIDRef cid, IAttributeList* arguments = nullptr) = 0;

	DECLARE_STRINGID_MEMBER (kComponentName) ///< edit environment component name

	// notifications
	DECLARE_STRINGID_MEMBER (kActiveEditorChanged)
	DECLARE_STRINGID_MEMBER (kSelectionChanged)
	DECLARE_STRINGID_MEMBER (kFocusItemChanged)

	DECLARE_IID (IEditEnvironment)
};

DEFINE_IID (IEditEnvironment, 0x49f3b53c, 0xdc26, 0x4007, 0xa6, 0xd9, 0x3, 0xf8, 0xa9, 0xaf, 0x3c, 0x49)
DEFINE_STRINGID_MEMBER (IEditEnvironment, kComponentName, "EditEnvironment")
DEFINE_STRINGID_MEMBER (IEditEnvironment, kActiveEditorChanged, "activeEditorChanged")
DEFINE_STRINGID_MEMBER (IEditEnvironment, kSelectionChanged, "selectionChanged")
DEFINE_STRINGID_MEMBER (IEditEnvironment, kFocusItemChanged, "focusItemChanged")

} // namespace CCL

#endif // _ccl_ieditenvironment_h
