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
// Filename    : graphicstest.cpp
// Description : UnitTests for Graphic system
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"

#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// GraphicsTestSuite
//************************************************************************************************

class GraphicsTestSuite: public ParameterizedTest<bool>
{
public:
	GraphicsTestSuite ()
	{
		// antialias disabled/enabled
		addTestValue (true);
		addTestValue (false);
	}
	
	void setUp () override
	{
		testColor = Colors::kRed;
	}
	
	void tearDown () override
	{
		image.release();
	}
	
protected:
	Color testColor;
	AutoPtr<IImage> image;
	static const uint32 kSurfaceSize = 10;

	IGraphics* createTestGraphics (int width, int height, float scaleFactor = 1.f)
	{
		if(image != nullptr)
			return nullptr;

		image = GraphicsFactory::createBitmap (width, height, IBitmap::kRGBAlpha, scaleFactor);
		if(image == nullptr)
			return nullptr;

		return GraphicsFactory::createBitmapGraphics (image);
	}

	void testBitmap (const char* expectedPixelMatrix, CStringPtr context)
	{
		UnknownPtr<IBitmap> bitmap (image);

		CCL_TEST_ASSERT (bitmap.isValid ());
		if(!bitmap.isValid ())
			return;

		int bitmapWidth = image->getWidth ();
		int bitmapHeight = image->getHeight ();
		uint32 testColorInt = uint32 (testColor) & 0x00FFFFFF;
		BitmapLockData bitmapData;

		tresult result = bitmap->lockBits (bitmapData, IBitmap::kRGBAlpha, IBitmap::kLockRead);
		CCL_TEST_ASSERT (result == kResultOk);
		if(result != kResultOk)
			return;

		for(int y = 0; y < bitmapHeight; y++)
		{
			MutableCString expected;
			MutableCString actual;

			for(int x = 0; x < bitmapWidth; x++)
			{
				char expectedPixel = expectedPixelMatrix[y * bitmapWidth + x];
				auto& rgba = bitmapData.rgbaAt (x, y);
				uint32 pixelColor = uint32 (Color (rgba.red, rgba.green, rgba.blue, rgba.alpha)) & 0x00FFFFFF;
				char actualChar = pixelColor == testColorInt ? '1': pixelColor == 0 ? '0' : '2';
				actual.append (actualChar);

				if(expectedPixel == 4)
					expected.append (actualChar);
				else if(expectedPixel == 3)
				{
					if(pixelColor == 0)
						expected.append ('3');
					else
						expected.append (actualChar);
				}
				else
				{
					expected.append (expectedPixel == 0 ? '0' : expectedPixel == 1 ? '1' : expectedPixel == 2   ? '2' : '?');
				}
			}

			CCL_TEST_ASSERT_EQUAL (expected, actual);
		}

		bitmap->unlockBits (bitmapData);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Horizontal)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Point p1 (1, 1);
		Point p2 (9, 1);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-1-H");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Vertical)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Point p1 (1, 1);
		Point p2 (1, 9);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-1-V");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia1NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		Point p1 (1, 1);
		Point p2 (7, 7);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia-NA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia1AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (1, 1);
		Point p2 (7, 7);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		4, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 4, 3, 4, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia2NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		Point p1 (7, 7);
		Point p2 (13, 1);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia2-NA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia2AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (7, 7);
		Point p2 (13, 1);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 4,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 4, 3, 4, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia2-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia3NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		Point p1 (7, 7);
		Point p2 (13, 13);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia3-NA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia3AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (7, 7);
		Point p2 (13, 13);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 4, 3, 4, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 4,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia3-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia4NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		Point p1 (7, 7);
		Point p2 (1 , 13);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia4-NA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine1Dia4AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (15, 15))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (7, 7);
		Point p2 (1 , 13);

		Pen pen (testColor, 1);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[15 * 15] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 4, 3, 4, 4, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		4, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	testBitmap (expectedPixelMatrix, "Line-1-Dia4-AA");
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2HorizontalNA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		Point p1 (1, 1);
		Point p2 (9, 1);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-H");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2HorizontalAA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (1, 1);
		Point p2 (9, 1);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 2, 2, 2, 2, 2, 2, 2, 2, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-H-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2Horizontal2NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		Point p1 (1, 2);
		Point p2 (9, 2);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-H2");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2Horizontal2AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (1, 2);
		Point p2 (9, 2);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 2, 2, 2, 2, 2, 2, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-H2-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2VerticalNA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		Point p1 (1, 1);
		Point p2 (1, 9);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-V");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2VerticalAA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (1, 1);
		Point p2 (1, 9);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-V-AA");
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2Vertical2NA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		Point p1 (2, 1);
		Point p2 (2, 9);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-V2");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestLine2Vertical2AA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		AntiAliasSetter smoother (*drawDevice);

		Point p1 (2, 1);
		Point p2 (2, 9);

		Pen pen (testColor, 2);
		drawDevice->drawLine (p1, p2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Line-2-V2-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRectFill)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Rect r (1, 1, 9, 9);
		SolidBrush brush (testColor);
		drawDevice->fillRect (r, brush);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-Fill");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1Frame)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Rect r (1, 1, 9, 9);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-1-Frame");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1FrameEven)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Rect r (0, 0, 8, 8);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
		1, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-1-Frame Even");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1FrameScaled1_5Even)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 1.5f);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 1.5))
	{
		Rect r (0, 0, 8, 8);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 1.5 - even pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1FrameScaled1_5Odd)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 1.5f);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 1.5f))
	{
		Rect r (1, 1, 9, 9);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0,
		4, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 4, 0,
		4, 1, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 4, 0,
		4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-1-Scaled 1.5 odd pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1FrameScaled2Even)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 2);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 2.f))
	{
		Rect r (0, 0, 8, 8);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 2.0 - even pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect1FrameScaled2Odd)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 2);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 2.f))
	{
		Rect r (1, 1, 9, 9);
		Pen pen (testColor);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 2.0 - odd pos");
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameScaled1_5Even)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 1.5f);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 1.5))
	{
		Rect r (0, 0, 8, 8);
		Pen pen (testColor,2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 1.5 - even pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameScaled1_5Odd)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 1.5f);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 1.5f))
	{
		Rect r (1, 1, 9, 9);
		Pen pen (testColor, 2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 0, 0, 0, 0, 0, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 0, 0, 0, 0, 0, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 0, 0, 0, 0, 0, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 0, 0, 0, 0, 0, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 0, 0, 0, 0, 0, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-1-Scaled 1.5 odd pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameScaled2Even)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 2);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 2.f))
	{
		Rect r (0, 0, 8, 8);
		Pen pen (testColor, 2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 2.0 - even pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameScaled2Odd)
{
	const int kSurfaceSizeScaled = int(kSurfaceSize * 2);
	
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSizeScaled, kSurfaceSizeScaled, 2.f))
	{
		Rect r (1, 1, 9, 9);
		Pen pen (testColor, 2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSizeScaled * kSurfaceSizeScaled] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	testBitmap (expectedPixelMatrix, "Rect-Scaled 2.0 - odd pos");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameNA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		Rect r (1, 1, 9, 9);
		Pen pen (testColor, 2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 
		0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 
		0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 
		0, 1, 1, 0, 0, 0, 0, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-2-Frame");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect2FrameAA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		AntiAliasSetter smoother (*drawDevice);

		Rect r (1, 1, 9, 9);
		Pen pen (testColor, 2);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 
		2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 
		2, 1, 2, 0, 0, 0, 0, 2, 1, 2, 
		2, 1, 2, 0, 0, 0, 0, 2, 1, 2, 
		2, 1, 2, 0, 0, 0, 0, 2, 1, 2, 
		2, 1, 2, 0, 0, 0, 0, 2, 1, 2,
		2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 
		2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	};

	testBitmap (expectedPixelMatrix, "Rect-2-Frame-AA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect3Frame)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize, kSurfaceSize))
	{
		bool antiAlias = getTestValue ();
		AntiAliasSetter smoother (*drawDevice, antiAlias);

		Rect r (1, 1, 9, 9);
		Pen pen (testColor, 3);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * kSurfaceSize] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	testBitmap (expectedPixelMatrix, "Rect-3-Frame");
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect4FrameNA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize * 2, kSurfaceSize * 2))
	{
		Rect r (3, 3, 17, 17);
		Pen pen (testColor, 4);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * 2 * kSurfaceSize * 2] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-2-Frame-NA");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRect4FrameAA)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize * 2, kSurfaceSize * 2))
	{
		AntiAliasSetter smoother (*drawDevice);

		Rect r (3, 3, 16, 16);
		Pen pen (testColor, 4);
		drawDevice->drawRect (r, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * 2 * kSurfaceSize * 2] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "Rect-4-Frame-AA");
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRoundRect1Frame)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize * 2, kSurfaceSize * 2))
	{
		AntiAliasSetter smoother (*drawDevice);

		Rect r (2, 2, 18, 18);
		Pen pen (testColor, 1);
		drawDevice->drawRoundRect (r, 2 , 2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * 2 * kSurfaceSize * 2] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0,
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0,
		0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "RoundRect-1-Frame");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRoundRect2Frame)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize * 2, kSurfaceSize * 2))
	{
		AntiAliasSetter smoother (*drawDevice);

		Rect r (2, 2, 18, 18);
		Pen pen (testColor, 2);
		drawDevice->drawRoundRect (r, 2 , 2, pen);
	}

	char expectedPixelMatrix[kSurfaceSize * 2 * kSurfaceSize * 2] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 0, 0,
		0, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0,
		0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
		0, 4, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 4, 0,
		0, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 0,
		0, 0, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "RoundRect-2-Frame");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (GraphicsTestSuite, TestRoundRectFill)
{
	if(AutoPtr<IGraphics> drawDevice = createTestGraphics (kSurfaceSize * 2, kSurfaceSize * 2))
	{
		AntiAliasSetter smoother (*drawDevice);

		Rect r (2, 2, 18, 18);
		SolidBrush brush (testColor);
		drawDevice->fillRoundRect (r, 2 , 2, brush);
	}

	char expectedPixelMatrix[kSurfaceSize * 2 * kSurfaceSize * 2] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 0, 0,
		0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0,
		0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0, 0,
		0, 0, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	testBitmap (expectedPixelMatrix, "RoundRect-Fill");
}
