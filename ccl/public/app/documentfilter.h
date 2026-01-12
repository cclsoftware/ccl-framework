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
// Filename    : ccl/public/app/documentfilter.h
// Description : Document Filter
//
//************************************************************************************************

#ifndef _ccl_documentfilter_h
#define _ccl_documentfilter_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/app/idocumentfilter.h"

namespace CCL {

//************************************************************************************************
// DocumentFilter
//************************************************************************************************

class DocumentFilter: public Unknown,
					  public PluginInstance,
					  public IDocumentFilter
{
public:
	// IDocumentFilter
	int CCL_API getFlags () const override;
	tresult CCL_API installFile (IUrl& path) const override;
	tresult CCL_API canImportFile (CCL::UrlRef path) const override;
	tresult CCL_API showImportOptions (IDocument& document) override;
	tresult CCL_API importDocument (IDocument& document, IProgressNotify* progress = nullptr) override;
	tbool CCL_API canExportDocument (IDocument& document) const override;
	tresult CCL_API showExportOptions (IDocument& document) override;
	tresult CCL_API exportDocument (IDocument& document, ExportParams& params, IProgressNotify* progress = nullptr) override;
	tresult CCL_API finalizeDocumentExport (CCL::IDocument& document, ExportParams& params) override;
	tbool CCL_API canMergeDocuments (IDocument& target, UrlRef sourceUrl) override;
	tresult CCL_API mergeDocuments (IDocument& target, IDocument& source, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (IDocumentFilter, IPluginInstance, Unknown)
};

} // namespace CCL

#endif // _ccl_documentfilter_h
