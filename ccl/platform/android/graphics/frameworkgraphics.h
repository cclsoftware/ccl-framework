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
// Filename    : ccl/platform/android/graphics/frameworkgraphics.h
// Description : Framework Graphics (native)
//
//************************************************************************************************

#ifndef _ccl_android_frameworkgraphics_h
#define _ccl_android_frameworkgraphics_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/platform/android/graphics/androidfont.h"
#include "ccl/platform/android/graphics/paintcache.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/collections/stack.h"

namespace CCL {

interface IMemoryStream;

namespace Android {

class AndroidBitmap;
class FrameworkGraphics;

//************************************************************************************************
// FrameworkGraphicsFactoryClass
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkGraphicsFactoryClass, CCLGUI_CLASS_PREFIX "FrameworkGraphicsFactory")
	DECLARE_JNI_CONSTRUCTOR (construct, int)
	DECLARE_JNI_METHOD (jobject, createBitmap, int, int, bool)
	DECLARE_JNI_METHOD (jobject, createBitmapRaw, int, int)
	DECLARE_JNI_METHOD (bool, saveBitmap, JniIntPtr, jobject, jstring, int)
	DECLARE_JNI_METHOD (jobject, loadBitmap, jbyteArray)
	DECLARE_JNI_METHOD (jobject, loadFont, jbyteArray)
	DECLARE_JNI_METHOD (jobject, createCachedBitmapPaint, int, int, bool)
	DECLARE_JNI_METHOD (jobject, createCachedDrawPaint, int, int, float, int, bool)
	DECLARE_JNI_METHOD (jobject, createCachedFillPaint, int, int, bool)
	DECLARE_JNI_METHOD (jobject, createCachedTextPaint, int, jobject, int, float, float, int)
	DECLARE_JNI_METHOD (jobject, createLinearGradientPaint, float, float, float, float, jintArray, jfloatArray)
	DECLARE_JNI_METHOD (jobject, createRadialGradientPaint, float, float, float, jintArray, jfloatArray)
END_DECLARE_JNI_CLASS (FrameworkGraphicsFactoryClass)

//************************************************************************************************
// FrameworkGraphicsClass
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkGraphicsClass, CCLGUI_CLASS_PREFIX "FrameworkGraphics")
	DECLARE_JNI_CONSTRUCTOR (constructWithBitmap, jobject)
	DECLARE_JNI_METHOD (bool, isHardwareAccelerated)
	DECLARE_JNI_METHOD (void, saveState)
	DECLARE_JNI_METHOD (void, restoreState)
	DECLARE_JNI_METHOD (void, saveStateAndClip, int, int, int, int)
	DECLARE_JNI_METHOD (void, clipRect, int, int, int, int)
	DECLARE_JNI_METHOD (void, clipRectF, float, float, float, float)
	DECLARE_JNI_METHOD (void, clipPath, jobject)
	DECLARE_JNI_METHOD (void, getClipBounds, jobject)
	DECLARE_JNI_METHOD (void, addTransform, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, translate, float, float)
	DECLARE_JNI_METHOD (void, clearRect, float, float, float, float)
	DECLARE_JNI_METHOD (void, drawRect, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, fillRect, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, drawLine, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, drawEllipse, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, fillEllipse, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, drawPath, jobject, jobject)
	DECLARE_JNI_METHOD (void, fillPath, jobject, jobject)
	DECLARE_JNI_METHOD (void, drawRoundRect, float, float, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, fillRoundRect, float, float, float, float, float, float, jobject)
	DECLARE_JNI_METHOD (void, drawString, jstring, float, float, jobject, int)
	DECLARE_JNI_METHOD (void, measureString, jobject, jstring, jobject)
	DECLARE_JNI_METHOD (void, measureStringF, jobject, jstring, jobject)
	DECLARE_JNI_METHOD (float, getStringWidth, jstring, jobject)
	DECLARE_JNI_METHOD (void, drawText, jstring, float, float, float, float, int, float, bool, jobject)
	DECLARE_JNI_METHOD (void, measureText, jobject, int, float, jstring, jobject)
	DECLARE_JNI_METHOD (void, drawBitmap, jobject, float, float, jobject)
	DECLARE_JNI_METHOD (void, drawBitmapR, jobject, int, int, int, int, int, int, int, int, jobject)
	DECLARE_JNI_METHOD (void, drawBitmapDirect, jobject, int, int, int, int)
