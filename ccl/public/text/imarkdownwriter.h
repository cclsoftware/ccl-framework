//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/text/imarkdownwriter.h
// Description : Markdown Writer Interface
//
//************************************************************************************************

#ifndef _ccl_imarkdownwriter_h
#define _ccl_imarkdownwriter_h

#include "ccl/public/text/itextwriter.h"

namespace CCL {

//************************************************************************************************
// IMarkdownWriter
/**	Markdown Writer - created via System::CreateMarkdownWriter ()
    \ingroup ccl_text */
//************************************************************************************************

interface IMarkdownWriter: IMarkupWriter
{
	DECLARE_IID (IMarkdownWriter)
};

DEFINE_IID (IMarkdownWriter, 0x807B8CA9, 0xE38B, 0x440E, 0xB7, 0xD6, 0x70, 0x19, 0x41, 0x90, 0xC9, 0xBE)

} // namespace CCL

#endif // _ccl_imarkdownwriter_h
