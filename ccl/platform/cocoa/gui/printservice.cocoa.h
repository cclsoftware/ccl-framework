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
// Filename    : ccl/platform/cocoa/gui/printservice.cocoa.h
// Description : macOS Native Print Job Data
//
//************************************************************************************************

#ifndef _ccl_printservice_cocoa_h
#define _ccl_printservice_cocoa_h

#include "ccl/base/object.h"
#include "ccl/gui/graphics/printservice.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// MacOSPrintService
//************************************************************************************************

class MacOSPrintService: public PrintService
{
public:	
	static MacOSPrintService& instance () { return (MacOSPrintService&) PrintService::instance (); }
	
	// PrintService
	IPrintJob* CCL_API createPrintJob ();
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info);
	IPageSetupDialog* CCL_API createPageSetupDialog ();
	Features CCL_API getSupportedFeatures () const;
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path);
};

//************************************************************************************************
// MacOSPrintJobData
//************************************************************************************************

class MacOSPrintJobData: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (MacOSPrintJobData, Object)
	
	MacOSPrintJobData ();

	struct PageRage
	{
		int pageFrom;
		int pageTo;
	};
	PageRage pageRange;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_printservice_cocoa_h
