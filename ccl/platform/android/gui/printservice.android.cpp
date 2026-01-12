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
// Filename    : ccl/platform/android/gui/printservice.android.cpp
// Description : Android Print service
//
//************************************************************************************************

#include "ccl/gui/graphics/printservice.h"
#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/platform/android/graphics/frameworkgraphics.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/math/mathprimitives.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// PrintPageRenderer
//************************************************************************************************

DECLARE_JNI_CLASS (PrintPageRenderer, CCLGUI_CLASS_PREFIX "PrintPageRenderer")
	DECLARE_JNI_CONSTRUCTOR (construct, JniIntPtr, jstring, int, int)
	DECLARE_JNI_METHOD (void, run, int, int)
	DECLARE_JNI_METHOD (bool, writePdf, jstring, int, int)
END_DECLARE_JNI_CLASS (PrintPageRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_JNI_CLASS (PrintPageRenderer)
	DEFINE_JNI_CONSTRUCTOR (construct, "(JLjava/lang/String;II)V")
	DEFINE_JNI_METHOD (run, "(II)V")
	DEFINE_JNI_METHOD (writePdf, "(Ljava/lang/String;II)Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

namespace CCL {

//************************************************************************************************
// AndroidPrintService
//************************************************************************************************

class AndroidPrintService: public PrintService
{
public:
	// IPrintService
	IPrintJob* CCL_API createPrintJob () override;
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) override;
	IPageSetupDialog* CCL_API createPageSetupDialog () override;
	Features CCL_API getSupportedFeatures () const override;
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override;
};

//************************************************************************************************
// AndroidPrintJob
//************************************************************************************************

class AndroidPrintJob: public PrintJob,
					   public JniCast<AndroidPrintJob>
{
public:
	DECLARE_CLASS (AndroidPrintJob, PrintJob)

	AndroidPrintJob ();

	PROPERTY_OBJECT (Url, pdfUrl, PdfUrl)

	// IPrintJob
	tresult CCL_API run (const PrinterDocumentInfo& doc, IPageRenderer* renderer, JobMode mode, IWindow* window) override;
};

//************************************************************************************************
// AndroidPageRenderer
//************************************************************************************************

class AndroidPageRenderer: public Object,
						   public JniCast<AndroidPageRenderer>
{
public:
	AndroidPageRenderer (IPageRenderer* renderer);

	void setLayout (int pageW, int pageH, int contentLeft, int contentTop, int contentRight, int contentBottom);
	void drawPage (FrameworkGraphics& graphics, int page);
	void onFinish (IPageRenderer::PrintJobStatus status);

	// convert between millimeters and PostScript points (1/72nd of an inch)
	static int mmToPoints (CoordF mm) { return (int) Math::millimeterToCoord (mm, kPointsPerInch); }
	static CoordF pointsToMm (int points) { return Math::inchToMillimeter (points / kPointsPerInch); }

	// convert between millimeters and mils (1000th of an inch)
	static int mmToMils (CoordF mm) { return (int) Math::millimeterToCoord (mm, kMilsPerInch); }
	static CoordF milsToMm (int mils) { return Math::inchToMillimeter (mils / kMilsPerInch); }

private:
	SharedPtr<IPageRenderer> renderer;
	PointF pageSize;
	RectF printableArea;

	static constexpr float kPointsPerInch = 72.f;
	static constexpr float kMilsPerInch = 1000.f;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AndroidPrintService
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (PrintService, AndroidPrintService)

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API AndroidPrintService::createPrintJob ()
{
	return NEW AndroidPrintJob;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidPrintService::getDefaultPrinterInfo (PrinterInfo& info)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPageSetupDialog* CCL_API AndroidPrintService::createPageSetupDialog ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintService::Features CCL_API AndroidPrintService::getSupportedFeatures () const
{
	return kPrinting|kPdfCreation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API AndroidPrintService::createPdfPrintJob (UrlRef path)
{
	AndroidPrintJob* job = NEW AndroidPrintJob;
	job->setPdfUrl (path);
	return job;
}

//************************************************************************************************
// AndroidPrintJob
//************************************************************************************************

DEFINE_CLASS (AndroidPrintJob, PrintJob)

AndroidPrintJob::AndroidPrintJob ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidPrintJob::run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	if(!renderer)
		return kResultInvalidPointer;

	if(mode == kJobModeSilent && pdfUrl.isEmpty ())
		return kResultNotImplemented;

	String name (docInfo.name);
	if(name.isEmpty ())
		name = CCLSTR ("printJob");

	JniCCLString jobName (name);
	AndroidPageRenderer* pageRenderer = NEW AndroidPageRenderer (renderer);

	JniAccessor jni;
	JniObject javaRenderer (jni, jni.newObject (PrintPageRenderer, PrintPageRenderer.construct, pageRenderer->asIntPtr (), jobName.getString (), docInfo.minPage, docInfo.maxPage));

	if(!pdfUrl.isEmpty ())
	{
		Url folder (pdfUrl);
		folder.ascend ();
		if(!File (folder).exists ())
			File (folder).create ();

		String urlString;
		if(pdfUrl.isNativePath ())
			urlString = NativePath (pdfUrl);
		else
			pdfUrl.getUrl (urlString, true);

		JniCCLString uriString (urlString);
		Point paperSize (AndroidPageRenderer::mmToPoints (docInfo.pageSize.x), AndroidPageRenderer::mmToPoints (docInfo.pageSize.y));

		bool success = PrintPageRenderer.writePdf (javaRenderer, uriString, paperSize.x, paperSize.y);
		if(!success)
			return kResultFailed;
	}
	else
	{
		Point paperSize (AndroidPageRenderer::mmToMils (docInfo.pageSize.x), AndroidPageRenderer::mmToMils (docInfo.pageSize.y));
		PrintPageRenderer.run (javaRenderer, paperSize.x, paperSize.y);
	}

	return kResultOk;
}

//************************************************************************************************
// AndroidPageRenderer
//************************************************************************************************

AndroidPageRenderer::AndroidPageRenderer (IPageRenderer* renderer)
: renderer (renderer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidPageRenderer::setLayout (int pageW, int pageH, int contentLeft, int contentTop, int contentRight, int contentBottom)
{
	pageSize (pointsToMm (pageW), pointsToMm (pageH));
	printableArea (pointsToMm (contentLeft), pointsToMm (contentTop), pointsToMm (contentRight), pointsToMm (contentBottom));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidPageRenderer::drawPage (FrameworkGraphics& graphics, int page)
{
	renderer->updateStatus (IPageRenderer::kPrinting);

	GraphicsDevice graphicsDevice;
	graphicsDevice.setNativeDevice (&graphics);

	IPageRenderer::PageRenderData data = { graphicsDevice, page, kPointsPerInch, pageSize, printableArea };
	renderer->renderPage (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidPageRenderer::onFinish (IPageRenderer::PrintJobStatus status)
{
	renderer->updateStatus (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, PrintPageRenderer, setLayout, JniIntPtr nativeRenderer, int pageW, int pageH, int contentLeft, int contentTop, int contentRight, int contentBottom)
{
	if(AndroidPageRenderer* pageRenderer = AndroidPageRenderer::fromIntPtr (nativeRenderer))
		pageRenderer->setLayout (pageW, pageH, contentLeft, contentTop, contentRight, contentBottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, PrintPageRenderer, drawPage, JniIntPtr nativeRenderer, jobject javaGraphics, int page)
{
	if(AndroidPageRenderer* pageRenderer = AndroidPageRenderer::fromIntPtr (nativeRenderer))
	{
		AutoPtr<FrameworkGraphics> graphics (NEW FrameworkGraphics (env, javaGraphics));
		pageRenderer->drawPage (*graphics, page);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, PrintPageRenderer, finishNative, JniIntPtr nativeRenderer, bool canceled)
{
	if(AndroidPageRenderer* pageRenderer = AndroidPageRenderer::fromIntPtr (nativeRenderer))
	{
		pageRenderer->onFinish (canceled ? IPageRenderer::kCanceled : IPageRenderer::kFinished);
		pageRenderer->release ();
	}
}