END_DECLARE_JNI_CLASS (FrameworkGraphicsClass)

//************************************************************************************************
// FrameworkGraphicsFactory
//************************************************************************************************

class FrameworkGraphicsFactory: public JniObject
{
public:
	FrameworkGraphicsFactory ();
	~FrameworkGraphicsFactory ();

	AndroidBitmap* loadBitmap (IStream& stream);
	AndroidBitmap* createBitmap (PointRef sizeInPixel, bool hasAlpha);
	bool saveBitmap (IStream& stream, AndroidBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions);
	FrameworkGraphics* createBitmapGraphics (AndroidBitmap& bitmap);

	AndroidFont* loadFont (IStream& stream, StringRef name, int fontStyle);
	AndroidFont* getFont (FontRef font);
	AndroidSystemFont* getSystemFont (FontRef font);
	IFontTable* collectFonts (int flags);

	#if DEBUG
	void dumpFonts ();
	#endif

	jobject getCachedBitmapPaint (int alpha, bool filtered);
	jobject getCachedBitmapPaint (const ImageMode* mode);
	jobject getCachedFillPaint (SolidBrushRef brush, bool antiAlias);
	jobject getCachedDrawPaint (PenRef pen, bool antiAlias);
	jobject getCachedTextPaint (FontRef font, SolidBrushRef brush);
	jobject getCachedTextPaint (FontRef font);

private:
	using FontFamily = AndroidFontTable::AndroidFontFamily;

	CCL::ObjectArray fonts;
	CCL::ObjectArray systemFonts;
	mutable Vector<FontFamily*> systemFontFamilies;
	PaintCache<BitmapPaintData> bitmapPaintCache;
	PaintCache<FillPaintData> fillPaintCache;
	PaintCache<DrawPaintData> drawPaintCache;
	PaintCache<TextPaintData> textPaintCache;
	static const int kCacheSize;

	static IMemoryStream* ensureMemoryStream (IStream& stream);
	Vector<FontFamily*>& getSystemFonts () const;

	static int getTypefaceStyle (FontRef font);
};

extern FrameworkGraphicsFactory* gGraphicsFactory;

//************************************************************************************************
// FrameworkGraphics
//************************************************************************************************

class FrameworkGraphics: public NativeGraphicsDevice,
						 public JniObject
{
public:
	DECLARE_CLASS_ABSTRACT (FrameworkGraphics, NativeGraphicsDevice)

	FrameworkGraphics (JNIEnv* jni, jobject object);

	struct ScaleHelper;
	struct FontHelper;

	void setContentScaleFactor (float factor) { contentScaleFactor = factor; }
	void saveStateAndClip (RectRef rect); // combines saveState & addClip (saves a JNI call)

	void getClipBounds (Rect& rect) const;
	bool isHardwareAccelerated () const;

	PROPERTY_OBJECT (Rect, updateRegion, UpdateRegion)

	void beginDraw (RectRef updateRegion);
	bool hasTransform () const;

	// NativeGraphicsDevice
	void setOrigin (PointRef point) override;
	tresult CCL_API saveState () override;
	tresult CCL_API restoreState () override;
	tresult CCL_API addClip (RectRef rect) override;
	tresult CCL_API addClip (RectFRef rect) override;
	tresult CCL_API addClip (IGraphicsPath* path) override;
	tresult CCL_API addTransform (TransformRef matrix) override;
	tresult CCL_API setMode (int mode) override;
	int CCL_API getMode () override;
	float CCL_API getContentScaleFactor () const override;
	tresult CCL_API clearRect (RectRef rect) override;
	tresult CCL_API clearRect (RectFRef rect) override;
	tresult CCL_API fillRect (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillRect (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawRect (RectRef rect, PenRef pen) override;
	tresult CCL_API drawRect (RectFRef rect, PenRef pen) override;
	tresult CCL_API drawLine (PointRef p1, PointRef p2, PenRef pen) override;
	tresult CCL_API drawLine (PointFRef p1, PointFRef p2, PenRef pen) override;
	tresult CCL_API drawEllipse (RectRef rect, PenRef pen) override;
	tresult CCL_API drawEllipse (RectFRef rect, PenRef pen) override;
	tresult CCL_API fillEllipse (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillEllipse (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawPath (IGraphicsPath* path, PenRef pen) override;
	tresult CCL_API fillPath (IGraphicsPath* path, BrushRef brush) override;
	tresult CCL_API drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen) override;
	tresult CCL_API drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen) override;
	tresult CCL_API fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush) override;
	tresult CCL_API fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush) override;
	tresult CCL_API drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	int CCL_API getStringWidth (StringRef text, FontRef font) override;
	CoordF CCL_API getStringWidthF (StringRef text, FontRef font) override;
	tresult CCL_API measureString (Rect& size, StringRef text, FontRef font) override;
	tresult CCL_API measureString (RectF& size, StringRef text, FontRef font) override;
	tresult CCL_API drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;

	static int toJavaColor (ColorRef c);
	static void toCCLPoint (Point& p, JniAccessor& jni, const JniObject& jpoint);	///< from AndroidPoint
	static void toCCLPoint (PointF& p, JniAccessor& jni, const JniObject& jpoint);	///< from AndroidPointF
	static void toCCLRect (Rect& r, JniAccessor& jni, const JniObject& jrect);		///< from AndroidRect
	static void toCCLRect (RectF& r, JniAccessor& jni, const JniObject& jrect);		///< from AndroidRectF

private:
	mutable JniAccessor jni;
	JniObject androidRect;
	JniObject androidRectF;
	float contentScaleFactor;
	bool wasTransformed;

	Stack<int> graphicsModeStack;
	int graphicsMode;

	inline bool isAntiAlias () const { return graphicsMode & kAntiAlias; }
};

