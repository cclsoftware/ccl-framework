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
// Filename    : ccl/public/extras/ibackupitem.h
// Description : Backup Interfaces
//
//************************************************************************************************

#ifndef _ccl_ibackupitem_h
#define _ccl_ibackupitem_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Class category for backup items
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_BACKUPITEM CCLSTR ("BackupItem")

//************************************************************************************************
// IBackupItem
/** Backup item interface. 
	\ingroup app_inter */
//************************************************************************************************

interface IBackupItem: IUnknown
{
	/** Get backup user folder. */
	virtual tresult CCL_API getUserFolder (String& title, IUrl& path) const = 0;

	DECLARE_IID (IBackupItem)
};

DEFINE_IID (IBackupItem, 0x81efeaf8, 0xbaa5, 0x4044, 0x97, 0x4, 0x9f, 0x95, 0x4c, 0x53, 0x7a, 0x3f)

} // namespace CCL

#endif // _ccl_ibackupitem_h
