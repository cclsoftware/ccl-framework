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
// Filename    : ccl/gui/graphics/nativegraphics.h
// Description : Native Graphics classes
//
//************************************************************************************************

#ifndef _ccl_nativegraphics_h
#define _ccl_nativegraphics_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/igraphicspath.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"

#include "ccl/gui/graphics/igraphicscleanup.h"
#include "ccl/gui/graphics/imaging/bitmapcodec.h"

namespace CCL {

class Window;
class NativeBitmap;
class NativeGraphicsPath;
class NativeGraphicsDevice;
class NativeGradient;
class NativeWindowRenderTarget;
class FileType;

class Native3DSurface;
interface INative3DSupport;

//************************************************************************************************
// NativeGraphicsEngine
//************************************************************************************************

class NativeGraphicsEngine: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (NativeGraphicsEngine, Object)

	NativeGraphicsEngine ();

	/** Graphics engine singleton. */
	static NativeGraphicsEngine& instance ();

	/** Suppress error reporting. */
	PROPERTY_BOOL (suppressErrors, SuppressErrors)

	/** Allocate graphics resources. */
	virtual bool startup () = 0;

	/** Release graphics resources. */
	virtual void shutdown ();

	/** Recover from hard error (e.g. device lost in D3D on Windows). */
	virtual void recoverFromError () {}

	/** Add object to be cleaned up on engine shutdown. */
	void addCleanup (IGraphicsCleanup* object);

	/** Create render target for given window. */
	virtual NativeWindowRenderTarget* createRenderTarget (Window* window);

	/** Create path object. */
	virtual NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type) = 0;

	/** Create bitmap object. */
	virtual NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f) = 0;

	/** Create offscreen bitmap. */
	virtual NativeBitmap* createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window = nullptr);

	/** Load bitmap from stream. */
	virtual NativeBitmap* loadBitmap (IStream& stream, const FileType& format) = 0;

	/** Save bitmap to stream. */
	virtual bool saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions = nullptr) = 0;

	/** Create gradient object. */
	virtual NativeGradient* createGradient (IGradient::TypeHint type) = 0;

	/** Create device object for painting to a window. */
	virtual NativeGraphicsDevice* createWindowDevice (Window* window, void* systemDevice = nullptr) = 0;

	/** Create device object for painting to a bitmap. */
	virtual NativeGraphicsDevice* createBitmapDevice (NativeBitmap* bitmap) = 0;

	/** Create screenshot from window. */
	virtual NativeBitmap* createScreenshotFromWindow (Window* window) = 0;

	/** Create text layout object. */
	virtual ITextLayout* createTextLayout ();

	/** Install font from memory resource. */
	virtual bool installFontFromMemory (const void* data, uint32 dataSize, StringRef name, int style) { return false; }
	
	/** Set font installation scope. Used to finish pending font registration tasks (optional). */
	virtual bool beginFontInstallation (bool state) { return true; }

	/** Collect installed fonts. */
	virtual IFontTable* collectFonts (int flags) { return nullptr; } 

	/** Check if graphics layers are available. */
	virtual bool hasGraphicsLayers () { return false; }

	/** Create graphics layer. */
	virtual IGraphicsLayer* createGraphicsLayer (UIDRef classID) { return nullptr; }

	/** Create print job. */
	virtual Object* createPrintJob () { return nullptr; }

	/** Get 3D support. */
	virtual INative3DSupport* get3DSupport () { return nullptr; }

	/** Verify availability of required graphics features. */
	bool verifyFeatureSupport ();

protected:
	LinkedList<IGraphicsCleanup*> cleanupList;
};

//************************************************************************************************
// NativeWindowRenderTarget
//************************************************************************************************

class NativeWindowRenderTarget: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (NativeWindowRenderTarget, Object)

	NativeWindowRenderTarget (Window& window);

	Window& getWindow () const { return window; }

	virtual bool shouldCollectUpdates () = 0;

	virtual IMutableRegion* getUpdateRegion () = 0;
	
	virtual void onRender () = 0;

	virtual void onSize () = 0;

	virtual void onScroll (RectRef rect, PointRef delta) = 0;

	virtual IMutableRegion* getInvalidateRegion () { return nullptr; }

	virtual void add3DSurface (Native3DSurface* surface) {}

	virtual void remove3DSurface (Native3DSurface* surface) {}

