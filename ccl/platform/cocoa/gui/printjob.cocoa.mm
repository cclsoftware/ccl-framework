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
// Filename    : ccl/platform/cocoa/gui/printjob.cocoa.mm
// Description : platform-specific print code
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/printjob.cocoa.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/system/ifilemanager.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/gui/printservice.cocoa.h"

#include "ccl/platform/cocoa/macutils.h"
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
// PrintView
//************************************************************************************************

@interface CCL_ISOLATED (PrintView) : NSView
{
	IPageRenderer* renderer;
	MacOSPrintJobData* jobData;
	BOOL printingStarted;
}

- (id)initWithPrintJobData:(MacOSPrintJobData*)jobData;
- (void)setRenderer:(IPageRenderer*)renderer;
- (void)getPageSize:(CGSize&)size printableArea:(CGRect&)rect;
- (BOOL)printingDidStart;

@end

//************************************************************************************************
// QuartzPrintView
//************************************************************************************************

@interface CCL_ISOLATED (QuartzPrintView) : CCL_ISOLATED (PrintView)
- (void)drawRect:(NSRect)rect;
@end

//************************************************************************************************
// SkiaPrintView
//************************************************************************************************

@interface CCL_ISOLATED (SkiaPrintView) : CCL_ISOLATED (PrintView)
- (void)drawRect:(NSRect)rect;
@end

//************************************************************************************************
// PrintView
//************************************************************************************************

@implementation CCL_ISOLATED (PrintView)

