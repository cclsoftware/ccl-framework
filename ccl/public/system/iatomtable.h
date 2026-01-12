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
// Filename    : ccl/public/system/iatomtable.h
// Description : Atom Table Interface
//
//************************************************************************************************

#ifndef _ccl_iatomtable_h
#define _ccl_iatomtable_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IAtom
/**	\ingroup ccl_system */
//************************************************************************************************

interface IAtom: IUnknown
{
	/** Get atom name. */
	virtual StringID CCL_API getAtomName () const = 0;
	
	DECLARE_IID (IAtom)
};

DEFINE_IID (IAtom, 0x7cd01493, 0xc527, 0x41b4, 0x8e, 0xa2, 0x60, 0xeb, 0xab, 0x64, 0x4f, 0xe0)

//************************************************************************************************
// IAtomTable
/** \ingroup ccl_system */
//************************************************************************************************

interface IAtomTable: IUnknown
{
	/** Create (or reuse existing) atom. Must be released by caller. */
	virtual IAtom* CCL_API createAtom (StringID name) = 0;

	DECLARE_IID (IAtomTable)
};

DEFINE_IID (IAtomTable, 0xfc1d6474, 0x13c6, 0x492b, 0xa5, 0x21, 0xca, 0x4e, 0xdc, 0x79, 0x24, 0xd9)

} // namespace CCL

#endif // _ccl_iatomtable_h
