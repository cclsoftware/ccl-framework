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
// Filename    : ccl/platform/android/graphics/java/FrameworkGraphics.java
// Description : Framework Graphics
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.core.CoreGuiConstants;

import android.graphics.*;
import android.text.*;

import androidx.annotation.Keep;

//************************************************************************************************
// FrameworkGraphics
//************************************************************************************************

@Keep
public class FrameworkGraphics
{
	private final Paint clearPaint;
	private final Matrix matrix;
	private final RectF rectF;
	private final Rect rect1;
	private final Rect rect2;
	private Canvas canvas;

	public FrameworkGraphics ()
	{
		clearPaint = new Paint ();
		matrix = new Matrix ();
		rectF = new RectF ();
		rect1 = new Rect ();
		rect2 = new Rect ();
		
		clearPaint.setXfermode (new PorterDuffXfermode (PorterDuff.Mode.CLEAR));
	}

	public FrameworkGraphics (Bitmap bitmap)
	{
		this ();

		setCanvas (new Canvas (bitmap));
	}

	public void setCanvas (Canvas c)
	{
		canvas = c;
	}

	public Canvas getCanvas ()
	{
		return canvas;
	}

	public boolean isHardwareAccelerated ()
	{
		return canvas.isHardwareAccelerated ();
	}

	public void saveState ()
	{
		canvas.save ();
	}

	public void restoreState ()
	{
		canvas.restore ();
	}

	public void saveStateAndClip (int left, int top, int right, int bottom)
	{
		canvas.save ();
		canvas.clipRect (left, top, right, bottom);
	}

	public void clipRect (int left, int top, int right, int bottom)
	{
		canvas.clipRect (left, top, right, bottom);
	}

	public void clipRectF (float left, float top, float right, float bottom)
	{
		canvas.clipRect (left, top, right, bottom);
	}

	public void clipPath (FrameworkGraphicsPath path)
	{
		canvas.clipPath (path.getPath ());
	}

	public void getClipBounds (Rect rect)
	{
		canvas.getClipBounds (rect);
	}

	public void addTransform (float a0, float a1, float b0, float b1, float t0, float t1)
	{
		float[] values = { a0, b0, t0, a1, b1, t1, 0, 0, 1 };
		matrix.setValues (values);
		canvas.concat (matrix);
	}

	public void translate (float x, float y)
	{
		canvas.translate (x, y);
	}

	public void clearRect (float left, float top, float right, float bottom)
	{
		canvas.drawRect (left, top, right, bottom, clearPaint);
	}

	public void drawRect (float left, float top, float right, float bottom, Paint drawPaint)
	{
		canvas.drawRect (left, top, right, bottom, drawPaint);
	}

	public void fillRect (float left, float top, float right, float bottom, Paint fillPaint)
	{
		canvas.drawRect (left, top, right, bottom, fillPaint);
	}

	public void drawLine (float x1, float y1, float x2, float y2, Paint drawPaint)
	{
		canvas.drawLine (x1, y1, x2, y2, drawPaint);
	}

	public void drawEllipse (float left, float top, float right, float bottom, Paint drawPaint)
	{
		rectF.set (left, top, right, bottom);
		canvas.drawOval (rectF, drawPaint);
	}

	public void fillEllipse (float left, float top, float right, float bottom, Paint fillPaint)
	{
		rectF.set (left, top, right, bottom);
		canvas.drawOval (rectF, fillPaint);
	}

	public void drawPath (FrameworkGraphicsPath path, Paint drawPaint)
	{
		canvas.drawPath (path.getPath (), drawPaint);
	}

	public void fillPath (FrameworkGraphicsPath path, Paint fillPaint)
	{
		canvas.drawPath (path.getPath (), fillPaint);
	}

	public void drawRoundRect (float left, float top, float right, float bottom, float rx, float ry, Paint drawPaint)
	{
		rectF.set (left, top, right, bottom);
		canvas.drawRoundRect (rectF, rx, ry, drawPaint);
	}

	public void fillRoundRect (float left, float top, float right, float bottom, float rx, float ry, Paint fillPaint)
	{
		rectF.set (left, top, right, bottom);
		canvas.drawRoundRect (rectF, rx, ry, fillPaint);
	}

	// todo: still used by FrameworkTextLayout
	public static void applyFont (Paint textPaint, Typeface typeface, int style, float fontSize, float spacing)
	{
		if(typeface != null)
			textPaint.setTypeface (typeface);
		else
		{
			textPaint.setTypeface (Typeface.SANS_SERIF);

			// serif font makes it obvious where custom font is missing
			//textPaint.setTypeface (Typeface.SERIF);
			//CCL.log ("Typeface not specified!");
		}

		textPaint.setUnderlineText  ((style & GraphicsConstants.kFontStyleUnderline) != 0);
		textPaint.setStrikeThruText ((style & GraphicsConstants.kFontStyleStrikeout) != 0);

		textPaint.setTextSize (fontSize);
		textPaint.setLetterSpacing (spacing);
	}

