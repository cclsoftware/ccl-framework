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
// Filename    : ccl/platform/android/graphics/java/FrameworkGraphicsFactory.java
// Description : Framework Graphics Factory
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.cclsystem.OutputStreamWrapper;

import android.graphics.*;
import android.text.*;

import androidx.annotation.Keep;

import java.io.*;

//************************************************************************************************
// PaintCache
//************************************************************************************************

class PaintCache
{
	protected Paint[] paints;

	public PaintCache (int cacheSize)
	{	
		paints = new Paint[cacheSize];
	}

	public Paint getPaint (int index)
	{
		return paints[index];
	}

	public Paint createPaint (int index)
	{
		Paint paint = paints[index];
		if(paint == null)
		{
			paint = createPaint ();
			paints[index] = paint;
		}
		return paint;
	}

	public Paint createPaint ()
	{
		return new Paint ();
	}
}

//************************************************************************************************
// TextPaintCache
//************************************************************************************************

class TextPaintCache extends PaintCache
{
	public TextPaintCache (int cacheSize)
	{	
		 super (cacheSize);
	}

	public Paint createPaint ()
	{
		return FrameworkGraphicsFactory.createTextPaint ();
	}
}

//************************************************************************************************
// DrawPaintCache
//************************************************************************************************

class DrawPaintCache extends PaintCache
{
	private final Paint.Style style;

	public DrawPaintCache (int cacheSize, Paint.Style style)
	{	
		 super (cacheSize);
		 this.style = style;
	}

	public Paint createPaint ()
	{
		Paint paint = new Paint ();
		paint.setStyle (style);
		return paint;
	}
}

//************************************************************************************************
// FrameworkGraphicsFactory
//************************************************************************************************

@Keep
public class FrameworkGraphicsFactory
{
	static private PaintCache bitmapPaints;
	static private DrawPaintCache fillPaints;
	static private DrawPaintCache drawPaints;
	static private TextPaintCache textPaints;

	public FrameworkGraphicsFactory (int cacheSize)
	{
		bitmapPaints = new PaintCache (cacheSize);
		fillPaints = new DrawPaintCache (cacheSize, Paint.Style.FILL);
		drawPaints = new DrawPaintCache (cacheSize, Paint.Style.STROKE);
		textPaints = new TextPaintCache (cacheSize);
	}

	public Bitmap loadBitmap (byte[] data)
	{
		return BitmapFactory.decodeByteArray (data, 0, data.length, null);
	}

	public Bitmap createBitmap (int width, int height, boolean hasAlpha)
	{
		Bitmap bitmap = Bitmap.createBitmap (width, height, Bitmap.Config.ARGB_8888);
		if(hasAlpha)
			bitmap.eraseColor (Color.TRANSPARENT);
		else
			bitmap.eraseColor (Color.BLACK); // TRANSPARENT would not work in this case! (further drawing into bitmap fails)

		bitmap.setHasAlpha (hasAlpha);
		// bitmap.setPremultiplied (true); // (tried during debugging; doesn't seem to have effect)
		return bitmap;
	}

	public Bitmap createBitmapRaw (int width, int height)
	{
		// create bitmap without initializing pixels (hasAlpha defaults to true for ARGB_8888)
		return Bitmap.createBitmap (width, height, Bitmap.Config.ARGB_8888);
	}

	public boolean saveBitmap (long nativeStream, Bitmap bitmap, String mimeType, int quality)
	{
		Bitmap.CompressFormat format;
		if(mimeType.equals ("image/png"))
			format = Bitmap.CompressFormat.PNG;
		else if(mimeType.equals ("image/jpeg"))
			format = Bitmap.CompressFormat.JPEG;
		else
			return false; // note: android also supports Bitmap.CompressFormat.WEBP_LOSSLESS, Bitmap.CompressFormat.WEBP_LOSSY

		OutputStreamWrapper outputStream = new OutputStreamWrapper (nativeStream);
		return bitmap.compress (format, quality, outputStream);
	}

