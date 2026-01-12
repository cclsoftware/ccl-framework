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
// Filename    : ccl/public/storage/iconfiguration.h
// Description : Configuration Interface
//
//************************************************************************************************

#ifndef _ccl_iconfiguration_h
#define _ccl_iconfiguration_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Configuration {

//************************************************************************************************
// Configuration::IRegistry
/** \ingroup base_io */
//************************************************************************************************

interface IRegistry: IUnknown
{
	/** Set configuration value. */
	virtual void CCL_API setValue (StringID section, StringID key, VariantRef value) = 0;

	/** Append a value to a list. */
	virtual void CCL_API appendValue (StringID section, StringID key, VariantRef value) = 0;

	/** Get configuration value. */
	virtual tbool CCL_API getValue (Variant& value, StringID section, StringID key) const = 0;

	DECLARE_IID (IRegistry)
};

DEFINE_IID (IRegistry, 0x109cf9c7, 0x202c, 0x4d43, 0xb1, 0x27, 0xe9, 0xb9, 0x96, 0x5, 0x37, 0x45)

} // namespace Configuration
} // namespace CCL

#endif // _ccl_iconfiguration_h