- (id)initWithPrintJobData:(MacOSPrintJobData*)_jobData
{
	if(!_jobData)
		return nil;
	if(self = [super initWithFrame:NSMakeRect (0, 0, CGFLOAT_MAX, CGFLOAT_MAX)])
	{
		jobData = _jobData;
		printingStarted = FALSE;
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setRenderer:(IPageRenderer*)_renderer
{
	renderer = _renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)knowsPageRange:(NSRangePointer)range
{
	range->location = jobData->pageRange.pageFrom;
	range->length = jobData->pageRange.pageTo - jobData->pageRange.pageFrom + 1;

    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRect)rectForPage:(NSInteger)page
{
	NSRange printRange;
	printRange.location = jobData->pageRange.pageFrom;
	printRange.length = jobData->pageRange.pageTo - jobData->pageRange.pageFrom + 1;
	if(!NSLocationInRange (page, printRange))
		return NSZeroRect;
	
	CGSize size;
	CGRect area;
	[self getPageSize:size printableArea:area];
	return NSMakeRect (0, (page - 1) * size.height, size.width, size.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)getPageSize:(CGSize&)size printableArea:(CGRect&)area
{
    if(NSPrintInfo* printInfo = [[NSPrintOperation currentOperation] printInfo])
    {
    	size = NSSizeToCGSize ([printInfo paperSize]);
    	area = NSRectToCGRect ([printInfo imageablePageBounds]);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)printingDidStart
{
	return printingStarted;
}

@end

//************************************************************************************************
// CCLQuartzPrintView
//************************************************************************************************

@implementation CCL_ISOLATED (QuartzPrintView)

- (void)drawRect:(NSRect)rect
{
	if(!renderer)
		return;
	
    if(!printingStarted && [[NSPrintOperation currentOperation] preferredRenderingQuality] == NSPrintRenderingQualityBest)
    {
    	renderer->updateStatus (IPageRenderer::kPrinting);
    	printingStarted = YES;
	}
	
	QuartzLayerRenderTarget renderTarget ((CGContextRef)[[NSGraphicsContext currentContext] CGContext], 2.); // TODO what shoud the contentScale factor be?
	QuartzScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);
	
	CGSize size;
	CGRect area;
	[self getPageSize:size printableArea:area];
	int pageNumber = (int)ccl_round<0>(rect.origin.y / size.height);

	Transform newMatrix;
	newMatrix.translate (0, (float)((pageNumber + 1) * size.height));
	newMatrix.scale (1, -1);
	nativeDevice.addTransform (newMatrix);

	PointF pageSize (CoordHelper::convertToMM (size.width), CoordHelper::convertToMM (size.height));
	RectF printableArea (CoordHelper::convertToMM (area.origin.x), CoordHelper::convertToMM (size.height - area.size.height - area.origin.y), PointF (CoordHelper::convertToMM (area.size.width), CoordHelper::convertToMM (area.size.height)));
	IPageRenderer::PageRenderData data = {device, pageNumber, CoordHelper::kDpi, pageSize, printableArea};
	renderer->renderPage (data);
}

@end

//************************************************************************************************
// CCLSkiaPrintView
//************************************************************************************************

@implementation CCL_ISOLATED (SkiaPrintView)

- (void)drawRect:(NSRect)rect
{
	if(!renderer)
		return;
	
    if(!printingStarted && [[NSPrintOperation currentOperation] preferredRenderingQuality] == NSPrintRenderingQualityBest)
    {
    	renderer->updateStatus (IPageRenderer::kPrinting);
    	printingStarted = YES;
	}

	CGSize size;
	CGRect area;
	[self getPageSize:size printableArea:area];
	int pageNumber = (int)ccl_round<0>(rect.origin.y / size.height);
	
	MemoryStream stream;
	{
		SkiaPDFRenderTarget renderTarget (stream, static_cast<float> (size.width), static_cast<float> (size.height));
		SkiaScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
		GraphicsDevice device;
		device.setNativeDevice (&nativeDevice);
				
		PointF pageSize (CoordHelper::convertToMM (size.width), CoordHelper::convertToMM (size.height));
		RectF printableArea (CoordHelper::convertToMM (area.origin.x), CoordHelper::convertToMM (size.height - area.size.height - area.origin.y), PointF (CoordHelper::convertToMM (area.size.width), CoordHelper::convertToMM (area.size.height)));
		IPageRenderer::PageRenderData data = {device, pageNumber, CoordHelper::kDpi, pageSize, printableArea};
		renderer->renderPage (data);
	}
	
	CFObj<CGDataProviderRef> provider = CGDataProviderCreateWithData (NULL, stream.getMemoryAddress (), stream.getBytesWritten (), NULL);
	CGPDFDocumentRef quartzPDF = CGPDFDocumentCreateWithProvider (provider);
	CGPDFPageRef page = CGPDFDocumentGetPage (quartzPDF, 1);
	CGContextTranslateCTM ((CGContextRef)[[NSGraphicsContext currentContext] CGContext], 0, (float)(pageNumber * size.height));
	CGContextDrawPDFPage ((CGContextRef)[[NSGraphicsContext currentContext] CGContext], page);
	CGPDFDocumentRelease (quartzPDF);
}

@end

//************************************************************************************************
// MacOSPrintJob
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MacOSPrintJob, PrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MacOSPrintJob::run (const PrinterDocumentInfo& doc, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	AutoPtr<MacOSPrintJobData> jobData = NEW MacOSPrintJobData;
	jobData->pageRange.pageFrom = doc.minPage + 1;
	jobData->pageRange.pageTo = doc.maxPage + 1;

	CCL_ISOLATED (PrintView)* printView = [createPrintView (jobData) autorelease];
	if(!printView)
		return kResultFailed;
	
	NSPrintOperation* printOp = [NSPrintOperation printOperationWithView:printView];
	NSString* jobString = [doc.name.createNativeString<NSString*> () autorelease];
	printOp.jobTitle = jobString;
	
	NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
	if(doc.pageSize.x > 0 && doc.pageSize.y > 0)
	{
		printInfo.orientation = doc.pageSize.x > doc.pageSize.y ? NSPaperOrientationLandscape : NSPaperOrientationPortrait;
		printInfo.paperSize = NSMakeSize (CoordHelper::convertFromMM (doc.pageSize.x), CoordHelper::convertFromMM (doc.pageSize.y));
	}
	
	Url parent = pdfUrl;
	parent.ascend ();
	System::GetFileManager ().setFileUsed (parent, true);

	if(!pdfUrl.isEmpty ())
	{
		NSURL* url = [NSURL alloc];
		MacUtils::urlToNSUrl (pdfUrl, url);
		[printInfo.dictionary setObject:url forKey:NSPrintJobSavingURL];
		printInfo.jobDisposition = NSPrintSaveJob;
	}

	printOp.printInfo = printInfo;
	[printOp setShowsPrintPanel:mode == kJobModeSilent ? NO : YES];

	[printView setRenderer:renderer];
	bool success = [printOp runOperation] == YES;
	System::GetFileManager ().setFileUsed (parent, false);
	if([printView printingDidStart])
	{
		renderer->updateStatus (success ? IPageRenderer::kFinished : IPageRenderer::kCanceled);
		return kResultOk;
	}
	else
	{
		return kResultAborted;
	}
}

//************************************************************************************************
// MacOSQuartzPrintJob
//************************************************************************************************

DEFINE_CLASS (MacOSQuartzPrintJob, MacOSPrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (PrintView)* MacOSQuartzPrintJob::createPrintView (MacOSPrintJobData* jobData)
{
	return [[CCL_ISOLATED (QuartzPrintView) alloc] initWithPrintJobData:jobData];
}

//************************************************************************************************
// MacOSSkiaPrintJob
//************************************************************************************************

DEFINE_CLASS (MacOSSkiaPrintJob, MacOSPrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (PrintView)* MacOSSkiaPrintJob::createPrintView (MacOSPrintJobData* jobData)
{
	return [[CCL_ISOLATED (SkiaPrintView) alloc] initWithPrintJobData:jobData];
}
