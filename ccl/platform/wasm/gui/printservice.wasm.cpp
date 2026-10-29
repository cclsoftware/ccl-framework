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
// Filename    : ccl/platform/wasm/gui/printservice.wasm.cpp
// Description : WebAssembly Print service
//
//************************************************************************************************

#include "ccl/gui/graphics/printservice.h"
#include "ccl/platform/shared/skia/skiaprintservice.h"

#include "ccl/public/base/memorystream.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, openPdfInNewTab, (const uint8* data, int size),
{
	try
	{
		const bytes = new Uint8Array (size);
		bytes.set (HEAPU8.subarray (data, data + size));
		const blob = new Blob ([bytes], {type: 'application/pdf'});
		CCL.openBlobInNewTab (blob);
	}
	catch(e)
	{}
});

//************************************************************************************************
// WasmPrintJob
//************************************************************************************************

class WasmPrintJob: public SkiaPrintJob
{
public:
	DECLARE_CLASS (WasmPrintJob, SkiaPrintJob)

	// PrintJob
	tresult CCL_API run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window) override;
};

} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmPrintService
//************************************************************************************************

class WasmPrintService: public PrintService
{
public:
	// IPrintService
	IPrintJob* CCL_API createPrintJob () override { return NEW WasmPrintJob; }
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) override;
	IPageSetupDialog* CCL_API createPageSetupDialog () override { return nullptr; }
	Features CCL_API getSupportedFeatures () const override { return kPrinting | kPdfCreation; }
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override { return NEW WasmPrintJob; }
};

//************************************************************************************************
// WasmPrintService
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (PrintService, WasmPrintService)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmPrintService::getDefaultPrinterInfo (PrinterInfo& info)
{
	info.name = "PDF";
	info.orientation = kPageOrientationPortrait;
	info.paperFormat = kPaperFormatA4;
	return kResultOk;
}

//************************************************************************************************
// WasmPrintJob
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WasmPrintJob, SkiaPrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmPrintJob::run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	if(docInfo.isValid () == false)
		return kResultInvalidArgument;

	if(renderer == nullptr)
		return kResultInvalidArgument;

	PrintService::instance ().onPrintJobStarted ();
	renderer->updateStatus (IPageRenderer::kPrinting);

	MemoryStream stream;
	bool succeeded = renderDocumentToStream (stream, renderer, docInfo);
	if(succeeded)
	{
		const uint8* data = static_cast<const uint8*> (stream.getMemoryAddress ());
		uint32 size = stream.getBytesWritten ();
		if(data && size > 0)
		{
			openPdfInNewTab (data, size);
			renderer->updateStatus (IPageRenderer::kFinished);
		}
		else
		{
			renderer->updateStatus (IPageRenderer::kFailed);
			succeeded = false;
		}
	}
	else
		renderer->updateStatus (IPageRenderer::kFailed);

	PrintService::instance ().onPrintJobDone ();
	return succeeded ? kResultOk : kResultFailed;
}
