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
// Filename    : ccl/platform/cocoa/gui/printservice.cocoa.mm
// Description : platform-specific printer dialog code
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/printservice.cocoa.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/gui/printjob.cocoa.h"

using namespace CCL;

extern void modalStateEnter ();
extern void modalStateLeave ();

namespace CCL {
namespace MacOS {

//************************************************************************************************
// MacOSPageSetupDialog
//************************************************************************************************

class MacOSPageSetupDialog: public PageSetupDialog
{
public:
	DECLARE_CLASS (MacOSPageSetupDialog, PageSetupDialog)
	
	// PrintService
	tbool CCL_API run (PageSetup& pageSetup, IWindow* window) override;	
};

} // namespace MacOS
} // namespace CCL


using namespace CCL;
using namespace MacOS;


//************************************************************************************************
// MacOSPrintService
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (PrintService, MacOSPrintService)

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API MacOSPrintService::createPrintJob ()
{
	return ccl_cast<PrintJob> (NativeGraphicsEngine::instance ().createPrintJob ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MacOSPrintService::getDefaultPrinterInfo (PrinterInfo& info)
{
	NSPrinter* defaultPrinter = [[NSPrintInfo sharedPrintInfo] printer];
	info.name.empty ();
	info.name.appendNativeString (defaultPrinter.name);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPageSetupDialog* CCL_API MacOSPrintService::createPageSetupDialog ()
{
	return NEW MacOSPageSetupDialog;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MacOSPrintService::Features CCL_API MacOSPrintService::getSupportedFeatures () const
{
	return kPrinting|kPdfCreation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API MacOSPrintService::createPdfPrintJob (UrlRef path)
{
	MacOSPrintJob* job = ccl_cast<MacOSPrintJob> (NativeGraphicsEngine::instance ().createPrintJob ());
	if(job)
		job->setUrl (path);
	
	return job;
}

//************************************************************************************************
// MacOSPageSetupDialog
//************************************************************************************************

DEFINE_CLASS (MacOSPageSetupDialog, PageSetupDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MacOSPageSetupDialog::run (PageSetup& setup, IWindow* window)
{
	NSPageLayout* layout = [NSPageLayout pageLayout];
	if(!layout)
		return false;
	
	NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
	if(!printInfo)
		return false;
		
	printInfo.orientation = setup.orientation == kPageOrientationPortrait ? NSPaperOrientationPortrait : NSPaperOrientationLandscape;
	if(!setup.size.isNull ())
		printInfo.paperSize = NSMakeSize (CoordHelper::convertFromMM (setup.size.x), CoordHelper::convertFromMM (setup.size.y));
	printInfo.topMargin = CoordHelper::convertFromMM (setup.margins.top);
	printInfo.bottomMargin = CoordHelper::convertFromMM (setup.margins.bottom);
	printInfo.leftMargin = CoordHelper::convertFromMM (setup.margins.left);
	printInfo.rightMargin = CoordHelper::convertFromMM (setup.margins.right);

	modalStateEnter ();
	[layout runModal];
	modalStateLeave ();

	setup.orientation = printInfo.orientation == NSPaperOrientationPortrait ? kPageOrientationPortrait : kPageOrientationLandscape;
	setup.size.x = CoordHelper::convertToMM (printInfo.paperSize.width);
	setup.size.y = CoordHelper::convertToMM (printInfo.paperSize.height);
	setup.margins.top = CoordHelper::convertToMM (printInfo.topMargin);
	setup.margins.bottom = CoordHelper::convertToMM (printInfo.bottomMargin);
	setup.margins.left = CoordHelper::convertToMM (printInfo.leftMargin);
	setup.margins.right = CoordHelper::convertToMM (printInfo.rightMargin);

	return true;
}

//************************************************************************************************
// MacOSPrintJobData
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MacOSPrintJobData, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MacOSPrintJobData::MacOSPrintJobData ()
{
	pageRange = {0, 0};
}

