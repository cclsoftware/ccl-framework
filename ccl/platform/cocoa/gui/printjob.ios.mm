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
// Filename    : ccl/platform/cocoa/gui/printjob.ios.mm
// Description : platform-specific print code
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/printjob.ios.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/base/memorystream.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/gui/printservice.ios.h"

#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/shared/skia/skiarendertarget.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace MacOS;

//////////////////////////////////////////////////////////////////////////////////////////////////

CGFloat CoordHelper::convertFromMM (CoordF l)
{
	return Math::millimeterToInch (l) * kDpi;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

CoordF CoordHelper::convertToMM (CGFloat l)
{
	return (CoordF)(Math::inchToMillimeter (l) / kDpi);
}

//************************************************************************************************
// PrintPageRenderer
//************************************************************************************************

@interface CCL_ISOLATED (PrintPageRenderer) : UIPrintPageRenderer
{
	IPageRenderer* renderer;
	PrinterDocumentInfo docInfo;
	BOOL printingStarted;
}

- (id)initWithPrintJobData:(const PrinterDocumentInfo*)_docInfo andRenderer:(IPageRenderer*)_renderer;
- (BOOL)printingDidStart;
- (void)getSize:(PointF&)pageSize andArea:(RectF&)printableArea;

@end

//************************************************************************************************
// QuartzPrintPageRenderer
//************************************************************************************************

@interface CCL_ISOLATED (QuartzPrintPageRenderer) : CCL_ISOLATED (PrintPageRenderer)
- (void)drawPageAtIndex:(NSInteger)page inRect:(CGRect)rect;
@end

//************************************************************************************************
// SkiaPrintPageRenderer
//************************************************************************************************

@interface CCL_ISOLATED (SkiaPrintPageRenderer) : CCL_ISOLATED (PrintPageRenderer)
- (void)drawPageAtIndex:(NSInteger)page inRect:(CGRect)rect;
@end

//************************************************************************************************
// PrintPageRenderer
//************************************************************************************************

@implementation CCL_ISOLATED (PrintPageRenderer)

- (id)initWithPrintJobData:(const PrinterDocumentInfo*)_docInfo andRenderer:(IPageRenderer*)_renderer
{
	if(!_docInfo || !_renderer)
		return nil;
	
	if(self = [super init])
	{
		docInfo = *_docInfo;
		renderer = _renderer;
		printingStarted = NO;
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSInteger)numberOfPages
{
	return docInfo.maxPage - docInfo.minPage + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CGFloat)headerHeight
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CGFloat)footerHeight
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)printingDidStart
{
	return printingStarted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)getSize:(PointF&)pageSize andArea:(RectF&)printableArea
{
	if(docInfo.pageSize.x > 0 && docInfo.pageSize.y > 0)
	{
		pageSize = docInfo.pageSize;
		printableArea = RectF (pageSize);
	}
	else
	{
		CGSize size = self.paperRect.size;
		CGRect area = self.printableRect;
		pageSize = PointF (CoordHelper::convertToMM (size.width), CoordHelper::convertToMM (size.height));
		printableArea = RectF (CoordHelper::convertToMM (area.origin.x), CoordHelper::convertToMM (size.height - area.size.height - area.origin.y), PointF (CoordHelper::convertToMM (area.size.width), CoordHelper::convertToMM (area.size.height)));
	}
}

@end

//************************************************************************************************
// QuartzPrintPageRenderer
//************************************************************************************************

@implementation CCL_ISOLATED (QuartzPrintPageRenderer)

- (void)drawPageAtIndex:(NSInteger)pageNo inRect:(CGRect)rect
{
	if(!renderer)
		return;

	UIPrintInteractionController* printController = [UIPrintInteractionController sharedPrintController];
	UIPrintInfo* printInfo = [printController printInfo];
	if(!printingStarted && printInfo.printerID.length > 0) // printerID appears to be empty while previewing
	{
		renderer->updateStatus (IPageRenderer::kPrinting);
		printingStarted = YES;
	}

	QuartzLayerRenderTarget renderTarget (UIGraphicsGetCurrentContext (), 2.);
	QuartzScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);

	PointF pageSize;
	RectF printableArea;
	[self getSize:pageSize andArea:printableArea];

	IPageRenderer::PageRenderData data = {device, static_cast<int> (pageNo), CoordHelper::kDpi, pageSize, printableArea};
	renderer->renderPage (data);
}

@end

//************************************************************************************************
// SkiaPrintPageRenderer
//************************************************************************************************

@implementation CCL_ISOLATED (SkiaPrintPageRenderer)