protected:
	Window& window;
};

//************************************************************************************************
// NativeGraphicsDevice
//************************************************************************************************

class NativeGraphicsDevice: public Object,
							public GraphicsObject<NativeGraphicsDevice>,
							public IGraphics
{
public:
	DECLARE_CLASS_ABSTRACT (NativeGraphicsDevice, Object)

	virtual void setOrigin (PointRef point) { origin = point; }
	PointRef getOrigin () const { return origin; }

	/** Remove any cached graphics objects. */
	virtual void flushStock () {}

	// IGraphics
	float CCL_API getContentScaleFactor () const override { return 1.f; }
	tresult CCL_API drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen) override;
	tresult CCL_API drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen) override;
	tresult CCL_API fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush) override;
	tresult CCL_API fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush) override;
	tresult CCL_API drawTriangle (const Point points[3], PenRef pen) override;
	tresult CCL_API drawTriangle (const PointF points[3], PenRef pen) override;
	tresult CCL_API fillTriangle (const Point points[3], BrushRef brush) override;
	tresult CCL_API fillTriangle (const PointF points[3], BrushRef brush) override;
	int CCL_API getStringWidth (StringRef text, FontRef font) override;
	CoordF CCL_API getStringWidthF (StringRef text, FontRef font) override;
	tresult CCL_API drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API setMode (int mode) override; // only to make this IGraphics method public

	CLASS_INTERFACE (IGraphics, Object)

protected:
	Point origin;

	// Hidden IGraphics methods
	tresult CCL_API drawPath (IGraphicsPath* path, PenRef pen) override;
	tresult CCL_API fillPath (IGraphicsPath* path, BrushRef brush) override;
	tresult CCL_API drawImage (IImage* image, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;

	static NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type = IGraphicsPath::kPaintPath) 
	{ return NativeGraphicsEngine::instance ().createPath (type); }
};

//************************************************************************************************
// NullGraphicsDevice
/** Used as fallback when platform-specific device allocation failed. */
//************************************************************************************************

class NullGraphicsDevice: public NativeGraphicsDevice
{
public:
	DECLARE_CLASS (NullGraphicsDevice, NativeGraphicsDevice)

	// NativeGraphicsDevice
	tresult CCL_API saveState () override;
	tresult CCL_API restoreState () override;
	tresult CCL_API addClip (RectRef rect) override;
	tresult CCL_API addClip (RectFRef rect) override;
	tresult CCL_API addClip (IGraphicsPath* path) override;
	tresult CCL_API addTransform (TransformRef matrix) override;
	tresult CCL_API setMode (int mode) override;
	int CCL_API getMode () override;
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
	tresult CCL_API drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API measureString (Rect& size, StringRef text, FontRef font) override;
	tresult CCL_API measureString (RectF& size, StringRef text, FontRef font) override;
	tresult CCL_API measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
};

/** Make sure either allocated device or fallback is returned. */
inline NativeGraphicsDevice* ensureGraphicsDevice (NativeGraphicsDevice* allocatedDevice)
{
	ASSERT (allocatedDevice != nullptr)
	return allocatedDevice ? allocatedDevice : NEW NullGraphicsDevice;
}

//************************************************************************************************
// NativeGraphicsPath
//************************************************************************************************

