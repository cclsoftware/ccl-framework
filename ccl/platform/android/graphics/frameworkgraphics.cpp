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
// Filename    : ccl/platform/android/graphics/frameworkgraphics.cpp
// Description : Framework Graphics (native)
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "frameworkgraphics.h"

#include "androidgraphics.h"
#include "androidbitmap.h"
#include "androidgradient.h"
#include "androidpath.h"
#include "androidtextlayout.h"

#include "ttfparser.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// ProfileScope
//************************************************************************************************

struct ProfileScope
{
	ProfileScope (const char* text)
	: text (text),
	  start (Debugger::getProfileTime ())
	{}
	~ProfileScope ()
	{
		double seconds = Debugger::getProfileTime () - start;
		Debugger::printf ("%f ms  %s\n", (float)seconds * 1000., text);
	}

	double start;
	const char* text;
};

#if DEBUG_LOG
#define PROFILE_SCOPE(s) ProfileScope _profile##s (#s);
#else
#define PROFILE_SCOPE(s)
#endif

//************************************************************************************************
// JNI classes
//************************************************************************************************

//************************************************************************************************
// FrameworkGraphicsFactoryClass
//************************************************************************************************

DEFINE_JNI_CLASS (FrameworkGraphicsFactoryClass)
	DEFINE_JNI_CONSTRUCTOR (construct, "(I)V")
	DEFINE_JNI_METHOD (createBitmap, "(IIZ)Landroid/graphics/Bitmap;")
	DEFINE_JNI_METHOD (createBitmapRaw, "(II)Landroid/graphics/Bitmap;")	
	DEFINE_JNI_METHOD (saveBitmap, "(JLandroid/graphics/Bitmap;Ljava/lang/String;I)Z")
	DEFINE_JNI_METHOD (loadBitmap, "([B)Landroid/graphics/Bitmap;")
	DEFINE_JNI_METHOD (loadFont, "([B)Landroid/graphics/Typeface;")
	DEFINE_JNI_METHOD (createCachedBitmapPaint, "(IIZ)Landroid/graphics/Paint;")
	DEFINE_JNI_METHOD (createCachedFillPaint, "(IIZ)Landroid/graphics/Paint;")
	DEFINE_JNI_METHOD (createCachedDrawPaint, "(IIFIZ)Landroid/graphics/Paint;")
	DEFINE_JNI_METHOD (createCachedTextPaint, "(ILandroid/graphics/Typeface;IFFI)Landroid/graphics/Paint;")
	DEFINE_JNI_METHOD (createLinearGradientPaint, "(FFFF[I[F)Landroid/graphics/Paint;")
	DEFINE_JNI_METHOD (createRadialGradientPaint, "(FFF[I[F)Landroid/graphics/Paint;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// FrameworkGraphicsClass
//************************************************************************************************

DEFINE_JNI_CLASS (FrameworkGraphicsClass)
	DEFINE_JNI_CONSTRUCTOR (constructWithBitmap, "(Landroid/graphics/Bitmap;)V")
	DEFINE_JNI_METHOD (isHardwareAccelerated, "()Z")	
	DEFINE_JNI_METHOD (saveState, "()V")
	DEFINE_JNI_METHOD (restoreState, "()V")
	DEFINE_JNI_METHOD (saveStateAndClip, "(IIII)V")
	DEFINE_JNI_METHOD (clipRect, "(IIII)V")
	DEFINE_JNI_METHOD (clipRectF, "(FFFF)V")
	DEFINE_JNI_METHOD (clipPath, "(L" CCLGUI_CLASS_PREFIX "FrameworkGraphicsPath;)V")
	DEFINE_JNI_METHOD (getClipBounds, "(Landroid/graphics/Rect;)V")
	DEFINE_JNI_METHOD (addTransform, "(FFFFFF)V")
	DEFINE_JNI_METHOD (translate, "(FF)V")
	DEFINE_JNI_METHOD (clearRect, "(FFFF)V")
	DEFINE_JNI_METHOD (drawRect, "(FFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (fillRect, "(FFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawLine, "(FFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawEllipse, "(FFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (fillEllipse, "(FFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawPath, "(L" CCLGUI_CLASS_PREFIX "FrameworkGraphicsPath;Landroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (fillPath, "(L" CCLGUI_CLASS_PREFIX "FrameworkGraphicsPath;Landroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawRoundRect, "(FFFFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (fillRoundRect, "(FFFFFFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawString, "(Ljava/lang/String;FFLandroid/graphics/Paint;I)V")
	DEFINE_JNI_METHOD (measureString, "(Landroid/graphics/Rect;Ljava/lang/String;Landroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (measureStringF, "(Landroid/graphics/RectF;Ljava/lang/String;Landroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (getStringWidth, "(Ljava/lang/String;Landroid/graphics/Paint;)F")
	DEFINE_JNI_METHOD (drawText, "(Ljava/lang/String;FFFFIFZLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (measureText, "(Landroid/graphics/Rect;IFLjava/lang/String;Landroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawBitmap, "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawBitmapR, "(Landroid/graphics/Bitmap;IIIIIIIILandroid/graphics/Paint;)V")
	DEFINE_JNI_METHOD (drawBitmapDirect, "(Landroid/graphics/Bitmap;IIII)V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// FrameworkGraphics::ScaleHelper
//************************************************************************************************

void FrameworkGraphics::ScaleHelper::init (FrameworkGraphics* device, float scaleFactor)
{
	if(scaleFactor != 1.f)
	{
		device->saveState ();
		device->addTransform (Transform ().scale (scaleFactor, scaleFactor));
		scaledDevice = device;
	}
	else
		scaledDevice = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::ScaleHelper::init (FrameworkGraphics* device, float factor, PointFRef pos)
{
	if(factor != 1.f)
	{
		factor = 1.f / factor; // scale bitmap pixels to coords

		Transform t;
		t.translate (pos.x, pos.y); // translate to scale at bitmap origin
		t.scale (factor, factor);
		t.translate (-pos.x, -pos.y); // translate back
		device->saveState ();
		device->addTransform (t);
		scaledDevice = device;
	}
	else
		scaledDevice = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::ScaleHelper::exit ()
{
	if(scaledDevice)
	{
		scaledDevice->restoreState ();
		scaledDevice = nullptr;
	}
}

//************************************************************************************************
// FrameworkGraphicsFactory
//************************************************************************************************

FrameworkGraphicsFactory* CCL::Android::gGraphicsFactory = nullptr;

const int FrameworkGraphicsFactory::kCacheSize = 16;

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkGraphicsFactory::FrameworkGraphicsFactory ()
: bitmapPaintCache (this, kCacheSize, "bitmap"),
  fillPaintCache (this, kCacheSize, "fill"),
  drawPaintCache (this, kCacheSize, "draw"),
  textPaintCache (this, kCacheSize, "text")
{
	JniAccessor jni;
	LocalRef localRef (jni, jni.newObject (FrameworkGraphicsFactoryClass, FrameworkGraphicsFactoryClass.construct, kCacheSize));
	assign (jni, localRef);

	fonts.objectCleanup ();
	systemFonts.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkGraphicsFactory::~FrameworkGraphicsFactory ()
{
	VectorForEach (systemFontFamilies, FontFamily*, family)
		family->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMemoryStream* FrameworkGraphicsFactory::ensureMemoryStream (IStream& stream)
{
	AutoPtr<IMemoryStream> memStream;
	if(UnknownPtr<IMemoryStream> sourceStream = &stream)
		memStream = sourceStream.detach ();
	else
	{
		memStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
		if(memStream == nullptr)
			return nullptr;
	}
	return memStream.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidBitmap* FrameworkGraphicsFactory::loadBitmap (IStream& stream)
{
	AutoPtr<IMemoryStream> memStream (ensureMemoryStream (stream));
	if(memStream == nullptr)
		return nullptr;

	// let Android BitmapFactory decode the image on the Java side
	JniAccessor jni;

	Core::Java::JniByteArray data (jni, (const jbyte*)memStream->getMemoryAddress (), (int)memStream->getBytesWritten ());
	LocalRef object (jni, FrameworkGraphicsFactoryClass.loadBitmap (*this, data));
	if(jni.checkException () || object == nullptr)
		return nullptr;

	return NEW AndroidBitmap (jni, object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkGraphicsFactory::saveBitmap (IStream& stream, AndroidBitmap& androidBitmap, const FileType& format, const IAttributeList* encoderOptions)
{
	JavaBitmap* javaBmp = androidBitmap.getJavaBitmap ();
	if(javaBmp)
	{
		int quality = 85;

		Variant value;
		if(encoderOptions && encoderOptions->getAttribute (value, ImageEncoding::kQuality))
			quality = value.parseInt ();

		JniCCLString mimeType (format.getMimeType ());
		return FrameworkGraphicsFactoryClass.saveBitmap (*this, (JniIntPtr) &stream, *javaBmp, mimeType, quality);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidBitmap* FrameworkGraphicsFactory::createBitmap (PointRef sizeInPixel, bool hasAlpha)
{
	JniAccessor jni;
	LocalRef object (jni, FrameworkGraphicsFactoryClass.createBitmap (*this, sizeInPixel.x, sizeInPixel.y, hasAlpha));
	if(jni.checkException () || object == nullptr)
		return nullptr;

	return NEW AndroidBitmap (jni, object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkGraphics* FrameworkGraphicsFactory::createBitmapGraphics (AndroidBitmap& bitmap)
{
	JavaBitmap* javaBitmap = bitmap.getJavaBitmap ();
	if(!javaBitmap)
		return nullptr;

	JniAccessor jni;
	LocalRef object (jni, jni.newObject (FrameworkGraphicsClass, FrameworkGraphicsClass.constructWithBitmap, javaBitmap->getJObject ()));
	if(jni.checkException () || object == nullptr)
		return nullptr;

	return NEW FrameworkBitmapGraphics (jni, object, bitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidFont* FrameworkGraphicsFactory::loadFont (IStream& stream, StringRef name, int fontStyle)
{
	AutoPtr<IMemoryStream> memStream (ensureMemoryStream (stream));
	if(memStream == nullptr)
		return nullptr;

	JniAccessor jni;

	Core::Java::JniByteArray data (jni, (const jbyte*)memStream->getMemoryAddress (), (int)memStream->getBytesWritten ());
	LocalRef typeface (jni, FrameworkGraphicsFactoryClass.loadFont (*this, data));
	if(typeface)
	{
		// parse font names from file
		TTFParser::FontInfo info;
		TTFParser::parseFontInfo (info, *memStream);

		AndroidFont* font = NEW AndroidFont (jni, typeface);
		font->setFamilyName (info.getString (TTFParser::kFamilyName));
		font->setFullName (info.getString (TTFParser::kFullFontName));

		CCL_PRINTF ("FrameworkGraphicsFactory::loadFont: \"%s\" (\"%s\")\n)", 
					MutableCString (font->getFamilyName ()).str (),
					MutableCString (font->getFullName ()).str ())

		font->setStyle (fontStyle);
		font->setSymbolFont (info.isSymbolFont ());
		fonts.add (font);
		return font;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* FrameworkGraphicsFactory::collectFonts (int flags)
{
	AutoPtr<AndroidFontTable> fontTable = NEW AndroidFontTable;

	bool collectSymbolicFonts = (flags & Font::kCollectSymbolicFonts) != 0;
	bool collectAppFonts = (flags & Font::kCollectAppFonts) != 0;

	for(FontFamily* systemFont : getSystemFonts ())
	{
		if(systemFont->symbolFont && !collectSymbolicFonts)
			continue;

		FontFamily* family = NEW FontFamily (*systemFont);
		fontTable->addFamilySorted (family);
	}

	if(collectAppFonts)
	{
		ArrayForEachFast (fonts, AndroidFont, font)
			if(font->isSymbolFont () && !collectSymbolicFonts)
				continue;

			FontFamily* family = fontTable->findFamily (font->getFamilyName ());
			if(!family)
			{
				family = NEW FontFamily;
				family->name = font->getFamilyName ();
				fontTable->addFamilySorted (family);
			}

			String style = "Regular";
			if(font->isBold () && font->isItalic ())
				style = "Bold Italic";
			else if(font->isBold ())
				style = "Bold";
			else if(font->isItalic ())
				style = "Italic";

			if(!family->styles.contains (style))
				family->styles.add (style);
		EndFor
	}

	return fontTable.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vector<FrameworkGraphicsFactory::FontFamily*>& FrameworkGraphicsFactory::getSystemFonts () const
{
	auto getFamily = [&] (const TTFParser::FontInfo& info) -> FontFamily*
	{
		StringRef familyName (info.getString (TTFParser::kFamilyName));
		for(FontFamily* family : systemFontFamilies)
			if(family->name == familyName)
				return family;

		FontFamily* family = NEW FontFamily;
		family->name = familyName;
		family->exampleText = info.getString (TTFParser::kSampleText);
		systemFontFamilies.add (family);
		return family;
	};

	auto addStyleOnce = [] (FontFamily& family, StringRef styleName, const IUrl* styleUrl)
	{
		if(family.styles.contains (styleName))
			return;

		String path;
		styleUrl->getUrl (path);
		family.styles.add (styleName);
		family.paths.add (Url (path));
	};

	if(systemFontFamilies.isEmpty ())
	{
		Url fontsFolder;
		fontsFolder.fromPOSIXPath ("/system/fonts", IUrl::kFolder);	

		ForEachFile (File (fontsFolder).newIterator (IFileIterator::kFiles), path)
			AutoPtr<IMemoryStream> memStream (File::loadBinaryFile (*path));
			if(memStream)
			{
				TTFParser::FontInfo info;
				if(TTFParser::parseFontInfo (info, *memStream))
				{
					StringRef subFamily (info.getString (TTFParser::kSubFamilyName));
					CCL_PRINTF ("FrameworkGraphicsFactory::scanSystemFonts: \"%s\", %s (\"%s\") %s",
								MutableCString (info.getString (TTFParser::kFamilyName)).str (),
								MutableCString (subFamily).str (),
								MutableCString (info.getString (TTFParser::kFullFontName)).str (),
								MutableCString (info.getString (TTFParser::kSampleText)).str ())
					
					FontFamily* family = getFamily (info);
					addStyleOnce (*family, subFamily, path);
				}
			}
		EndFor
	}
	return systemFontFamilies;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FrameworkGraphicsFactory::getTypefaceStyle (FontRef font)
{
	int style = Typeface.NORMAL;
	if(font.getStyleName ().isEmpty ())
	{
		if(font.isBold ())
			style |= Typeface.BOLD;
		if(font.isItalic ())
			style |= Typeface.ITALIC;
	}
	else
	{
		if(font.getStyleName ().contains ("bold", false))
			style |= Typeface.BOLD;
		if(font.getStyleName ().contains ("italic", false))
			style |= Typeface.ITALIC;
	}
	return style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidFont* FrameworkGraphicsFactory::getFont (FontRef referenceFont)
{
	StringRef fontFace = referenceFont.getFace ();
	int fontStyle = getTypefaceStyle (referenceFont);

	// find best matching font
	AndroidFont* androidFont = nullptr;
	ArrayForEachFast (fonts, AndroidFont, font)
		// font family must match
		if(font->getFamilyName () != fontFace)
			continue;

		// skip bold/italic fonts if that style is not requested
		if((font->isBold () && !(fontStyle & Typeface.BOLD)) ||
		   (font->isItalic () && !(fontStyle & Typeface.ITALIC)))
			continue;

		// prefer italic over bold if both are requested, but only one is available
		if(androidFont && androidFont->isItalic () && !font->isItalic ())
			continue;

		androidFont = font;

		if(androidFont->getStyle () == fontStyle)
			break;
	EndFor

	// if necessary, create a derived typeface emulating missing styles
	if(androidFont && androidFont->getStyle () != fontStyle)
	{
		JniAccessor jni;
		androidFont = NEW AndroidFont (jni, Typeface.createWithTypeface (*androidFont, fontStyle));
		androidFont->setFamilyName (fontFace);
		androidFont->setStyle (fontStyle);
		fonts.add (androidFont);
	}
	return androidFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSystemFont* FrameworkGraphicsFactory::getSystemFont (FontRef font)
{
	AndroidSystemFont* systemFont = systemFonts.findIf<AndroidSystemFont> ([&] (const AndroidSystemFont& sysFont)
	{
		return sysFont.matches (font);
	});

	if(!systemFont)
	{
		// load font from font file
		for(FontFamily* family : systemFontFamilies)
		{
			if(family->name != font.getFace ())
				continue;

			for(int i = 0; i < family->styles.count (); i++)
			{
				if(family->styles[i] != font.getStyleName ())
					continue;

				String pathString;
				family->paths[i].toDisplayString (pathString);

				JniAccessor jni;
				systemFont = NEW AndroidSystemFont (jni, Typeface.createFromFile (JniCCLString (pathString)), font);
				systemFonts.add (systemFont);
				break;
			}
		}
	}

	if(!systemFont)
	{
		// load font by font family name (serif, sans-serif, monospace etc.)
		JniCCLString familyString (font.getFace ());
		int style = getTypefaceStyle (font);

		JniAccessor jni;
		systemFont = NEW AndroidSystemFont (jni, Typeface.create (familyString, style), font);
		systemFonts.add (systemFont);
	}
	return systemFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void FrameworkGraphicsFactory::dumpFonts ()
{
	int fontIndex = 0;
	ArrayForEachFast (fonts, AndroidFont, font)
		MutableCString fontName (font->getFullName ());
		CStringPtr fontNamePtr = fontName.str ();
		Debugger::printf ("Font %d: '%s' %s\n", fontIndex++, fontNamePtr, font->isBold () ? "bold" : "regular");
	EndFor
}
#endif

//************************************************************************************************
// FrameworkGraphics::FontHelper
//************************************************************************************************

FrameworkGraphics::FontHelper::FontHelper (FontRef font)
: typeface (getTypeFace (font))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject FrameworkGraphics::FontHelper::getTypeFace (FontRef font)
{
	// application fonts
	AndroidFont* androidFont = gGraphicsFactory->getFont (font);
	if(androidFont)
		return *androidFont;

	// system fonts
	if(AndroidSystemFont* systemFont = gGraphicsFactory->getSystemFont (font))
		return *systemFont;

	#if (0 && DEBUG)
	if(font.getFace () != Font::getDefaultFont ().getFace ())
	{
		gGraphicsFactory->dumpFonts ();
	}
	#endif
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float FrameworkGraphics::FontHelper::getLetterSpacing (FontRef font)
{
	if(font.getSpacing () == 0.f)
		return 0.f;
	else
		return font.getSpacing () / font.getSize ();
}

//************************************************************************************************
// FrameworkBitmapGraphics
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FrameworkBitmapGraphics, FrameworkGraphics)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkBitmapGraphics::FrameworkBitmapGraphics (JNIEnv* jni, jobject object, AndroidBitmap& bitmap)
: FrameworkGraphics (jni, object)
{
	#if (0 && DEBUG) // expecting false here for rendering into software bitmap
	bool isAccelerated = isHardwareAccelerated ();
	#endif

	setUpdateRegion (Rect (0, 0, bitmap.getWidth (), bitmap.getHeight ()));

	setContentScaleFactor (bitmap.getContentScaleFactor ());
	scaler.init (this, getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkBitmapGraphics::~FrameworkBitmapGraphics ()
{
	scaler.exit ();
}

//************************************************************************************************
// FrameworkGraphics
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FrameworkGraphics, NativeGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

int FrameworkGraphics::toJavaColor (ColorRef c) 
{
	return (c.alpha << 24) | (c.red << 16) | (c.green << 8) | c.blue; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::toCCLPoint (Point& p, JniAccessor& jni, const JniObject& jpoint)
{
	p.x = jni.getField (jpoint, AndroidPoint.x);
	p.y = jni.getField (jpoint, AndroidPoint.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::toCCLPoint (PointF& p, JniAccessor& jni, const JniObject& jpoint)
{
	p.x = jni.getField (jpoint, AndroidPointF.x);
	p.y = jni.getField (jpoint, AndroidPointF.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::toCCLRect (Rect& r, JniAccessor& jni, const JniObject& jrect)
{
	r.left = jni.getField (jrect, AndroidRect.left);
	r.top = jni.getField (jrect, AndroidRect.top);
	r.right = jni.getField (jrect, AndroidRect.right);
	r.bottom = jni.getField (jrect, AndroidRect.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::toCCLRect (RectF& r, JniAccessor& jni, const JniObject& jrect)
{
	r.left = jni.getField (jrect, AndroidRectF.left);
	r.top = jni.getField (jrect, AndroidRectF.top);
	r.right = jni.getField (jrect, AndroidRectF.right);
	r.bottom = jni.getField (jrect, AndroidRectF.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkGraphics::FrameworkGraphics (JNIEnv* jni, jobject object)
: JniObject (jni, object),
  contentScaleFactor (1.f),
  wasTransformed (false),
  graphicsMode (0)
{
	androidRect.newObject (jni, AndroidRect);
	androidRectF.newObject (jni, AndroidRectF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::getClipBounds (Rect& rect) const
{
	FrameworkGraphicsClass.getClipBounds (*this, androidRect);
	toCCLRect (rect, jni, androidRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkGraphics::isHardwareAccelerated () const
{
	return FrameworkGraphicsClass.isHardwareAccelerated (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::beginDraw (RectRef updateRegion)
{
	setUpdateRegion (updateRegion);
	wasTransformed = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::setOrigin (PointRef point)
{
	FrameworkGraphicsClass.translate (*this, point.x - origin.x, point.y - origin.y);
	NativeGraphicsDevice::setOrigin (point);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::saveState ()
{
	graphicsModeStack.push (graphicsMode);
	FrameworkGraphicsClass.saveState (*this);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::restoreState ()
{
	FrameworkGraphicsClass.restoreState (*this);
	graphicsMode = graphicsModeStack.pop ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkGraphics::saveStateAndClip (RectRef rect)
{
	graphicsModeStack.push (graphicsMode);
	FrameworkGraphicsClass.saveStateAndClip (*this, rect.left, rect.top, rect.right, rect.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::addClip (RectRef rect)
{
	FrameworkGraphicsClass.clipRect (*this, rect.left, rect.top, rect.right, rect.bottom);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult CCL_API FrameworkGraphics::addClip (RectFRef rect)
{
	FrameworkGraphicsClass.clipRectF (*this, rect.left, rect.top, rect.right, rect.bottom);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::addClip (IGraphicsPath* path)
{
	AndroidGraphicsPath* p = unknown_cast<AndroidGraphicsPath> (path);
	ASSERT (p)
	if(!p)
		return kResultInvalidArgument;

	FrameworkGraphicsClass.clipPath (*this, *p);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::addTransform (TransformRef t)
{
	wasTransformed = true; // very rough, we don't track restoreSate etc.

	FrameworkGraphicsClass.addTransform (*this, t.a0, t.a1, t.b0, t.b1, t.t0, t.t1);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::setMode (int mode)
{
	graphicsMode = mode & kAntiAlias;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FrameworkGraphics::getMode ()
{
	return graphicsMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API FrameworkGraphics::getContentScaleFactor () const
{
	return contentScaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::clearRect (RectRef rect)
{
	return clearRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::clearRect (RectFRef rect)
{
	PROFILE_SCOPE (clearRect)

	FrameworkGraphicsClass.clearRect (*this, rect.left, rect.top, rect.right, rect.bottom);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawRect (RectRef rect, PenRef pen)
{
	return drawRect (rectIntToF (rect), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawRect (RectFRef rect, PenRef pen)
{
	PROFILE_SCOPE (drawRect)

	jobject paint = gGraphicsFactory->getCachedDrawPaint (pen, isAntiAlias ());
	FrameworkGraphicsClass.drawRect (*this, rect.left, rect.top, rect.right, rect.bottom, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillRect (RectRef rect, BrushRef brush)
{
	return fillRect (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillRect (RectFRef rect, BrushRef brush)
{
	PROFILE_SCOPE (fillRect)

	jobject paint;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		paint = gGraphicsFactory->getCachedFillPaint (*solidBrush, isAntiAlias ());
	else if(auto gradient = NativeGradient::resolveTo<AndroidGradient> (brush.getGradient ()))
		paint = gradient->getPaint ();
	else
		return kResultInvalidArgument;

	FrameworkGraphicsClass.fillRect (*this, rect.left, rect.top, rect.right, rect.bottom, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	return drawLine (pointIntToF (p1), pointIntToF (p2), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	PROFILE_SCOPE (drawLine)

	jobject paint = gGraphicsFactory->getCachedDrawPaint (pen, isAntiAlias ());
	FrameworkGraphicsClass.drawLine (*this, p1.x, p1.y, p2.x, p2.y, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawEllipse (RectRef rect, PenRef pen)
{
	return drawEllipse (rectIntToF (rect), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawEllipse (RectFRef rect, PenRef pen)
{
	PROFILE_SCOPE (drawEllipse)

	//AntiAliasSetter smoother (*this); // enable anti-aliasing
	jobject paint = gGraphicsFactory->getCachedDrawPaint (pen, true);

	FrameworkGraphicsClass.drawEllipse (*this, rect.left, rect.top, rect.right, rect.bottom, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillEllipse (RectRef rect, BrushRef brush)
{
	return fillEllipse (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillEllipse (RectFRef rect, BrushRef brush)
{
	PROFILE_SCOPE (fillEllipse)

	jobject paint;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		paint = gGraphicsFactory->getCachedFillPaint (*solidBrush, true); // enable anti-aliasing
	else if(auto gradient = NativeGradient::resolveTo<AndroidGradient> (brush.getGradient ()))
		paint = gradient->getPaint ();
	else
		return kResultInvalidArgument;

	FrameworkGraphicsClass.fillEllipse (*this, rect.left, rect.top, rect.right, rect.bottom, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawPath (IGraphicsPath* path, PenRef pen)
{
	PROFILE_SCOPE (drawPath)

	AndroidGraphicsPath* p = unknown_cast<AndroidGraphicsPath> (path);
	if(!p)
		return kResultInvalidArgument;

	//AntiAliasSetter smoother (*this); // enable anti-aliasing
	jobject paint = gGraphicsFactory->getCachedDrawPaint (pen, true);

	FrameworkGraphicsClass.drawPath (*this, *p, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillPath (IGraphicsPath* path, BrushRef brush)
{
	PROFILE_SCOPE (fillPath)

	AndroidGraphicsPath* p = unknown_cast<AndroidGraphicsPath> (path);
	if(!p)
		return kResultInvalidArgument;

	jobject paint;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		paint = gGraphicsFactory->getCachedFillPaint (*solidBrush, true); // enable anti-aliasing
	else if(auto gradient = NativeGradient::resolveTo<AndroidGradient> (brush.getGradient ()))
		paint = gradient->getPaint ();
	else
		return kResultInvalidArgument;

	FrameworkGraphicsClass.fillPath (*this, *p, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen)
{
	return drawRoundRect (rectIntToF (rect), CoordF (rx), CoordF (ry), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen)
{
	PROFILE_SCOPE (drawRoundRect)

	jobject paint = gGraphicsFactory->getCachedDrawPaint (pen, isAntiAlias ());

	FrameworkGraphicsClass.drawRoundRect (*this, rect.left, rect.top, rect.right, rect.bottom, rx, ry, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush)
{
	return fillRoundRect (rectIntToF (rect), CoordF (rx), CoordF (ry), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush)
{
	PROFILE_SCOPE (fillRoundRect)

	jobject paint;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		paint = gGraphicsFactory->getCachedFillPaint (*solidBrush, isAntiAlias ());
	else if(auto gradient = NativeGradient::resolveTo<AndroidGradient> (brush.getGradient ()))
		paint = gradient->getPaint ();
	else
		return kResultInvalidArgument;

	FrameworkGraphicsClass.fillRoundRect (*this, rect.left, rect.top, rect.right, rect.bottom, rx, ry, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawString (rectIntToF (rect), text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	PROFILE_SCOPE (drawString)

	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	if(!solidBrush)
		return kResultInvalidArgument;

	JniCCLString jniString (text);

	jobject paint = gGraphicsFactory->getCachedTextPaint (font, *solidBrush);
	FrameworkGraphicsClass.drawText (*this, jniString, rect.left, rect.top, rect.getWidth (), rect.getHeight (), alignment.align, 1.0, false, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return drawString (pointIntToF (point), text, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	if(!solidBrush)
		return kResultInvalidArgument;

	JniCCLString jniString (text);

	jobject paint = gGraphicsFactory->getCachedTextPaint (font, *solidBrush);
	FrameworkGraphicsClass.drawString (*this, jniString, point.x, point.y, paint, options);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FrameworkGraphics::getStringWidth (StringRef text, FontRef font)
{
	return coordFToInt (getStringWidthF (text, font));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF CCL_API FrameworkGraphics::getStringWidthF (StringRef text, FontRef font)
{
	PROFILE_SCOPE (getStringWidth)

	JniCCLString jniString (text);
	
	jobject paint = gGraphicsFactory->getCachedTextPaint (font);
	return FrameworkGraphicsClass.getStringWidth (*this, jniString, paint);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::measureString (Rect& size, StringRef text, FontRef font)
{
	PROFILE_SCOPE (measureString)

	JniCCLString jniString (text);

	jobject paint = gGraphicsFactory->getCachedTextPaint (font);
	FrameworkGraphicsClass.measureString (*this, androidRect, jniString, paint);

	toCCLRect (size, jni, androidRect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::measureString (RectF& size, StringRef text, FontRef font)
{
	PROFILE_SCOPE (measureString)

	JniCCLString jniString (text);

	jobject paint = gGraphicsFactory->getCachedTextPaint (font);
	FrameworkGraphicsClass.measureStringF (*this, androidRectF, jniString, paint);

	toCCLRect (size, jni, androidRectF);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	return drawText (rectIntToF (rect), text, font, brush, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	PROFILE_SCOPE (drawText)

	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	if(!solidBrush)
		return kResultInvalidArgument;

	JniCCLString jniString (text);

	int align = format.getAlignment ().align;
	// todo: wordbreak

	jobject paint = gGraphicsFactory->getCachedTextPaint (font, *solidBrush);
	FrameworkGraphicsClass.drawText (*this, jniString, rect.left, rect.top, rect.getWidth (), rect.getHeight (), align, font.getLineSpacing (), true, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font)
{
	PROFILE_SCOPE (measureText)

	JniCCLString jniString (text);

	jobject paint = gGraphicsFactory->getCachedTextPaint (font);
	FrameworkGraphicsClass.measureText (*this, androidRect, lineWidth, font.getLineSpacing (), jniString, paint);

	toCCLRect (size, jni, androidRect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font)
{
	// (there is no float equivalent for the implementation on the java side)
	Rect s;
	measureText (s, (Coord)lineWidth, text, font);
	size = rectIntToF (s);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	return drawTextLayout (pointIntToF (pos), textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkGraphics::drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	PROFILE_SCOPE (drawTextLayout)

	AndroidTextLayout* androidTextLayout = unknown_cast<AndroidTextLayout> (textLayout);
	if(!androidTextLayout)
		return kResultInvalidArgument;

	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	if(!solidBrush)
		return kResultInvalidArgument;

	androidTextLayout->draw (*this, pos, solidBrush->getColor (), options);
	return kResultOk;
}
