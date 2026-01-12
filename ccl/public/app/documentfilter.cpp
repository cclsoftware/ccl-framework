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
// Filename    : ccl/public/app/documentfilter.cpp
// Description : Document Filter
//
//************************************************************************************************

#include "ccl/public/app/documentfilter.h"

using namespace CCL;

//************************************************************************************************
// DocumentFilter
//************************************************************************************************

int CCL_API DocumentFilter::getFlags () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::installFile (IUrl& path) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::canImportFile (UrlRef path) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::showImportOptions (IDocument& document)
{
	CCL_NOT_IMPL ("DocumentFilter::showImportOptions()")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::importDocument (IDocument& document, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("DocumentFilter::importDocument()")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentFilter::canExportDocument (IDocument& document) const
{
	CCL_NOT_IMPL ("DocumentFilter::canExportDocument()")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::showExportOptions (IDocument& document)
{
	CCL_NOT_IMPL ("DocumentFilter::showExportOptions()")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::exportDocument (IDocument& document, ExportParams& params, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("DocumentFilter::exportDocument()")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::finalizeDocumentExport (IDocument& document, ExportParams& params)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentFilter::canMergeDocuments (IDocument& target, UrlRef sourceUrl)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentFilter::mergeDocuments (IDocument& target, IDocument& source, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("DocumentFilter::importDocument()")
	return kResultNotImplemented;
}