class NativeGraphicsPath: public Object,
						  public GraphicsObject<NativeGraphicsPath>,
						  public IGraphicsPath
{
public:
	DECLARE_CLASS_ABSTRACT (NativeGraphicsPath, Object)

	// Internal methods
	virtual tresult draw (NativeGraphicsDevice& device, PenRef pen) = 0;
	virtual tresult fill (NativeGraphicsDevice& device, BrushRef brush) = 0;

	// IGraphicsPath
	void CCL_API addTriangle (PointRef p1, PointRef p2, PointRef p3) override;

	using IGraphicsPath::getBounds;
	using IGraphicsPath::addRect;
	using IGraphicsPath::addRoundRect;
	using IGraphicsPath::addBezier;
	using IGraphicsPath::addArc;
	using IGraphicsPath::lineTo;
	using IGraphicsPath::startFigure;

	void CCL_API startFigure (PointRef p) override;
	void CCL_API getBounds (RectF& bounds) const override;
	void CCL_API lineTo (PointRef p) override;
	void CCL_API addRect (RectFRef rect) override;
	void CCL_API addRoundRect (RectFRef rect, CoordF rx, CoordF ry) override;
	void CCL_API addTriangle (PointFRef p1, PointFRef p2, PointFRef p3) override;
	void CCL_API addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2) override;
	void CCL_API addArc (RectFRef rect, float startAngle, float sweepAngle) override;

	CLASS_INTERFACE (IGraphicsPath, Object)
};

//************************************************************************************************
// NativeBitmap
//************************************************************************************************

class NativeBitmap: public Object,
					public GraphicsObject<NativeBitmap>,
					public IImage,
					public IBitmap
{
public:
	DECLARE_CLASS_ABSTRACT (NativeBitmap, Object)

	NativeBitmap (PointRef sizeInPixel, float contentScaleFactor = 1.f)
	: sizeInPixel (sizeInPixel),
	  contentScaleFactor (contentScaleFactor)
	{}

	void setContentScaleFactor (float factor) { contentScaleFactor = factor; }
	float CCL_API getContentScaleFactor () const override { return contentScaleFactor; }  ///< points to pixels scaling factor

	// ATTENTION: IImage::getWidth()/getHeight() use points, not pixels!
	int CCL_API getWidth () const override;
	int CCL_API getHeight () const override;

	int getPixelWidth () const { return sizeInPixel.x; }
	int getPixelHeight () const	{ return sizeInPixel.y; }
	Point CCL_API getPixelSize () const override { return sizeInPixel; }

	// Internal methods
	virtual tresult draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) = 0;
	virtual tresult tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) = 0;

	// IImage
	ImageType CCL_API getType () const override { return kBitmap; }
	int CCL_API getFrameCount () const override { return 1; }
	int CCL_API getCurrentFrame () const override { return 0; }
	void CCL_API setCurrentFrame (int frameIndex) override {}
	int CCL_API getFrameIndex (StringID name) const override { return -1; }
	IImage* CCL_API getOriginal () override { return this; }

	// IBitmap
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

	CLASS_INTERFACE2 (IImage, IBitmap, Object)

protected:
	Point sizeInPixel;
	float contentScaleFactor;
};

//************************************************************************************************
// NativeGradient
//************************************************************************************************

class NativeGradient: public Object,
					  public GraphicsObject<NativeGradient>,
					  public IGradient
{
public:
	DECLARE_CLASS_ABSTRACT (NativeGradient, Object)

	virtual bool isValid () const { return true; }

	CLASS_INTERFACE (IGradient, Object)

	static constexpr int kMaxStopCount = 10; ///< internal maximum stop count

	static NativeGradient* resolve (IGradient* gradient);

	template <class T>
	static T* resolveTo (IGradient* gradient) { return ccl_cast<T> (resolve (gradient)); }
};

//************************************************************************************************
// NativeGraphicsLayer
//************************************************************************************************