	public Typeface loadFont (byte[] data)
	{
		try
		{
			// copy to temporary file (hmm...)
			File tempFile = File.createTempFile ("font", ".ttf", null);
			FileOutputStream stream = new FileOutputStream (tempFile.getPath ());
			stream.write (data);
			stream.close ();

			Typeface typeface = Typeface.createFromFile (tempFile);
			tempFile.delete ();
			return typeface;
		}
		catch (IOException e)
		{
		}
		return null;
	}

	public static TextPaint createTextPaint ()
	{
		TextPaint textPaint = new TextPaint ();
		textPaint.setAntiAlias (true); // anti-aliasing for text is always enabled
		textPaint.setHinting (Paint.HINTING_OFF); // turn off hinting, otherwise our custom font looks poor
		textPaint.setSubpixelText (true);
		return textPaint;
	}

	public Paint createCachedBitmapPaint (int index, int alpha, boolean filtered)
	{
		Paint paint = bitmapPaints.createPaint (index);
		paint.setAlpha (alpha);
		paint.setFilterBitmap (filtered);
		paint.setAntiAlias (false);
		return paint;
	}

	public Paint createCachedFillPaint (int index, int color, boolean antiAlias)
	{
		Paint paint = fillPaints.createPaint (index);
		paint.setColor (color);
		paint.setAntiAlias (antiAlias);
		return paint;
	}

	public Paint createCachedDrawPaint (int index, int color, float width, int penStyle, boolean antiAlias)
	{
		Paint paint = drawPaints.createPaint (index);
		paint.setColor (color);
		paint.setStrokeWidth (width);
		paint.setAntiAlias (antiAlias);

		Paint.Cap cap = Paint.Cap.BUTT;
		if((penStyle & GraphicsConstants.kPenLineCapSquare) != 0)
			cap = Paint.Cap.SQUARE;
		else if((penStyle & GraphicsConstants.kPenLineCapRound) != 0)
			cap = Paint.Cap.ROUND;
 		
		Paint.Join join = Paint.Join.MITER;
		if((penStyle & GraphicsConstants.kPenLineJoinBevel) != 0)
			join = Paint.Join.BEVEL;
		else if((penStyle & GraphicsConstants.kPenLineJoinRound) != 0)
			join = Paint.Join.ROUND;

		paint.setStrokeCap (cap);
		paint.setStrokeJoin (join);
		return paint;
	}

	public Paint createCachedTextPaint (int index, Typeface typeface, int style, float fontSize, float spacing, int color)
	{
		Paint paint = textPaints.createPaint (index);

		if(typeface != null)
			paint.setTypeface (typeface);
		else
		{
			paint.setTypeface (Typeface.SANS_SERIF);

			// serif font makes it obvious where custom font is missing
			//paint.setTypeface (Typeface.SERIF);
			//CCL.log ("Typeface not specified!");
		}

		paint.setUnderlineText  ((style & GraphicsConstants.kFontStyleUnderline) != 0);
		paint.setStrikeThruText ((style & GraphicsConstants.kFontStyleStrikeout) != 0);

		paint.setTextSize (fontSize);
		paint.setLetterSpacing (spacing);

		paint.setColor (color);
		return paint;
	}

	public Paint createLinearGradientPaint (float x0, float y0, float x1, float y1, int[] colors, float[] positions)
	{
		Paint gradientPaint = new Paint ();
		gradientPaint.setShader (new LinearGradient (x0, y0, x1, y1, colors, positions, Shader.TileMode.CLAMP));
		return gradientPaint;
	}

	public Paint createRadialGradientPaint (float centerX, float centerY, float radius, int[] colors, float[] positions)
	{
		Paint gradientPaint = new Paint ();
		gradientPaint.setShader (new RadialGradient (centerX, centerY, radius, colors, positions, Shader.TileMode.CLAMP));
		return gradientPaint;
	}
}
