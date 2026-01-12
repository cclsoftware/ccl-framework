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
// Filename    : ccl/platform/android/graphics/java/FrameworkTextLayout.java
// Description : Framework Text Layout
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.core.CoreGuiConstants;

import android.graphics.*;
import android.text.*;
import android.text.style.*;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

//************************************************************************************************
// BaselineOffsetSpan
//************************************************************************************************

class BaselineOffsetSpan extends MetricAffectingSpan
{
	private final float offset;

	public BaselineOffsetSpan (float offset)
	{
		this.offset = offset;
	}

    @Override
    public void updateDrawState (TextPaint textPaint)
	{
        textPaint.baselineShift += Math.round (textPaint.ascent () * offset);
    }

    @Override
    public void updateMeasureState (@NonNull TextPaint textPaint)
	{
        textPaint.baselineShift += Math.round (textPaint.ascent () * offset);
    }
}

//************************************************************************************************
// LetterSpacingSpan
//************************************************************************************************

class LetterSpacingSpan extends MetricAffectingSpan
{
	private final float spacing;

	public LetterSpacingSpan (float spacing)
	{
		this.spacing = spacing;
	}

    @Override
    public void updateDrawState (TextPaint textPaint)
	{
        textPaint.setLetterSpacing (spacing);
    }

    @Override
    public void updateMeasureState (@NonNull TextPaint textPaint)
	{
        textPaint.setLetterSpacing (spacing);
    }
}

//************************************************************************************************
// FrameworkTextLayout
//************************************************************************************************

@Keep
public class FrameworkTextLayout
{
	private final TextPaint textPaint;
	private Layout layout;
	private final SpannableString spannable;
	private int width;
	private int height;
	private final int align;
	private final int mode;
	private float xOffset = 0;
	private float yOffset = 0;
	private final int options;
	private float fontSize;
	private float lineSpacing;

	public FrameworkTextLayout (String text, int width, int height, int align, int mode, int options, Typeface typeface, int style, float fontSize, float letterSpacing, float lineSpacing)
	{
		this.textPaint = FrameworkGraphicsFactory.createTextPaint ();
		this.spannable = new SpannableString (text);
		this.width = width;
		this.height = height;
		this.align = align;
		this.mode = mode;
		this.options = options;
		this.fontSize = fontSize;
		this.lineSpacing = lineSpacing;

		FrameworkGraphics.applyFont (textPaint, typeface, style, fontSize, letterSpacing);

		createLayout ();
	}

	private void createLayout ()
	{
		textPaint.setTextSize (fontSize);

		// horizontal alignment
		Layout.Alignment alignment = Layout.Alignment.ALIGN_NORMAL;
		switch(align & CoreGuiConstants.kAlignmentHMask)
		{
		case CoreGuiConstants.kAlignmentHCenter :
			alignment = Layout.Alignment.ALIGN_CENTER;
			xOffset = (width - Layout.getDesiredWidth (spannable, textPaint)) / 2;
			break;

		case CoreGuiConstants.kAlignmentRight :
			alignment = Layout.Alignment.ALIGN_OPPOSITE;
			xOffset = width - Layout.getDesiredWidth (spannable, textPaint);
			break;
		}

		// use left alignment for single line layouts to avoid cut off text on some devices
		if(mode == GraphicsConstants.kTextLayoutLineModeSingleLine)
			alignment = Layout.Alignment.ALIGN_NORMAL;

		// use max width for single line layouts to avoid forced line breaks
		int layoutWidth = width;
		if(mode == GraphicsConstants.kTextLayoutLineModeSingleLine)
			layoutWidth = CoreGuiConstants.kCoordLimitsMaxCoord / 2;

		// create layout
		StaticLayout.Builder layoutBuilder = StaticLayout.Builder.obtain (spannable, 0, spannable.length (), textPaint, layoutWidth);

		layoutBuilder.setAlignment (alignment);
		layoutBuilder.setLineSpacing (0, lineSpacing);
		layoutBuilder.setIncludePad (false);

		layout = layoutBuilder.build ();

		// vertical alignment
		switch(align & CoreGuiConstants.kAlignmentVMask)
		{
		case CoreGuiConstants.kAlignmentVCenter :
			yOffset = (height - layout.getHeight ()) / 2;
			break;

		case CoreGuiConstants.kAlignmentBottom :
			yOffset = height - layout.getHeight ();
			break;
		}
	}