class NativeGraphicsLayer: public Object,
						   public IGraphicsLayer
{
public:
	DECLARE_CLASS_ABSTRACT (NativeGraphicsLayer, Object)

	NativeGraphicsLayer ();
	~NativeGraphicsLayer ();

	PROPERTY_BOOL (deferredRemoval, DeferredRemoval)

	void setContentScaleFactorDeep (float contentScaleFactor); ///< set for this and all sublayers
	void setUpdateNeededRecursive ();
	void removeSublayers ();

	// IGraphicsLayer
	IGraphicsLayer* CCL_API getParentLayer () override;
	IGraphicsLayer* CCL_API getNextSibling (IGraphicsLayer* layer) const override;
	IGraphicsLayer* CCL_API getPreviousSibling (IGraphicsLayer* layer) const override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	virtual void CCL_API setTileSize (int size) override {}
	virtual void CCL_API setBackColor (const Color& color) override {}
	
	CLASS_INTERFACE (IGraphicsLayer, Object)

protected:
	NativeGraphicsLayer* parentLayer;
	ObjectList sublayers;
	ObjectList removedSublayers;
	
	void removePendingSublayers ();
	tresult moveLayer (IGraphicsLayer* layer, IGraphicsLayer* _sibling, bool above);

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// NativeTextLayout
//************************************************************************************************

class NativeTextLayout: public Object,
						public ITextLayout
{
public:
	DECLARE_CLASS_ABSTRACT (NativeTextLayout, Object)

	// ITextLayout
	tresult CCL_API getWordRange (Range& range, int textIndex) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;
	tresult CCL_API getExplicitLineRange (Range& range, int textIndex) const override;

	CLASS_INTERFACE (ITextLayout, Object)

protected:
	static constexpr float kSubscriptSizeFactor = .62;
	static constexpr float kSubscriptBaselineFactor = .16;
	static constexpr float kSuperscriptSizeFactor = .75;
	static constexpr float kSuperscriptBaselineFactor = .3;

private:
	enum RangeMode
	{
		kWord,
		kLine
	};

	tresult getWordOrLineRange (Range& range, RangeMode mode, bool tryNonWord = false) const;
};

//************************************************************************************************
// SimpleTextLayout
//************************************************************************************************

class SimpleTextLayout: public NativeTextLayout
{
public:
	DECLARE_CLASS (SimpleTextLayout, NativeTextLayout)

	SimpleTextLayout ();

	PROPERTY_OBJECT (Font, font, Font)
	PROPERTY_VARIABLE (CoordF, width, Width)
	PROPERTY_VARIABLE (CoordF, height, Height)
	PROPERTY_OBJECT (TextFormat, format, Format)
	PROPERTY_VARIABLE (LineMode, lineMode, LineMode)

	Coord getWidthInt () const { return coordFToInt (getWidth ()); }
	Coord getHeightInt () const { return coordFToInt (getHeight ()); }

	// ITextLayout
	tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format = TextFormat ()) override;
	StringRef CCL_API getText () const override;
	tresult CCL_API resize (Coord width, Coord height) override;
	tresult CCL_API resize (CoordF width, CoordF height) override;
	tresult CCL_API setFontStyle (const Range& range, int style, tbool state) override;
	tresult CCL_API setFontSize (const Range& range, float size) override;
	tresult CCL_API setSpacing (const Range& range, float spacing) override;
	tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API getBounds (Rect& bounds, int flags = 0) const override;
	tresult CCL_API getBounds (RectF& bounds, int flags = 0) const override;
	tresult CCL_API getImageBounds (RectF& bounds) const override;
	tresult CCL_API getBaselineOffset (PointF& offset) const override;
	tresult CCL_API hitTest (int& textIndex, PointF& position) const override;
	tresult CCL_API getCharacterBounds (RectF& bounds, int textIndex) const override;
	tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;

private:
	String text;
};

//************************************************************************************************
// SimpleFontTable
//************************************************************************************************

class SimpleFontTable: public Object,
					   public IFontTable
{
public:
	DECLARE_CLASS (SimpleFontTable, Object)

	struct FontFamily: public Unknown
	{
		String name;
		String exampleText;
		Vector<String> styles;
	};

	void clear ();
	void addFamily (FontFamily* family);
	void addFamilySorted (FontFamily* family);

	// IFontTable
	int CCL_API countFonts () override;
	tresult CCL_API getFontName (String& name, int index) override;
	int CCL_API countFontStyles (int fontIndex) override;
	tresult CCL_API getFontStyleName (String& name, int fontIndex, int styleIndex) override;
	tresult CCL_API getExampleText (String& text, int fontIndex, int styleIndex) override;

	CLASS_INTERFACE (IFontTable, Object)

protected:	
	Vector<AutoPtr <FontFamily> > fonts;
};

} // namespace CCL

#endif // _ccl_nativegraphics_h
