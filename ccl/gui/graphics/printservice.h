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
// Filename    : ccl/gui/graphics/printservice.h
// Description : Printer Dialogs and Job
//
//************************************************************************************************

#ifndef _ccl_printservice_h
#define _ccl_printservice_h

#include "ccl/base/singleton.h"

#include "ccl/public/gui/framework/iprintservice.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// PrintService
/** Base class for platform print service. */
//************************************************************************************************

class PrintService: public Object,
				    public IPrintService,
				    public ExternalSingleton<PrintService>
{
public:
	PrintService ();

	void onPrintJobStarted ();
	void onPrintJobDone ();

	// IPrintService
	tbool CCL_API isAnyPrintJobActive () override;
	const PaperFormat& CCL_API getPaperFormat (SymbolicPaperFormat symbolicFormat) const override;
	const PaperFormat& CCL_API lookupPaperFormatBySize (PointF size, PageOrientation orientation) const override;
	Features CCL_API getSupportedFeatures () const override { return kPrinting; }
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override { return nullptr; }

	CLASS_INTERFACE (IPrintService, Object)
	
protected:
	Threading::AtomicInt printJobCounter;
	
	static const PaperFormat paperFormatTable[];
	static const PaperFormat kUnknownFormat;
};

//************************************************************************************************
// PageSetupDialog
/** Base class for platform page setup dialog. */
//************************************************************************************************

class PageSetupDialog: public Object,
					   public IPageSetupDialog
{
public:
	DECLARE_CLASS (PageSetupDialog, Object)

	PageSetupDialog () {}

	// IPageSetupDialog
	tbool CCL_API run (PageSetup& pageSetup, IWindow* window) override;

	CLASS_INTERFACE (IPageSetupDialog, Object)
};

//************************************************************************************************
// PrintJob
/** Base class for platform print job. */
//************************************************************************************************

class PrintJob: public Object,
			    public IPrintJob
{
public:
	DECLARE_CLASS (PrintJob, Object)

	PrintJob () {}

	// IPrintJob
	tresult CCL_API run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window) override;

	CLASS_INTERFACE (IPrintJob, Object)
};

} // namespace CCL

#endif // _ccl_printservice_h
