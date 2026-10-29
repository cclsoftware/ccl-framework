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
// Filename    : sourcefiletraverser.h
// Description : Shared recursive source-tree walk for ClassScanner/DocumentationScanner
//
//************************************************************************************************

#ifndef _ccl_sourcefiletraverser_h
#define _ccl_sourcefiletraverser_h

#include "ccl/base/storage/url.h"

namespace CCL {

interface IProgressNotify;

//************************************************************************************************
// SourceFileVisitor
//************************************************************************************************

class SourceFileVisitor
{
public:
	/** Called once per file matching the walk's filter, in a folder for which
		isMatchingFolder() returned true. */
	virtual void visitFile (UrlRef path) = 0;

	/** Override to restrict which folders get descended into (e.g. GuiDocuScanner restricts to
		/gui and /ccl/meta paths). Default: every folder matches. */
	virtual bool isMatchingFolder (UrlRef folder) { return true; }
};

//************************************************************************************************
// SourceFileTraverser
//************************************************************************************************

class SourceFileTraverser
{
public:
	/** Recursively traverses 'root', calling visitor.visitFile() for every file matching
		'filter' inside a folder for which visitor.isMatchingFolder() returned true. If 'root' is
		a file, it is visited directly if it matches 'filter' (no folder matching, since it was
		requested explicitly). Reports progress and checks for cancellation via 'progress' if given. */
	static bool traverse (UrlRef root, IUrlFilter& filter, SourceFileVisitor& visitor,
		IProgressNotify* progress = nullptr);

	/** The shared ".h"/".cpp" filter used by ClassScanner, DocumentationScanner, and TypeLibScanner. */
	static IUrlFilter& cppSourceFilter ();

protected:
	static bool traverseFolder (UrlRef folder, IUrlFilter& filter, SourceFileVisitor& visitor,
		IProgressNotify* progress, bool inMatchingFolder);
};

} // namespace CCL

#endif // _ccl_sourcefiletraverser_h