	public void setFontStyle (int start, int end, int style, boolean state)
	{
		//CCL.log ("setFontStyle (" + start + ", " + end + "): " + style + ", " + state);
		int boldItalic = style & (GraphicsConstants.kFontStyleBold | GraphicsConstants.kFontStyleItalic);
		if(boldItalic != 0)
		{
			int s = Typeface.BOLD_ITALIC;
			if(boldItalic == GraphicsConstants.kFontStyleItalic)
				s = Typeface.ITALIC;
			else if(boldItalic == GraphicsConstants.kFontStyleBold)
				s = Typeface.BOLD;

			if(state)
				spannable.setSpan (new StyleSpan (s), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			else
			{
				// to turn a style off,  we have to remove the span (flags in multiple StyleSpans accumulate)
				StyleSpan[] spans = spannable.getSpans (start, end, StyleSpan.class);
				for(StyleSpan span : spans)
					if((span.getStyle () & s) != 0)
						spannable.removeSpan (span);
			}
		}

		if((style & GraphicsConstants.kFontStyleUnderline) != 0)
		{
			if(state)
				spannable.setSpan (new UnderlineSpan (), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			else
				removeSpansInRange (start, end, UnderlineSpan.class);
		}

		if((style & GraphicsConstants.kFontStyleStrikeout) != 0)
		{
			if(state)
				spannable.setSpan (new StrikethroughSpan (), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			else
				removeSpansInRange (start, end, StrikethroughSpan.class);
		}
	}

	public void setFontSize (int start, int end, int size)
	{
		boolean isFullRange = start <= 0 && end >= spannable.length () - 1;
		if(isFullRange)
		{
			removeSpansInRange (start, end, AbsoluteSizeSpan.class);
			this.fontSize = size;
		}
		else
			spannable.setSpan (new AbsoluteSizeSpan (size), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

		createLayout ();
	}

	public void setSpacing (int start, int end, float spacing)
	{
		spannable.setSpan (new LetterSpacingSpan (spacing), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
	}

	public void setLineSpacing (int start, int end, float lineSpacing)
	{
		boolean isFullRange = start <= 0 && end >= spannable.length () - 1;
		if(isFullRange)
		{
			// full text range is affected: remove all LineHeightSpans and recreate layout with the new spacing
			removeSpansInRange (start, end, LineHeightSpan.class);

			if(lineSpacing != this.lineSpacing)
			{
				this.lineSpacing = lineSpacing;
				createLayout ();
			}
		}
		else
		{
			int height = Math.round (fontSize * lineSpacing);
			spannable.setSpan (new LineHeightSpan.Standard (height), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
		}
	}

	public void setBaselineOffset (int start, int end, float offset)
	{
		spannable.setSpan (new BaselineOffsetSpan (offset), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
	}

	public void setRelativeSize (int start, int end, float size)
	{
		spannable.setSpan (new RelativeSizeSpan (size), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
	}

	public void setTextColor (int start, int end, int color)
	{
		spannable.setSpan (new ForegroundColorSpan (color), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
	}

	private <T> void removeSpansInRange (int start, int end, Class<T> kind)
	{
		T[] spans = spannable.getSpans (start, end, kind);
		for(T span : spans)
			spannable.removeSpan (span);
	}

	public int getLineForOffset (int index) { return layout.getLineForOffset (index); }
	public int getLineStart (int line) { return layout.getLineStart (line); }
	public int getLineEnd (int line) { return layout.getLineEnd (line); }
	
	public int getOffsetToLeftOf (int offset) { return layout.getOffsetToLeftOf (offset); }
	public int getOffsetToRightOf (int offset) { return layout.getOffsetToRightOf (offset); }

	public float getWidth () { return Layout.getDesiredWidth (spannable, textPaint); }
	public int getHeight () { return layout.getHeight (); }

	public int getBaseline () { return layout.getLineBaseline (0); }

	public void getBounds (Rect size)
	{
		RectF sizeF = new RectF ();
		getBoundsF (sizeF);

		size.left = (int) Math.floor (sizeF.left);
		size.top = (int) Math.floor (sizeF.top);
		size.right = (int) Math.ceil (sizeF.right);
		size.bottom = (int) Math.ceil (sizeF.bottom);
	}

	public void getBoundsF (RectF size)
	{
		float desiredWidth = Layout.getDesiredWidth (spannable, textPaint);
		float layoutWidth = desiredWidth;
		
		if((options & GraphicsConstants.kTextFormatOptionsWordBreak) != 0)
			layoutWidth = Math.min (desiredWidth, layout.getWidth ());

		size.left = xOffset;
		size.top = yOffset;
		size.right = xOffset + layoutWidth;
		size.bottom = yOffset + layout.getHeight ();
	}

	public void getImageBounds (RectF size)
	{
		Path path = new Path ();
		textPaint.getTextPath (spannable.toString (), 0, spannable.length (), xOffset, -textPaint.ascent () + yOffset, path);
		path.computeBounds (size, true);
	}
	
	public void getRangeBounds (RectF bounds, int start, int end)
	{
		Path path = new Path ();
		layout.getSelectionPath (start, end, path);
		path.computeBounds (bounds, true);

		bounds.top += yOffset;
		bounds.bottom += yOffset;
	}

	public int getOffsetForPosition (float x, float y)
	{
		int line = layout.getLineForVertical (Math.round (y));
		int offset = layout.getOffsetForHorizontal (line, x);
		return layout.getLineStart (line) + offset;
	}

	public void draw (FrameworkGraphics graphics, float x, float y, int color, int options)
	{
		float baselineOffset = 0;
		if((options & GraphicsConstants.kDrawTextOptionsDrawAtBaseline) != 0)
			baselineOffset = -getBaseline (); // translate y from top to baseline

		textPaint.setColor (color);
		Canvas canvas = graphics.getCanvas ();

		canvas.save ();
		if(mode == GraphicsConstants.kTextLayoutLineModeSingleLine)
			canvas.translate (x + xOffset, y + yOffset + baselineOffset);
		else
			canvas.translate (x, y + yOffset + baselineOffset);
		layout.draw (canvas);
		canvas.restore ();
	}

	public boolean resize (int width, int height)
	{
		if(width == this.width && height == this.height)
			return true;

		this.width = width;
		this.height = height;

		createLayout ();
		return true;
	}
}
