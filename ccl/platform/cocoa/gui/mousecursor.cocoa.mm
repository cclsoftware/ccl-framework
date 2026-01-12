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
// Filename    : ccl/platform/cocoa/gui/mousecursor.cocoa.mm
// Description : Platform-specific Cursor implementation
//
//************************************************************************************************

#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/theme/theme.h"

#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// CocoaMouseCursor
//************************************************************************************************

class CocoaMouseCursor: public MouseCursor
{
public:
	CocoaMouseCursor (NSCursor* nativeCursor, bool ownCursor);
	~CocoaMouseCursor ();

	// MouseCursor
	void makeCurrent ();

private:
	NSCursor* nativeCursor;
};

//************************************************************************************************
// CocoaCursorFactory
//************************************************************************************************

class CocoaCursorFactory: public MouseCursorFactory
{
public:
	// MouseCursorFactory
	MouseCursor* createCursor (int themeCursorId);
	MouseCursor* createCursor (Image& image, PointRef hotspot);
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (CocoaMouseCursor, kFrameworkLevelFirst)
{
	static CocoaCursorFactory theFactory;
	MouseCursor::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// CocoaCursorFactory
//************************************************************************************************

MouseCursor* CocoaCursorFactory::createCursor (Image& image, PointRef hotspot)
{
	SharedPtr<CCL::Bitmap> bitmap = ccl_cast<CCL::Bitmap>(&image);
	if(!bitmap)
	{
		image.setCurrentFrame (0); // always draw first frame to get a consistent result
		BitmapProcessor processor;
		const Point size (image.getSize ());
		processor.setup (&image, Colors::kWhite, 0, &size);
		BitmapFilterList copyFilter; // no filtering, just copy
		processor.process (static_cast<IBitmapFilterList&> (copyFilter));
		bitmap = unknown_cast<Bitmap> (processor.getOutput ());
	}
	
	ASSERT (bitmap != nullptr)
	NSImage* nsImage = MacOS::nsImageFromBitmap (*bitmap);
	if(!nsImage)
		return nullptr;
		
	NSCursor *cursor = [[NSCursor alloc] initWithImage:nsImage hotSpot:NSMakePoint (hotspot.x, hotspot.y)];
	[nsImage release];

	return NEW CocoaMouseCursor (cursor, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* CocoaCursorFactory::createCursor (int themeCursorId)
{
	ASSERT (themeCursorId >= 0 && themeCursorId < ThemeElements::kNumCursors)

	NSCursor* nativeCursor = nil;
	switch(themeCursorId)
	{
	case ThemeElements::kArrowCursor :
		nativeCursor = [NSCursor arrowCursor];
		break;
	case ThemeElements::kWaitCursor :
		nativeCursor = [NSCursor arrowCursor];
		break;
	case ThemeElements::kCrosshairCursor :
		nativeCursor = [NSCursor crosshairCursor];
		break;
	case ThemeElements::kPointhandCursor :
		nativeCursor = [NSCursor pointingHandCursor];
		break;
	case ThemeElements::kSizeHorizontalCursor :
		nativeCursor = [NSCursor resizeLeftRightCursor];
		break;
	case ThemeElements::kSizeVerticalCursor :
		nativeCursor = [NSCursor resizeUpDownCursor];
		break;
	case ThemeElements::kSizeUpCursor :
		nativeCursor = [NSCursor resizeUpCursor];
		break;
	case ThemeElements::kSizeRightCursor :
		nativeCursor = [NSCursor resizeRightCursor];
		break;
	case ThemeElements::kSizeDownCursor :
		nativeCursor = [NSCursor resizeDownCursor];
		break;
	case ThemeElements::kSizeLeftCursor :
		nativeCursor = [NSCursor resizeLeftCursor];
		break;
	// properties starting with _ are Apple private API, this can break anytime, public API available since macOS 15
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wobjc-method-access"
	case ThemeElements::kSizeLeftUpCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionTopLeft inDirections:NSCursorFrameResizeDirectionsOutward];
		else
			nativeCursor = [NSCursor _windowResizeNorthWestCursor];
		break;
	case ThemeElements::kSizeLeftDownCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionBottomLeft inDirections:NSCursorFrameResizeDirectionsOutward];
		else
			nativeCursor = [NSCursor _windowResizeSouthWestCursor];
		break;
	case ThemeElements::kSizeRightUpCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionTopRight inDirections:NSCursorFrameResizeDirectionsOutward];
		else
			nativeCursor = [NSCursor _windowResizeNorthEastCursor];
		break;
	case ThemeElements::kSizeRightDownCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionBottomRight inDirections:NSCursorFrameResizeDirectionsOutward];
		else
		nativeCursor = [NSCursor _windowResizeSouthEastCursor];
		break;
	case ThemeElements::kSizeLeftUpRightDownCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionTopLeft inDirections:NSCursorFrameResizeDirectionsAll];
		else
			nativeCursor = [NSCursor _windowResizeNorthWestSouthEastCursor];
		break;
	case ThemeElements::kSizeLeftDownRightUpCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionBottomLeft inDirections:NSCursorFrameResizeDirectionsAll];
		else
			nativeCursor = [NSCursor _windowResizeNorthEastSouthWestCursor];
		break;
	case ThemeElements::kZoomInCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor zoomInCursor];
		else
			nativeCursor = [NSCursor _zoomInCursor];
		break;
	case ThemeElements::kZoomOutCursor :
		if(@available (macOS 15, *))
			nativeCursor = [NSCursor zoomOutCursor];
		else
			nativeCursor = [NSCursor _zoomOutCursor];
		break;
	#pragma clang diagnostic pop
	case ThemeElements::kTextCursor :
		nativeCursor = [NSCursor IBeamCursor];
		break;
	case ThemeElements::kCopyCursor :
		nativeCursor = [NSCursor dragCopyCursor];
		break;
	case ThemeElements::kNoDropCursor :
		nativeCursor = [NSCursor operationNotAllowedCursor];
		break;
	case ThemeElements::kGrabCursor :
		nativeCursor = [NSCursor openHandCursor];
		break;
	case ThemeElements::kGrabbingCursor :
		nativeCursor = [NSCursor closedHandCursor];
		break;
	}
	if(nativeCursor)
		[nativeCursor retain];
		
	return NEW CocoaMouseCursor (nativeCursor, true);
}

//************************************************************************************************
// CocoaMouseCursor
//************************************************************************************************

CocoaMouseCursor::CocoaMouseCursor (NSCursor* nativeCursor, bool ownCursor)
: MouseCursor (ownCursor),
  nativeCursor (nativeCursor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaMouseCursor::~CocoaMouseCursor ()
{
	if(ownCursor)
		[nativeCursor release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaMouseCursor::makeCurrent ()
{
	[nativeCursor set];
}
