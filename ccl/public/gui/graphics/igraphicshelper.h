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
// Filename    : ccl/public/gui/graphics/igraphicshelper.h
// Description : Graphics Helper Interface
//
//************************************************************************************************

#ifndef _ccl_igraphicshelper_h
#define _ccl_igraphicshelper_h

#include "ccl/public/cclexports.h"
#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/igraphicspath.h"

namespace CCL {

class FileType;
interface IAttributeList;
interface IBitmapFilter;
interface ITextLayout;
interface IGraphicsLayer;
interface IUIValue;
interface IFontTable;

namespace Internal {

//************************************************************************************************
// Internal::IGraphicsHelper
/** Helper methods for public graphics class implementation. Do not use this interface directly. 
	\ingroup gui_graphics */
//************************************************************************************************

interface IGraphicsHelper: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Color
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum ColorFormatFlags { kColorWithAlpha = 1<<0 };
	virtual tbool CCL_API Color_fromCString (Color& This, CStringPtr cString) = 0;
	virtual tbool CCL_API Color_toCString (const Color& This, char* cString, int cStringSize, int flags) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Font
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual const Font& CCL_API Font_getDefaultFont () = 0;
	virtual void CCL_API Font_measureString (Rect& size, StringRef text, const Font& font, int flags) = 0;
	virtual void CCL_API Font_measureString (RectF& size, StringRef text, const Font& font, int flags) = 0;
	virtual void CCL_API Font_measureStringImage (RectF& size, StringRef text, const Font& font, tbool shiftToBaseline) = 0;
	virtual void CCL_API Font_measureText (Rect& size, Coord lineWidth, StringRef text, const Font& font, TextFormatRef format) = 0;
	virtual void CCL_API Font_measureText (RectF& size, CoordF lineWidth, StringRef text, const Font& font, TextFormatRef format) = 0;
	virtual void CCL_API Font_collapseString (String& string, CoordF maxWidth, const Font& font, int trimMode, tbool exact = false) = 0;
	virtual IFontTable* CCL_API Font_collectFonts (int flags) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Factory
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual int CCL_API Factory_getNumImageFormats () = 0;
	virtual const FileType* CCL_API Factory_getImageFormat (int index) = 0;
	virtual IImage* CCL_API Factory_loadImageFile (UrlRef path) = 0;
	virtual tbool CCL_API Factory_saveImageFile (UrlRef path, IImage* image, const IAttributeList* encoderOptions = nullptr) = 0;
	virtual IImage* CCL_API Factory_loadImageStream (IStream& stream, const FileType& format) = 0;
	virtual tbool CCL_API Factory_saveImageStream (IStream& stream, IImage* image, const FileType& format, const IAttributeList* encoderOptions = nullptr) = 0;
	virtual IImage* CCL_API Factory_createBitmap (int width, int height, IBitmap::PixelFormat format = IBitmap::kRGB, float scaleFactor = 1.f) = 0;
	virtual IGraphics* CCL_API Factory_createBitmapGraphics (IImage* bitmap) = 0;
	virtual IBitmapFilter* CCL_API Factory_createBitmapFilter (StringID which) = 0;
	virtual IGraphicsPath* CCL_API Factory_createPath (IGraphicsPath::TypeHint type) = 0;
	virtual IGradient* CCL_API Factory_createGradient (IGradient::TypeHint type) = 0;
	virtual IImage* CCL_API Factory_createShapeImage () = 0;
	virtual IGraphics* CCL_API Factory_createShapeBuilder (IImage* shapeImage) = 0;
	virtual ITextLayout* CCL_API Factory_createTextLayout () = 0;
	virtual IGraphicsLayer* CCL_API Factory_createGraphicsLayer (UIDRef cid) = 0;
	virtual IUIValue* CCL_API Factory_createValue () = 0;
	virtual IImage* CCL_API Factory_createFilmstrip (IImage* sourceImage, StringID frames) = 0;
	virtual IImage* CCL_API Factory_createImagePart (IImage* sourceImage, RectRef partRect) = 0;
	virtual IImage* CCL_API Factory_createMultiImage (IImage* images[], CString frameNames[], int count) = 0;
	virtual IImage* CCL_API Factory_createMultiResolutionBitmap (IImage* bitmaps[], float scaleFactors[], int count) = 0;
	
	DECLARE_IID (IGraphicsHelper)
};

DEFINE_IID (IGraphicsHelper, 0xf4567fc5, 0x6322, 0x4240, 0x90, 0x7e, 0xb8, 0x8d, 0x78, 0x16, 0x44, 0xcf)

} // namespace Internal

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace System {

/** Get graphics helper singleton (internal) */
CCL_EXPORT Internal::IGraphicsHelper& CCL_API CCL_ISOLATED (GetGraphicsHelper) ();
inline Internal::IGraphicsHelper& GetGraphicsHelper () { return CCL_ISOLATED (GetGraphicsHelper) (); }

} // namespace System
} // namespace CCL

#endif // _ccl_igraphicshelper_h
