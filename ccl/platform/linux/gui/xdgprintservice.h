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
// Filename    : ccl/platform/linux/gui/xdgprintservice.h
// Description : Print Service implementation using XDG Desktop Portal
//
//************************************************************************************************

#ifndef _ccl_xdgprintservice_h
#define _ccl_xdgprintservice_h

#include "ccl/gui/graphics/printservice.h"

#include "ccl/platform/linux/interfaces/idbussupport.h"

namespace CCL {
namespace Linux {

typedef std::map<std::string, sdbus::Variant> XdgPrintSettings;

//************************************************************************************************
// XdgPrintService
//************************************************************************************************

class XdgPrintService: public PrintService
{
public:
	XdgPrintService ();

	static XdgPrintService& instance () { return static_cast<XdgPrintService&> (PrintService::instance ()); }

	PROPERTY_POINTER (IDBusSupport, dbusSupport, DBusSupport)
	XdgPrintSettings& getSettings () { return settings; }
	XdgPrintSettings& getSetup () { return setup; }

	// PrintService
	IPrintJob* CCL_API createPrintJob () override;
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override;
	Features CCL_API getSupportedFeatures () const override {return kPrinting|kPdfCreation;}
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) override { return kResultFailed; }
	IPageSetupDialog* CCL_API createPageSetupDialog () override;

private:
	XdgPrintSettings settings;
	XdgPrintSettings setup;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_xdgprintservice_h
