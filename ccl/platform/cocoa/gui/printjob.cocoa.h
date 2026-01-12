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
// Filename    : ccl/platform/cocoa/gui/printjob.cocoa.h
// Description : platform-specific print code
//
//************************************************************************************************

#ifndef _ccl_printjob_cocoa_h
#define _ccl_printjob_cocoa_h

#include "ccl/base/object.h"
#include "ccl/base/storage/url.h"

#include "ccl/gui/graphics/printservice.h"
#include <CoreGraphics/CGBase.h>

@class NSPrintOperation;
@class CCL_ISOLATED (PrintView);

namespace CCL {
namespace MacOS {

class MacOSPrintJobData;

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
// MacOSPrintJob
//************************************************************************************************

class MacOSPrintJob: public PrintJob
{
public:
	DECLARE_CLASS_ABSTRACT (MacOSPrintJob, PrintJob)

	void setUrl (CCL::UrlRef url) { pdfUrl = url; }
	
	// PrintJob
	tresult CCL_API run (const PrinterDocumentInfo& doc, IPageRenderer* renderer, JobMode mode, IWindow* window) override;
	
protected:
	Url pdfUrl;

	virtual CCL_ISOLATED (PrintView)* createPrintView (MacOSPrintJobData* jobData) = 0;
};


//************************************************************************************************
// MacOSQuartzPrintJob
//************************************************************************************************

class MacOSQuartzPrintJob: public MacOSPrintJob
{
public:
	DECLARE_CLASS (MacOSQuartzPrintJob, MacOSPrintJob)

protected:
	// MacOSPrintJob
	CCL_ISOLATED (PrintView)* createPrintView (MacOSPrintJobData* jobData) override;
};

//************************************************************************************************
// MacOSSkiaPrintJob
//************************************************************************************************

class MacOSSkiaPrintJob: public MacOSPrintJob
{
public:
	DECLARE_CLASS (MacOSSkiaPrintJob, MacOSPrintJob)

protected:
	// MacOSPrintJob
	CCL_ISOLATED (PrintView)* createPrintView (MacOSPrintJobData* jobData) override;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_printjob_cocoa_h
