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
// Filename    : ccl/platform/shared/skia/skiaprintservice.cpp
// Description : Shared Skia-based print job implementation
//
//************************************************************************************************

#include "skiaprintservice.h"
#include "skiarendertarget.h"
#include "skiadevice.h"

using namespace CCL;

//************************************************************************************************
// SkiaPrintJob
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaPrintJob, PrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPrintJob::SkiaPrintJob (float defaultDpi)
: defaultDpi (defaultDpi)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaPrintJob::getPageSize (PointF& pageSize, const PrinterDocumentInfo& documentInfo, SymbolicPaperFormat defaultFormat)
{
	pageSize = documentInfo.pageSize;
	if(pageSize.isNull ())
	{
		const PaperFormat& format = PrintService::instance ().getPaperFormat (defaultFormat);
		if(format.isValid ())
			pageSize = format.size;
		else
			return false;
	}
	return !pageSize.isNull ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaPrintJob::renderSinglePageToStream (IStream& stream, IPageRenderer* renderer, int pageNumber, const PointF& pageSize, const RectF& printableArea, float dpi)
{
	PointF pixelSize;
	pixelSize.x = Math::millimeterToInch (pageSize.x) * dpi;
	pixelSize.y = Math::millimeterToInch (pageSize.y) * dpi;

	if(pixelSize.isNull ())
		return false;

	SkiaPDFRenderTarget renderTarget (stream, pixelSize.x, pixelSize.y);
	if(renderTarget.getCanvas () == nullptr)
		return false;

	SkiaScopedGraphicsDevice nativeDevice (renderTarget, *renderTarget.asUnknown ());
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);

	IPageRenderer::PageRenderData data = {device, pageNumber, dpi, pageSize, printableArea};
	return renderer->renderPage (data) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaPrintJob::renderDocumentToStream (IStream& stream, IPageRenderer* renderer, const PrinterDocumentInfo& documentInfo)
{
	PointF pageSize;
	if(!getPageSize (pageSize, documentInfo))
		return false;

	PointF pixelSize;
	pixelSize.x = Math::millimeterToInch (pageSize.x) * defaultDpi;
	pixelSize.y = Math::millimeterToInch (pageSize.y) * defaultDpi;

	if(pixelSize.isNull ())
		return false;

	SkiaPDFRenderTarget renderTarget (stream, pixelSize.x, pixelSize.y);
	if(renderTarget.getCanvas () == nullptr)
		return false;

	SkiaScopedGraphicsDevice nativeDevice (renderTarget, *renderTarget.asUnknown ());
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);

	int minPage = documentInfo.hasValidPageRange () ? documentInfo.minPage : 0;
	int maxPage = documentInfo.hasValidPageRange () ? documentInfo.maxPage : 0;

	for(int pageNumber = minPage; pageNumber <= maxPage; pageNumber++)
	{
		IPageRenderer::PageRenderData data = {device, pageNumber, defaultDpi, pageSize, pageSize};
		if(renderer->renderPage (data) != kResultOk)
			return false;

		if(pageNumber < maxPage)
			renderTarget.nextPage ();
	}

	return true;
}
