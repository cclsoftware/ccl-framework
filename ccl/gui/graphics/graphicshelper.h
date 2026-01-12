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
// Filename    : ccl/gui/graphics/graphicshelper.h
// Description : Graphics Helper
//
//************************************************************************************************

#ifndef _ccl_graphicshelper_h
#define _ccl_graphicshelper_h

#include "ccl/base/object.h"
#include "ccl/base/typelib.h"

#include "ccl/public/gui/graphics/igraphicshelper.h"
#include "ccl/public/gui/graphics/iuivalue.h"

namespace CCL {

//************************************************************************************************
// GraphicsHelper
//************************************************************************************************

class GraphicsHelper: public Object,
					  public Internal::IGraphicsHelper
{
public:
	DECLARE_CLASS_ABSTRACT (GraphicsHelper, Object)
	DECLARE_METHOD_NAMES (GraphicsHelper)

	static GraphicsHelper& instance ();

	EnumTypeInfo& getDefaultColors ();

	// Color
	tbool CCL_API Color_fromCString (Color& This, CStringPtr cString) override;
	tbool CCL_API Color_toCString (const Color& This, char* cString, int cStringSize, int flags) override;
	
	// Font
	const Font& CCL_API Font_getDefaultFont () override;
	void CCL_API Font_measureString (Rect& size, StringRef text, const Font& f, int flags) override;
	void CCL_API Font_measureString (RectF& size, StringRef text, const Font& f, int flags) override;
	void CCL_API Font_measureStringImage (RectF& size, StringRef text, const Font& font, tbool shiftToBaseline) override;
	void CCL_API Font_measureText (Rect& size, Coord lineWidth, StringRef text, const Font& f, TextFormatRef format) override;
	void CCL_API Font_measureText (RectF& size, CoordF lineWidth, StringRef text, const Font& f, TextFormatRef format) override;
	void CCL_API Font_collapseString (String& string, CoordF maxWidth, const Font& font, int trimMode, tbool exact = false) override;
	IFontTable* CCL_API Font_collectFonts (int flags) override;

	// Factory
	int CCL_API Factory_getNumImageFormats () override;
	const FileType* CCL_API Factory_getImageFormat (int index) override;
	IImage* CCL_API Factory_loadImageFile (UrlRef path) override;
	tbool CCL_API Factory_saveImageFile (UrlRef path, IImage* image, const IAttributeList* encoderOptions = nullptr) override;
	IImage* CCL_API Factory_loadImageStream (IStream& stream, const FileType& format) override;
	tbool CCL_API Factory_saveImageStream (IStream& stream, IImage* image, const FileType& format, const IAttributeList* encoderOptions = nullptr) override;
	IImage* CCL_API Factory_createBitmap (int width, int height, IBitmap::PixelFormat format = IBitmap::kRGB, float scaleFactor = 1.f) override;
	IGraphics* CCL_API Factory_createBitmapGraphics (IImage* bitmap) override;
	IBitmapFilter* CCL_API Factory_createBitmapFilter (StringID which) override;
	IGraphicsPath* CCL_API Factory_createPath (IGraphicsPath::TypeHint type) override;
	IGradient* CCL_API Factory_createGradient (IGradient::TypeHint type) override;
	IImage* CCL_API Factory_createShapeImage () override;
	IGraphics* CCL_API Factory_createShapeBuilder (IImage* shapeImage) override;
	ITextLayout* CCL_API Factory_createTextLayout () override;
	IGraphicsLayer* CCL_API Factory_createGraphicsLayer (UIDRef cid) override;
	IUIValue* CCL_API Factory_createValue () override;
	IImage* CCL_API Factory_createFilmstrip (IImage* sourceImage, StringID frames) override;
	IImage* CCL_API Factory_createImagePart (IImage* sourceImage, RectRef partRect) override;
	IImage* CCL_API Factory_createMultiImage (IImage* images[], CString frameNames[], int count) override;
	IImage* CCL_API Factory_createMultiResolutionBitmap (IImage* bitmaps[], float scaleFactors[], int count) override;

	CLASS_INTERFACE (IGraphicsHelper, Object)

protected:
	class DefaultColorEnum: public EnumTypeInfo
	{
	public:
		DefaultColorEnum ();

		// EnumTypeInfo
		int CCL_API getEnumeratorCount () const override;
		tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override;
	};

	DefaultColorEnum defaultColorEnum;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// UIValue
//************************************************************************************************

class UIValue: public Object,
			   public IUIValue
{
public:
	DECLARE_CLASS (UIValue, Object)

	UIValue ();

	// IUIValue
	void CCL_API reset () override;
	tbool CCL_API copyFrom (const IUIValue* value) override;
	Type CCL_API getType () const override;
	void CCL_API fromPoint (PointRef p) override;
	tbool CCL_API toPoint (Point& p) const override;
	void CCL_API fromRect (RectRef r) override;
	tbool CCL_API toRect (Rect& r) const override;
	void CCL_API fromTransform (TransformRef t) override;
	tbool CCL_API toTransform (Transform& t) const override;
	void CCL_API fromColor (ColorRef c) override;
	tbool CCL_API toColor (Color& color) const override;
	void CCL_API fromColorF (ColorFRef c) override;
	tbool CCL_API toColorF (ColorF& color) const override;
	void CCL_API fromPointF (PointFRef p) override;
	tbool CCL_API toPointF (PointF& p) const override;
	void CCL_API fromRectF (RectFRef r) override;
	tbool CCL_API toRectF (RectF& r) const override;
	void CCL_API fromPointF3D (PointF3DRef p) override;
	tbool CCL_API toPointF3D (PointF3D& p) const override;
	void CCL_API fromPointF4D (PointF4DRef p) override;
	tbool CCL_API toPointF4D (PointF4D& p) const override;
	void CCL_API fromTransform3D (Transform3DRef t) override;
	tbool CCL_API toTransform3D (Transform3D& t) const override;

	// Internal methods to avoid extra copy
	PointRef asPointRef () const { return reinterpret_cast<PointRef> (data.point); }
	RectRef asRectRef () const { return reinterpret_cast<RectRef> (data.rect); }
	TransformRef asTransformRef () const { return reinterpret_cast<TransformRef> (data.transform); }
	ColorRef asColorRef () const { return reinterpret_cast<ColorRef> (data.color); }
	ColorFRef asColorFRef () const { return reinterpret_cast<ColorFRef> (data.colorF); }
	PointFRef asPointFRef () const { return reinterpret_cast<PointFRef> (data.pointF); }
	PointF3DRef asPointF3DRef () const { return reinterpret_cast<PointF3DRef> (data.pointF3d); }
	PointF4DRef asPointF4DRef () const { return reinterpret_cast<PointF4DRef> (data.pointF4d); }
	RectFRef asRectFRef () const { return reinterpret_cast<RectFRef> (data.rectF); }
	Transform3DRef asTransform3DRef () const { return static_cast<Transform3DRef> (data.transform3d); }

	CLASS_INTERFACE (IUIValue, Object)

protected:
	Type type;
	union
	{
		struct { Coord x, y; } point;
		struct { Coord left, top, right, bottom; } rect;
		struct { float a0, a1, b0, b1, t0, t1; } transform;		
		struct { uint8 red, green, blue, alpha; } color;
		struct { float values[4]; } colorF;
		struct { CoordF x, y; } pointF;
		struct { CoordF left, top, right, bottom; } rectF;
		struct { CoordF x, y, z; } pointF3d;
		struct { CoordF x, y, z, w; } pointF4d;
		PlainTransform3D transform3d;
	} data;
};

} // namespace CCL

#endif // _ccl_graphicshelper_h
