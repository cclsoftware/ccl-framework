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
// Filename    : classscanner.h
// Description : Static Class Model Source Scanner
//
//************************************************************************************************

#ifndef _ccl_classscanner_h
#define _ccl_classscanner_h

#include "ccl/base/storage/url.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

namespace Model {
class ClassRepository; }

//************************************************************************************************
// ClassScanner
/** Builds a ClassRepository by statically scanning C++ source for the
	DEFINE_CLASS-family macros (ccl/base/objectmacros.h), without compiling or
	running the scanned code. This is the static counterpart to
	ClassRepositoryBuilder::build(ITypeLibrary&), which instead reflects over a
	live, already-loaded type library. */
//************************************************************************************************

class ClassScanner
{
public:
	/** Scan every .h/.cpp file under all 'sourceRoots' (folders or single files) and add the resulting classes
		to 'repository'. The roots are scanned as one source tree, so classes can reference
		parents and symbols from any of them. Returns false and logs an error on unrecoverable
		parse errors (e.g. ambiguous macro-argument resolution). */
	static bool scan (Model::ClassRepository& repository, const Vector<Url>& sourceRoots);
};

} // namespace CCL

#endif // _ccl_classscanner_h
