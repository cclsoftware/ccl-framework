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
// Filename    : ccl/public/gui/idatatarget.h
// Description : Data Target Interface
//
//************************************************************************************************

#ifndef _ccl_idatatarget_h
#define _ccl_idatatarget_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IUnknownList;
interface IDragSession;
interface IView;

//************************************************************************************************
// IDataTarget
/** Interface for components that can receive data via drag'n'drop or copy & paste.
	Used for delegating drag events from a view to underlying components.
	The optional insertIndex might specify a position in the target, where appropriate. 
	\ingroup gui_data */
//************************************************************************************************

interface IDataTarget: IUnknown
{
	/** Check if data can be inserted. */
	virtual tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) = 0;
	
	/** Insert data. */
	virtual tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) = 0;

	DECLARE_IID (IDataTarget)
};

DEFINE_IID (IDataTarget, 0x9967D17C, 0xA519, 0x4C00, 0xA7, 0xCD, 0x92, 0x1E, 0x4D, 0x7C, 0x5E, 0xCB)

} // namespace CCL

#endif // _ccl_idatatarget_h
