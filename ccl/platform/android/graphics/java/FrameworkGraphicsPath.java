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
// Filename    : ccl/platform/android/graphics/java/FrameworkGraphicsPath.java
// Description : Framework Graphics Path
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.graphics.*;

import androidx.annotation.Keep;

//************************************************************************************************
// FrameworkGraphicsPath
//************************************************************************************************

@Keep
public class FrameworkGraphicsPath
{
	private final Path path = new Path ();

	public Path getPath ()
	{
		return path;
	}

	public void computeBounds (RectF bounds)
	{
		path.computeBounds (bounds, true);
	}

	public void transform (float a0, float a1, float b0, float b1, float t0, float t1)
	{
		float[] values = { a0, b0, t0, a1, b1, t1, 0, 0, 1 };

		Matrix matrix = new Matrix ();
		matrix.setValues (values);
		path.transform (matrix);
	}

	public void moveTo (float x, float y)
	{
		path.moveTo (x, y);
	}

	public void close ()
	{
		path.close ();
	}

	public void lineTo (float x, float y)
	{
		path.lineTo (x, y);
	}

	public void addRect (float left, float top, float right, float bottom)
	{
		path.addRect (left, top, right, bottom, Path.Direction.CCW);
	}

	public void addRoundRect (float left, float top, float right, float bottom, float rx, float ry)
	{
		path.addRoundRect (new RectF (left, top, right, bottom), rx, ry, Path.Direction.CCW);
	}

	public void addBezier (float x1, float y1, float x2, float y2, float x3, float y3)
	{
		path.cubicTo (x1, y1, x2, y2, x3, y3);
	}

	public void addArc (float left, float top, float right, float bottom, float startAngle, float sweepAngle)
	{
		// draw actual circles using bezier curves for improved rendering in PDFs
		if(Math.abs ((right - left) - (bottom - top)) < 0.001 && sweepAngle >= 360.f)
		{
			float hCenter = (left + right) / 2.f;
			float vCenter = (top + bottom) / 2.f;
			float controlOffset = Math.abs (right - left) / 2.f * 0.55f;

			path.moveTo (hCenter, top);
			path.cubicTo (hCenter + controlOffset, top, right, vCenter - controlOffset, right, vCenter);
			path.cubicTo (right, vCenter + controlOffset, hCenter + controlOffset, bottom, hCenter, bottom);
			path.cubicTo (hCenter - controlOffset, bottom, left, vCenter + controlOffset, left, vCenter);
			path.cubicTo (left, vCenter - controlOffset, hCenter - controlOffset, top, hCenter, top);
		}
		else
			path.arcTo (new RectF (left, top, right, bottom), startAngle, sweepAngle);
	}

	public void setFillMode (boolean nonzero)
	{
		path.setFillType (nonzero ? Path.FillType.WINDING : Path.FillType.EVEN_ODD);
	}
}
