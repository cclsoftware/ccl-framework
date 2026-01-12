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
// Filename    : ccl/platform/cocoa/gui/printservice.ios.mm
// Description : platform-specific printer dialog code
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/printservice.ios.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/gui/printjob.ios.h"

namespace CCL {
namespace MacOS {

void linkIOSPrintservice ();

} // namespace MacOS
} // namespace CCL


using namespace CCL;
using namespace MacOS;

void linkIOSPrintservice () {} // call to link this file

//************************************************************************************************
// IOSPrintService
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (PrintService, IOSPrintService)

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API IOSPrintService::createPrintJob ()
{
	return ccl_cast<PrintJob> (NativeGraphicsEngine::instance ().createPrintJob ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IOSPrintService::getDefaultPrinterInfo (PrinterInfo& info)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPageSetupDialog* CCL_API IOSPrintService::createPageSetupDialog ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSPrintService::Features CCL_API IOSPrintService::getSupportedFeatures () const
{
	return kPrinting|kPdfCreation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API IOSPrintService::createPdfPrintJob (UrlRef path)
{
	IOSPrintJob* job = ccl_cast<IOSPrintJob> (NativeGraphicsEngine::instance ().createPrintJob ());
	if(job)
		job->setUrl (path);
	
	return job;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

////************************************************************************************************
//// IOSPrintJobData
////************************************************************************************************
//
//DEFINE_CLASS_ABSTRACT_HIDDEN (IOSPrintJobData, Object)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//IOSPrintJobData::IOSPrintJobData ()
//{
//	pageRange = { 0, 0 };
//}
//
