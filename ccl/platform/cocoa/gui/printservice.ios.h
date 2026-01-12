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
// Filename    : ccl/platform/cocoa/gui/printservice.ios.h
// Description : platform-specific printer dialog code
//
//************************************************************************************************

#ifndef _ccl_printservice_ios_h
#define _ccl_printservice_ios_h

#include "ccl/base/object.h"
#include "ccl/gui/graphics/printservice.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// IOSPrintService
//************************************************************************************************

class IOSPrintService: public PrintService
{
public:
	static IOSPrintService& instance () { return (IOSPrintService&) PrintService::instance (); }
	
	// PrintService
	IPrintJob* CCL_API createPrintJob ();
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info);
	IPageSetupDialog* CCL_API createPageSetupDialog ();
	Features CCL_API getSupportedFeatures () const;
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path);
};

////************************************************************************************************
//// IOSPrintJobData
////************************************************************************************************
//
//class IOSPrintJobData: public Object
//{
//public:	
//	DECLARE_CLASS_ABSTRACT (IOSPrintJobData, Object)
//	
//	IOSPrintJobData ();
//	
//	PROPERTY_STRING (jobName, JobName)
//	struct PageRage
//	{
//		int pageFrom;
//		int pageTo;
//	};
//	PageRage pageRange;
//	SymbolicPaperFormat paperFormat;
//	
//};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_printservice_ios_h