//************************************************************************************************
// FrameworkGraphics::ScaleHelper
//************************************************************************************************

struct FrameworkGraphics::ScaleHelper
{
	ScaleHelper (): scaledDevice (nullptr) {}

	ScaleHelper (FrameworkGraphics* device, float scaleFactor)
	{ init (device, scaleFactor); }

	ScaleHelper (FrameworkGraphics* device, float scaleFactor, PointFRef pos)
	{ init (device, scaleFactor, pos); }

	~ScaleHelper ()
	{ exit (); }

	void init (FrameworkGraphics* device, float scaleFactor);
	void init (FrameworkGraphics* device, float scaleFactor, PointFRef pos);
	void exit ();

private:
	FrameworkGraphics* scaledDevice;
};

//************************************************************************************************
// FrameworkGraphics::FontHelper
//************************************************************************************************

struct FrameworkGraphics::FontHelper
{
	jobject typeface;

	FontHelper (FontRef font);

	static jobject getTypeFace (FontRef font);
	static float getLetterSpacing (FontRef font);
};

//************************************************************************************************
// FrameworkBitmapGraphics
//************************************************************************************************

class FrameworkBitmapGraphics: public FrameworkGraphics
{
public:
	DECLARE_CLASS_ABSTRACT (FrameworkBitmapGraphics, FrameworkGraphics)

	FrameworkBitmapGraphics (JNIEnv* jni, jobject object, AndroidBitmap& bitmap);
	~FrameworkBitmapGraphics ();

protected:
	ScaleHelper scaler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FrameworkGraphics::hasTransform () const
{ return wasTransformed; }

inline jobject FrameworkGraphicsFactory::getCachedBitmapPaint (int alpha, bool filtered)
{ return bitmapPaintCache.getPaint (BitmapPaintData (alpha, filtered)); }

inline jobject FrameworkGraphicsFactory::getCachedBitmapPaint (const ImageMode* mode)
{ return bitmapPaintCache.getPaint (BitmapPaintData (mode)); }

inline jobject FrameworkGraphicsFactory::getCachedFillPaint (SolidBrushRef brush, bool antiAlias)
{ return fillPaintCache.getPaint (FillPaintData (brush, antiAlias)); }

inline jobject FrameworkGraphicsFactory::getCachedDrawPaint (PenRef pen, bool antiAlias)
{ return drawPaintCache.getPaint (DrawPaintData (pen, antiAlias)); }

inline jobject FrameworkGraphicsFactory::getCachedTextPaint (FontRef font, SolidBrushRef brush)
{ return textPaintCache.getPaint (TextPaintData (font, brush)); }

inline jobject FrameworkGraphicsFactory::getCachedTextPaint (FontRef font)
{ return textPaintCache.getPaint (TextPaintData (font)); }

} // namespace Android
} // namespace CCL

#endif // _ccl_android_frameworkgraphics_h
