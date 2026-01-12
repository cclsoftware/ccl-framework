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
// Filename    : ccl/public/gui/framework/ilistview.h
// Description : List View Interface
//
//************************************************************************************************

#ifndef _ccl_ilistview_h
#define _ccl_ilistview_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// List style
//************************************************************************************************

namespace Styles
{
	DEFINE_ENUM (ListViewType)
	{
		kListViewList,			///< vertical list of items with icon & title
		kListViewDetails,		///< vertical list of items with multiple columns
		kListViewIcons,			///< two-dimensional arrangement of icons & title
		kNumListViewTypes
	};
}

//************************************************************************************************
// IListView
/** 
	\ingroup gui_item */
//************************************************************************************************

interface IListView: IUnknown
{
	/** Set the list view type. */
	virtual void CCL_API setViewType (Styles::ListViewType which) = 0;

	/** Set text trim mode. */
	virtual void CCL_API setTextTrimMode (int trimMode) = 0;

	DECLARE_IID (IListView)
};

DEFINE_IID (IListView, 0x15929FBB, 0xA083, 0x4E13, 0x8E, 0x44, 0xCC, 0x3E, 0x8F, 0x6D, 0x9B, 0xCA)

} // namespace CCL

#endif // _ccl_ilistview_h
