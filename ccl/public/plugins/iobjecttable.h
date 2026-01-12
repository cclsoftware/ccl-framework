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
// Filename    : ccl/public/plugins/iobjecttable.h
// Description : Object Table Interface
//
//************************************************************************************************

#ifndef _ccl_iobjecttable_h
#define _ccl_iobjecttable_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IObjectTable
/**	The object table is used to share global objects between modules.
	\ingroup base_plug  */
//************************************************************************************************

interface IObjectTable: IUnknown
{
	enum Flags
	{
		kIsHostApp = 1<<0 ///< object can be accessed via kHostApp alias
	};

	/** Register object with unique identifier and name. */
	virtual tresult CCL_API registerObject (IUnknown* obj, UIDRef id, StringID name, int flags = 0) = 0;

	/** Unregister object. */
	virtual tresult CCL_API unregisterObject (IUnknown* obj) = 0;

	/** Get number of objects. */
	virtual int CCL_API countObjects () const = 0;

	/** Get name of object at given index. */
	virtual StringID CCL_API getObjectName (int index) const = 0;

	/** Get object by table index. */
	virtual IUnknown* CCL_API getObjectByIndex (int index) const = 0;

	/** Get object by unique identifier. */
	virtual IUnknown* CCL_API getObjectByID (UIDRef id) const = 0;

	/** Get object by name. */
	virtual IUnknown* CCL_API getObjectByName (StringID name) const = 0;
	
	/** Get object by URL. */
	virtual IUnknown* CCL_API getObjectByUrl (UrlRef url) const = 0;

	DECLARE_STRINGID_MEMBER (kHostApp) ///< host application alias

	DECLARE_IID (IObjectTable)
};

DEFINE_IID (IObjectTable, 0x89656398, 0x20a4, 0x4107, 0xa0, 0x52, 0xf0, 0xa2, 0x8e, 0xfb, 0xaf, 0x53)
DEFINE_STRINGID_MEMBER (IObjectTable, kHostApp, "hostapp")

} // namespace CCL

#endif // _ccl_iobjecttable_h
