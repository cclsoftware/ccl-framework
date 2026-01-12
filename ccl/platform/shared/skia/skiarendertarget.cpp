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
// Filename    : ccl/platform/shared/skia/skiarendertarget.cpp
// Description : Skia Engine
//
//************************************************************************************************

#include "skiarendertarget.h"

using namespace CCL;

//************************************************************************************************
// SkiaRenderTarget
//************************************************************************************************

SkCanvas* SkiaRenderTarget::getCanvas ()
{
	return surface ? surface->getCanvas () : nullptr;
}

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaWindowRenderTarget, NativeWindowRenderTarget)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaWindowRenderTarget::SkiaWindowRenderTarget (Window& window)
: NativeWindowRenderTarget (window),
  size (window.getSize ().getSize (), window.getContentScaleFactor ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkiaWindowRenderTarget::getContentScaleFactor () const
{
	return window.getContentScaleFactor ();
}

//************************************************************************************************
// SkiaPDFRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaPDFRenderTarget, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPDFRenderTarget::SkiaPDFRenderTarget (IStream& stream, float width, float height)
: canvas (nullptr),
  width (width),
  height (height)
{
	pdfMetaData.fRasterDPI = 600.;
	writer = NEW Writer (stream);
	document = SkPDF::MakeDocument (writer, pdfMetaData);
	if(document)
		canvas = document->beginPage (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPDFRenderTarget::~SkiaPDFRenderTarget ()
{
	if(document)
	{
		document->endPage ();
		document->close ();
	}
	delete writer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaPDFRenderTarget::nextPage ()
{
	document->endPage ();
	canvas = document->beginPage (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkiaPDFRenderTarget::getCanvas ()
{
	return canvas;
}