	public void drawString (String text, float x, float y, Paint textPaint, int options)
	{
		if((options & GraphicsConstants.kDrawTextOptionsDrawAtBaseline) == 0)
			y -= textPaint.ascent (); // translate y from baseline to top
		canvas.drawText (text, x, y, textPaint);
	}

	public void measureString (Rect size, String text, Paint textPaint)
	{
		size.left = 0;
		size.right = (int) Math.ceil (textPaint.measureText (text));

		// use vertical metrics based on font, independent from the actual characters (ascent is < 0)
		size.top = 0;
		size.bottom = (int) Math.ceil (textPaint.descent () - textPaint.ascent ());
	}

	public void measureStringF (RectF size, String text, Paint textPaint)
	{
		size.left = 0;
		size.right = textPaint.measureText (text);

		// use vertical metrics based on font, independent from the actual characters (ascent is < 0)
		size.top = 0;
		size.bottom = textPaint.descent () - textPaint.ascent ();
	}

	public float getStringWidth (String text, Paint textPaint)
	{
		return textPaint.measureText (text);
	}

	public void drawText (String text, float x, float y, float width, float height, int align, float lineSpacing, boolean multiLine, Paint textPaint)
	{
		// horizontal alignment (performed by StaticLayout)
		Layout.Alignment alignment = Layout.Alignment.ALIGN_NORMAL;
		switch(align & CoreGuiConstants.kAlignmentHMask)
		{
		case CoreGuiConstants.kAlignmentHCenter : alignment = Layout.Alignment.ALIGN_CENTER; break;
		case CoreGuiConstants.kAlignmentLeft :    alignment = Layout.Alignment.ALIGN_NORMAL; break;
		case CoreGuiConstants.kAlignmentRight :   alignment = Layout.Alignment.ALIGN_OPPOSITE; break;
		}

		// create layout
		StaticLayout layout = new StaticLayout (text, (TextPaint) textPaint, multiLine ? (int) width : CoreGuiConstants.kCoordLimitsMaxCoord / 2, alignment, lineSpacing, 0, false);

		// adjust horizontal offset
		float xOffset = 0;
		if(!multiLine)
		{
			switch(align & CoreGuiConstants.kAlignmentHMask)
			{
			case CoreGuiConstants.kAlignmentHCenter : xOffset = (width - layout.getWidth ()) / 2; break;
			case CoreGuiConstants.kAlignmentRight :   xOffset = (width - layout.getWidth ()); break;
			}
		}

		// vertical alignment
		float yOffset = 0;
		switch(align & CoreGuiConstants.kAlignmentVMask)
		{
		case CoreGuiConstants.kAlignmentVCenter : yOffset = (height - layout.getHeight ()) / 2; break;
		case CoreGuiConstants.kAlignmentBottom :  yOffset = (height - layout.getHeight ()); break;
		}

		// draw layout
		canvas.save ();
		canvas.translate (x, y);
		canvas.clipRect (0, 0, width, height);
		canvas.translate (xOffset, yOffset);
		layout.draw (canvas);
		canvas.restore ();
	}

	public void measureText (Rect size, int width, float lineSpacing, String text, Paint textPaint)
	{
		Layout.Alignment alignment = Layout.Alignment.ALIGN_NORMAL;
		StaticLayout layout = new StaticLayout (text, (TextPaint) textPaint, width, alignment, lineSpacing, 0, false);

		size.left = 0;
		size.top = 0;
		size.right = layout.getWidth ();
		size.bottom = layout.getHeight ();
	}

	public void drawBitmap (Bitmap bitmap, float left, float top, Paint bitmapPaint)
	{
		canvas.drawBitmap (bitmap, left, top, bitmapPaint);
	}

	public void drawBitmapR (Bitmap bitmap, int srcLeft, int srcTop, int srcRight, int srcBottom, int dstLeft, int dstTop, int dstRight, int dstBottom, Paint bitmapPaint)
	{
		rect1.set (srcLeft, srcTop, srcRight, srcBottom);
		rect2.set (dstLeft, dstTop, dstRight, dstBottom);
		canvas.drawBitmap (bitmap, rect1, rect2, bitmapPaint);
	}

	public void drawBitmapDirect (Bitmap bitmap, int srcLeft, int srcTop, int srcRight, int srcBottom)
	{
		rect1.set (srcLeft, srcTop, srcRight, srcBottom);
		canvas.drawBitmap (bitmap, rect1, rect1, null);
	}
}
