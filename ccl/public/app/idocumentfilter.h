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
// Filename    : ccl/public/app/idocumentfilter.h
// Description : Document Filter Interface
//
//************************************************************************************************

#ifndef _ccl_idocumentfilter_h
#define _ccl_idocumentfilter_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class FileType;
interface IDocument;
interface IProgressNotify;
interface IUnknownList;
interface IObjectFilter;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Class category for document filters
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_DOCUMENTFILTER_ "DocumentFilter"
#define PLUG_CATEGORY_DOCUMENTFILTER CCLSTR (PLUG_CATEGORY_DOCUMENTFILTER_)

//************************************************************************************************
// IDocumentFilter
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocumentFilter: IUnknown
{
	enum Flags
	{
		kCanImport = 1<<0,				///< filter can import documents
		kCanExport = 1<<1,				///< filter can export documents
		kHasImportOptions = 1<<2,		///< filter has options for import
		kHasExportOptions = 1<<3,		///< filter has options for export
		kIsPrivate = 1<<4,				///< filter is used internally, should not be presented to user
		kInstallRequired = 1<<5,		///< document type requires installation
		kNeedsCancel = 1<<6,			///< filter can be time consuming, the progress should enable cancel
		kNeedsExportFinalization = 1<<7	///< filter needs extra processing step after the export is complete (out of progress scope)
	};

	struct ExportParams
	{
		ExportParams (UrlRef targetUrl, IUnknownList* additionalFiles = nullptr, IObjectFilter* dataFilter = nullptr)
		: targetUrl (targetUrl), additionalFiles (additionalFiles), dataFilter (dataFilter), setupController (nullptr) {}

		UrlRef targetUrl;					///< file to create
		IUnknownList* additionalFiles;		///< [output] list of additionally written files (optional)
		IObjectFilter* dataFilter;			///< export only objects that match this filter (optional)
		IUnknown* setupController;          ///< controller of setup view (optional) - if filter implements IViewFactory instead of showExportOptions
	};

	/** Get filter capability flags. */
	virtual int CCL_API getFlags () const = 0;

	/** Get document file type. */
	virtual const FileType& CCL_API getFileType () const = 0;

	/**	Install document to location determined by filter (optional).
		This might move or copy the document file before it's being imported. */
	virtual tresult CCL_API installFile (IUrl& path) const = 0;

	/** Check if given document can be imported. */
	virtual tresult CCL_API canImportFile (UrlRef path) const = 0;

	/** Show options before import (optional). */
	virtual tresult CCL_API showImportOptions (IDocument& document) = 0;

	/** Import document. */
	virtual tresult CCL_API importDocument (IDocument& document, IProgressNotify* progress = nullptr) = 0;

	/** Check if given document can be exported. */
	virtual tbool CCL_API canExportDocument (IDocument& document) const = 0;

	/** Show options before export (optional). */
	virtual tresult CCL_API showExportOptions (IDocument& document) = 0;

	/** Export document. */
	virtual tresult CCL_API exportDocument (IDocument& document, ExportParams& params, IProgressNotify* progress = nullptr) = 0;

	/** Post process the exported document (optional) */
	virtual tresult CCL_API finalizeDocumentExport (IDocument& document, ExportParams& params) = 0;

	/** Check if given documents can be merged. */
	virtual tbool CCL_API canMergeDocuments (IDocument& target, UrlRef sourceUrl) = 0;

	/** Merge documents. Source is not loaded so far... */
	virtual tresult CCL_API mergeDocuments (IDocument& target, IDocument& source, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IDocumentFilter)
};

DEFINE_IID (IDocumentFilter, 0xeeda2d1a, 0xcdc9, 0x4639, 0xab, 0xcb, 0x1f, 0x1, 0xf6, 0xd8, 0x68, 0x30)

} // namespace CCL

#endif // _ccl_idocumentfilter_h