- (void)drawPageAtIndex:(NSInteger)pageNo inRect:(CGRect)rect
{
	if(!renderer)
		return;

	UIPrintInteractionController* printController = [UIPrintInteractionController sharedPrintController];
	UIPrintInfo* printInfo = [printController printInfo];
	if(!printingStarted && printInfo.printerID.length > 0) // printerID appears to be empty while previewing
	{
		renderer->updateStatus (IPageRenderer::kPrinting);
		printingStarted = YES;
	}
	
	PointF pageSize;
	RectF printableArea;
	[self getSize:pageSize andArea:printableArea];
	
	MemoryStream stream;
	{
		SkiaPDFRenderTarget renderTarget (stream, static_cast<float> (CoordHelper::convertFromMM (pageSize.x)), static_cast<float> (CoordHelper::convertFromMM (pageSize.y)));
		SkiaScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
		GraphicsDevice device;
		device.setNativeDevice (&nativeDevice);

		Transform newMatrix;
		newMatrix.translate (0, static_cast<float> (CoordHelper::convertFromMM (pageSize.y)));
		newMatrix.scale (1, -1);
		nativeDevice.addTransform (newMatrix);
				
		IPageRenderer::PageRenderData data = {device, static_cast<int> (pageNo), CoordHelper::kDpi, pageSize, printableArea};
		renderer->renderPage (data);
	}
	
	CGDataProviderRef provider = CGDataProviderCreateWithData (NULL, stream.getMemoryAddress (), stream.getBytesWritten (), NULL);
	CGPDFDocumentRef quartzPDF = CGPDFDocumentCreateWithProvider (provider);
	CGPDFPageRef page = CGPDFDocumentGetPage (quartzPDF, 1);
	CGContextDrawPDFPage (UIGraphicsGetCurrentContext (), page);
	CGPDFDocumentRelease (quartzPDF);
	CFRelease (provider);
}

@end

//************************************************************************************************
// IOSPrintJob
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (IOSPrintJob, PrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IOSPrintJob::run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	if(!renderer)
		return kResultInvalidPointer;
	
	if(!pdfUrl.isEmpty ())
		return exportPdf (docInfo, renderer);
	
	if(UIPrintInteractionController.printingAvailable == NO)
		return kResultFailed;
	
	if(mode == kJobModeSilent)
		return kResultNotImplemented; // could be done with printToPrinter:completionHandler:, but need UIPrinter object to print to
	
	NSString* jobString = [docInfo.name.createNativeString<NSString*> () autorelease];
	UIPrintInfo* printInfo = [UIPrintInfo printInfo];
	printInfo.jobName = jobString;

	UIPrintInteractionController* printController = [UIPrintInteractionController sharedPrintController];
	printController.printInfo = printInfo;
	CCL_ISOLATED (PrintPageRenderer)* pageRenderer = [getPrintRenderer (&docInfo, renderer) autorelease];
	printController.printPageRenderer = pageRenderer;

	renderer->retain ();
	BOOL success = [printController presentAnimated:YES completionHandler:^(UIPrintInteractionController*, BOOL completed, NSError*)
		{
			if([pageRenderer printingDidStart])
			{
				if(completed)
					renderer->updateStatus (IPageRenderer::kFinished);
				else
					renderer->updateStatus (IPageRenderer::kCanceled);
			}
			renderer->release ();
		}];
	
	return success ? kResultOk : kResultAborted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult IOSPrintJob::exportPdf (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer)
{
	CCL_ISOLATED (PrintPageRenderer)* pageRenderer = [getPrintRenderer (&docInfo, renderer) autorelease];
	
	CGRect paperSize = CGRectZero;
	if(docInfo.pageSize.x > 0 && docInfo.pageSize.y > 0)
		paperSize = CGRectMake (0, 0, CoordHelper::convertFromMM (docInfo.pageSize.x), CoordHelper::convertFromMM (docInfo.pageSize.y));

	NSString* path = [pdfUrl.getPath ().createNativeString<NSString*> () autorelease];
	@try
	{
		if(!UIGraphicsBeginPDFContextToFile (path, paperSize, NULL))
			return kResultFailed;

		for(int i = docInfo.minPage; i <= docInfo.maxPage; i++)
		{
			UIGraphicsBeginPDFPage ();
			[pageRenderer drawPageAtIndex:i inRect:UIGraphicsGetPDFContextBounds ()];
		}

		UIGraphicsEndPDFContext ();

		return kResultOk;
	}
	@catch (NSException* e)
	{
		return kResultFailed;
	}
}

//************************************************************************************************
// IOSQuartzPrintJob
//************************************************************************************************

DEFINE_CLASS (IOSQuartzPrintJob, IOSPrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (PrintPageRenderer)* IOSQuartzPrintJob::getPrintRenderer (const PrinterDocumentInfo* docInfo, IPageRenderer* renderer)
{
	return [[CCL_ISOLATED (QuartzPrintPageRenderer) alloc] initWithPrintJobData:docInfo andRenderer:renderer];
}

//************************************************************************************************
// IOSSkiaPrintJob
//************************************************************************************************

DEFINE_CLASS (IOSSkiaPrintJob, IOSPrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (PrintPageRenderer)* IOSSkiaPrintJob::getPrintRenderer (const PrinterDocumentInfo* docInfo, IPageRenderer* renderer)
{
	return [[CCL_ISOLATED (SkiaPrintPageRenderer) alloc] initWithPrintJobData:docInfo andRenderer:renderer];
}
