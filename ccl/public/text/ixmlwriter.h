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
// Filename    : ccl/public/text/ixmlwriter.h
// Description : XML Writer Interface
//
//************************************************************************************************

#ifndef _ccl_ixmlwriter_h
#define _ccl_ixmlwriter_h

#include "ccl/public/text/itextwriter.h"

namespace CCL {

//************************************************************************************************
// IXmlWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface IXmlWriter: ISgmlWriter
{
	/** Write character data using "CDATA" syntax. */
	virtual tresult CCL_API characterData (IStream& charData, TextEncoding encoding = Text::kUnknownEncoding) = 0;

	DECLARE_IID (IXmlWriter)
};

DEFINE_IID (IXmlWriter, 0x233f3a82, 0x6682, 0x4d4f, 0xa8, 0xef, 0x3b, 0x33, 0x38, 0x94, 0x13, 0x84)

} // namespace CCL

#endif // _ccl_ixmlwriter_h
