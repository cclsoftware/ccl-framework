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
// Filename    : ccl/platform/cocoa/gui/printjob.ios.h
// Description : platform-specific print code
//
//************************************************************************************************

#ifndef _ccl_printjob_ios_h
#define _ccl_printjob_ios_h

#include "ccl/base/object.h"
#include "ccl/base/storage/url.h"

#include "ccl/gui/graphics/printservice.h"

#include "ccl/platform/cocoa/cclcocoa.h"

@class CCL_ISOLATED (PrintPageRenderer);

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CoordHelper
//************************************************************************************************

struct CoordHelper
{
	static CGFloat convertFromMM (CoordF l);
	static CoordF convertToMM (CGFloat l);
	
	static const int kDpi = 72;
};

//************************************************************************************************
// IOSPrintJob
//************************************************************************************************

class IOSPrintJob: public PrintJob
{
public:	
	DECLARE_CLASS_ABSTRACT (IOSPrintJob, PrintJob)

	void setUrl (CCL::UrlRef url) { pdfUrl = url; }
	
	// PrintJob
	tresult CCL_API run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window) override;
	
protected:
	Url pdfUrl;
	
	tresult exportPdf (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer);
	
	virtual CCL_ISOLATED (PrintPageRenderer)* getPrintRenderer (const PrinterDocumentInfo* docInfo, IPageRenderer* renderer) = 0;
};


//************************************************************************************************
// IOSQuartzPrintJob
//************************************************************************************************

class IOSQuartzPrintJob: public IOSPrintJob
{
public:
	DECLARE_CLASS (IOSQuartzPrintJob, IOSPrintJob)

protected:
	// IOSPrintJob
	CCL_ISOLATED (PrintPageRenderer)* getPrintRenderer (const PrinterDocumentInfo* docInfo, IPageRenderer* renderer) override;
};

//************************************************************************************************
// IOSSkiaPrintJob
//************************************************************************************************

class IOSSkiaPrintJob: public IOSPrintJob
{
public:
	DECLARE_CLASS (IOSSkiaPrintJob, IOSPrintJob)

protected:
	// IOSPrintJob
	CCL_ISOLATED (PrintPageRenderer)* getPrintRenderer (const PrinterDocumentInfo* docInfo, IPageRenderer* renderer) override;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_printjob_ios_h
