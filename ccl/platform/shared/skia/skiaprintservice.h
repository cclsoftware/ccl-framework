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
// Filename    : ccl/platform/shared/skia/skiaprintservice.h
// Description : Shared Skia-based print job implementation
//
//************************************************************************************************

#ifndef _ccl_skia_printservice_h
#define _ccl_skia_printservice_h

#include "ccl/gui/graphics/printservice.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// SkiaPrintJob
/** Base class for print jobs that render using Skia PDF backend. */
//************************************************************************************************

class SkiaPrintJob: public PrintJob
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaPrintJob, PrintJob)

	SkiaPrintJob (float defaultDpi = 72.0f);

protected:
	/** Render complete multi-page document to PDF stream using Skia.
	    Returns true on success, false on failure. */
	bool renderDocumentToStream (IStream& stream, IPageRenderer* renderer, const PrinterDocumentInfo& documentInfo);

	/** Render a single page to PDF stream using Skia.
	    Useful for platforms that render pages individually (e.g., macOS Quartz).
	    Returns true on success, false on failure. */
	bool renderSinglePageToStream (IStream& stream, IPageRenderer* renderer, int pageNumber, const PointF& pageSize, const RectF& printableArea, float dpi = 72.0f);

	/** Get page size with optional default fallback.
	    If documentInfo.pageSize is null, returns the specified default format size.
	    Returns false if no valid page size could be determined. */
	bool getPageSize (PointF& pageSize, const PrinterDocumentInfo& documentInfo, SymbolicPaperFormat defaultFormat = kPaperFormatA4);

	float defaultDpi;
};

} // namespace CCL

#endif // _ccl_skia_printservice_h