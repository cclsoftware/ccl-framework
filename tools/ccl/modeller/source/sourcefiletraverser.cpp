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
// Filename    : sourcefiletraverser.cpp
// Description : Shared recursive source-tree walk for ClassScanner/DocumentationScanner
//
//************************************************************************************************

#include "sourcefiletraverser.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/smartptr.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceFileTraverser::traverse (UrlRef root, IUrlFilter& filter, SourceFileVisitor& visitor, IProgressNotify* progress)
{
	if(root.isFile ())
	{
		if(filter.matches (root))
			visitor.visitFile (root);
		else
		{
			String rootStr;
			root.toDisplayString (rootStr);
			Logging::warning ("Ignoring '%(1)', not a supported source file", rootStr);
		}
		return true;
	}

	return traverseFolder (root, filter, visitor, progress, visitor.isMatchingFolder (root));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter& SourceFileTraverser::cppSourceFilter ()
{
	static AutoPtr<FileTypeFilter> filter;
	if(filter == nullptr)
	{
		filter = NEW FileTypeFilter;
		filter->addFileType (FileType (nullptr, "h"));
		filter->addFileType (FileType (nullptr, "cpp"));
	}
	return *filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceFileTraverser::traverseFolder (UrlRef folder, IUrlFilter& filter, SourceFileVisitor& visitor,
	IProgressNotify* progress, bool inMatchingFolder)
{
	ForEachFile (System::GetFileSystem ().newIterator (folder), _path)
		UrlRef path = *_path;
		if(path.isFolder ())
		{
			bool folderMatches = inMatchingFolder ? true : visitor.isMatchingFolder (path);

			if(progress && folderMatches)
				progress->setProgressText (path.getPath ());

			if(traverseFolder (path, filter, visitor, progress, folderMatches) == false)
				return false;
		}
		else if(inMatchingFolder && filter.matches (path))
		{
			visitor.visitFile (path);
		}

		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				return false;
		}
	EndFor

	return true;
}
