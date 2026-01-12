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
// Filename    : ccl/public/cclexports.h
// Description : CCL Exports
//
//************************************************************************************************

#ifndef _ccl_exports_h
#define _ccl_exports_h

#include "core/public/corebasicmacros.h" // LAZYCAT

#define CCL_EXPORT_PREFIX CCL

#ifdef CCL_EXPORT_POSTFIX
#define CCL_ISOLATED(symbol) LAZYCAT (CCL_EXPORT_PREFIX, LAZYCAT (symbol, CCL_EXPORT_POSTFIX))
#else
#define CCL_ISOLATED(symbol) LAZYCAT (CCL_EXPORT_PREFIX, symbol)
#endif

#endif // _ccl_exports_h
