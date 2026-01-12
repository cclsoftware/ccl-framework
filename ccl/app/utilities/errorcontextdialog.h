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
// Filename    : ccl/app/utilities/errorcontextdialog.h
// Description : Error Context List Dialog
//
//************************************************************************************************

#ifndef _ccl_errorcontextdialog_h
#define _ccl_errorcontextdialog_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IErrorContext;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alert functions extended
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Alert
{
	bool showErrorContextList (IErrorContext* context, StringRef text = nullptr, StringRef question = nullptr, bool deep = false);
}

} // namespace CCL

#endif // _ccl_errorcontextdialog_h
