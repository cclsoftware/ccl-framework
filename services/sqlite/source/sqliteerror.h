//************************************************************************************************
//
// SQLite Database Engine
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
// Filename    : sqliteerror.h
// Description : SQLite error logging
//
//************************************************************************************************

#ifndef _sqliteerror_h
#define _sqliteerror_h

#include "ccl/public/text/cclstring.h"

#define LOG_ERRORS (1 && DEBUG_LOG)

struct sqlite3;

namespace CCL {
namespace Database {

//////////////////////////////////////////////////////////////////////////////////////////////////

void logError (sqlite3* connection, int code = -1, const char* context = nullptr);

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Database
} // namespace CCL

#endif // _sqliteerror_h
