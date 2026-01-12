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
// Filename    : ccl/platform/linux/gui/printservice.linux.h
// Description : Linux Print Service
//
//************************************************************************************************

#ifndef _ccl_printservice_linux_h
#define _ccl_printservice_linux_h

#include "ccl/gui/graphics/printservice.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// PrintServiceStub
//************************************************************************************************

class PrintServiceStub: public PrintService
{
public:
	// PrintService
	IPrintJob* CCL_API createPrintJob () override { return nullptr; }
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override { return nullptr; }
	Features CCL_API getSupportedFeatures () const override {return 0;}
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) override { return kResultFailed; }
	IPageSetupDialog* CCL_API createPageSetupDialog () override { return nullptr; }
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_printservice_linux_h
