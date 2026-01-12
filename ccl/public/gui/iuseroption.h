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
// Filename    : ccl/public/gui/iuseroption.h
// Description : User option Interface
//
//************************************************************************************************

#ifndef _ccl_iuseroption_h
#define _ccl_iuseroption_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (UserOptionDialog, 0x6e0c7b4, 0x73e, 0x4187, 0xa5, 0x30, 0xa0, 0xdf, 0x8d, 0xf6, 0x5, 0x26)
}

#define PLUG_CATEGORY_USEROPTION CCLSTR ("UserOption")

//************************************************************************************************
// IUserOption
/** User option interface. 
	\ingroup gui */
//************************************************************************************************

interface IUserOption: IUnknown
{
	virtual StringRef CCL_API getName () const = 0;
	
	virtual StringRef CCL_API getTitle () const = 0;
	
	virtual IImage* CCL_API getIcon () const = 0;

	virtual tbool CCL_API needsApply () const = 0;
	
	virtual tbool CCL_API apply () = 0;

	virtual void CCL_API opened () = 0;

	virtual void CCL_API closed () = 0;

	static const String strSeparator;	///< title separator

	DECLARE_IID (IUserOption)
};

DEFINE_IID (IUserOption, 0xF5C54669, 0xB7BD, 0x43BA, 0xB3, 0xC7, 0x2E, 0xCE, 0x5B, 0x25, 0x95, 0xF9)

//************************************************************************************************
// IUserOptionList
/** User option list interface. 
	\ingroup gui */
//************************************************************************************************

interface IUserOptionList: IUnknown
{
	virtual StringRef CCL_API getName () const = 0;

	virtual StringRef CCL_API getTitle () const = 0;

	virtual int CCL_API countOptions () const = 0;

	virtual IUserOption* CCL_API getOption (int index) const = 0;

	virtual StringRef CCL_API getLastSelected () const = 0;

	virtual void CCL_API setLastSelected (StringRef name) = 0;

	DECLARE_IID (IUserOptionList)
};

DEFINE_IID (IUserOptionList, 0xa3fd6c2c, 0xc1, 0x4f46, 0x8a, 0xff, 0xf6, 0xce, 0xad, 0x72, 0x87, 0xd1)

//************************************************************************************************
// IUserOptionDialog
/** User option dialog interface. 
	\ingroup gui */
//************************************************************************************************

interface IUserOptionDialog: IUnknown
{
	virtual tresult CCL_API run (IUserOptionList& optionList) = 0;

	virtual tresult CCL_API run (IUserOptionList* lists[], int count, int index) = 0;

	DECLARE_IID (IUserOptionDialog)
};

DEFINE_IID (IUserOptionDialog, 0xFBD80A3A, 0x979D, 0x491D, 0xA1, 0xEC, 0x06, 0x92, 0xE9, 0xD1, 0x6D, 0x6B)

} // namespace CCL

#endif // _ccl_iuseroption_h
