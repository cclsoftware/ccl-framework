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
// Filename    : typelibscanner.h
// Description : Static Type Library Source Scanner
//
//************************************************************************************************

#ifndef _ccl_typelibscanner_h
#define _ccl_typelibscanner_h

#include "ccl/base/storage/url.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

namespace Model {
class ClassRepository; }

//************************************************************************************************
// TypeLibScanner
/** Builds a ClassRepository by statically scanning C++ source for DEFINE_TYPELIB_* macros, without
	compiling or running the scanned code. Since several type libraries typically coexist in the
	same source tree (e.g. SKIN_TYPELIB, VISUALSTYLES_TYPELIB), 'libraryToken' selects one - the
	exact, literal text of each macro's trailing 'library' argument, compared verbatim. */
//************************************************************************************************

class TypeLibScanner
{
public:
	/** Scan source files under all 'sourceRoots' (folders or single files, as one source tree) and add every entry for
		'libraryToken' to 'repository'. Returns false on a parse error or if no matching entry was found. */
	static bool scan (Model::ClassRepository& repository, const Vector<Url>& sourceRoots, CStringRef libraryToken);
};

} // namespace CCL

#endif // _ccl_typelibscanner_h
