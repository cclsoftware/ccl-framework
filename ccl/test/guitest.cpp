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
// Filename    : guitest.h
// Description : GUI Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/app/params.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/framework/idialogbuilder.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/system/logging.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// GUITestSuite
//************************************************************************************************

CCL_TEST (GUITestSuite, TestSprite)
{
	ISprite* sprite = ccl_new<ISprite> (ClassID::FloatingSprite);
	CCL_TEST_ASSERT (sprite != nullptr);
	if(!sprite)
		return;

	// TODO!
	//sprite->construct (view, size);

	sprite->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (GUITestSuite, TestClipboard)
{
	IClipboard& clipboard = System::GetClipboard ();
	static const String text = CCLSTR ("This text will be copied to System Clipboard!");
	CCL_TEST_ASSERT (clipboard.setText (text) != 0);
	String temp;
	CCL_TEST_ASSERT (clipboard.getText (temp) != 0);
	CCL_TEST_ASSERT (temp == text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (GUITestSuite, TestParam)
{
	Logging::debug ("convex");
	AutoPtr<ParamCurve> c = NEW ConvexCurve ();
	for(float v = 0; v <= 1.f; v += 0.05f)
	{
		Logging::debugf ("v %f %f %f \n", v, c->displayToNormalized (v), c->normalizedToDisplay (c->displayToNormalized (v)));
	}

	Logging::debug ("concav");
	c = NEW ConcaveCurve ();
	for(float v = 0; v <= 1.f; v += 0.05f)
	{
		Logging::debugf ("v %f %f %f \n", v, c->displayToNormalized (v), c->normalizedToDisplay (c->displayToNormalized (v)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (GUITestSuite, TestDpiScale)
{
	Rect r1, r2, r3;

	for(int convertDirection = 0; convertDirection < 2; convertDirection++)
	{
		for(int dpiIdx = 0; dpiIdx <= 3; dpiIdx++)
		{
			float scaleFactor = 1;
			switch(dpiIdx)
			{
			case 0: scaleFactor = 1; break;
			case 1: scaleFactor = 1.25f; break;
			case 2: scaleFactor = 1.5f; break;
			case 3: scaleFactor = 2; break;
			}

			for(int rectSizeIdx = 0; rectSizeIdx <= 3; rectSizeIdx++)
			{
				switch(rectSizeIdx)
				{
				case 0: r1 (10, 10, 10, 10); break;
				case 1: r1 (16, 16, 16, 16); break;
				case 2: r1 (10, 10, 16, 16); break;
				case 3: r1 (16, 16, 10, 10); break;
				}

				r2 = r1;
				if(convertDirection == 0)
				{
					DpiScale::toPixelRect (r2, scaleFactor);
					r3 = r2;
					DpiScale::toCoordRect (r3, scaleFactor);
				}
				else
				{
					DpiScale::toCoordRect (r2, scaleFactor);
					r3 = r2;
					DpiScale::toPixelRect (r3, scaleFactor);
				}

				if(r1 != r3)
				{
					CCL_TEST_ASSERT_EQUAL (r1 , r3);
					Logging::debugf ("r1(%d,%d,%d,%d) r2(%d,%d,%d,%d) r3(%d,%d,%d,%d) scale:%f [%s]\n", r1.left, r1.top, r1.right, r1.bottom, r2.left, r2.top, r2.right, r2.bottom, r3.left, r3.top, r3.right, r3.bottom, scaleFactor, convertDirection ? "toCoord->toPixel" : "toPixel->toCoord");
				}
			}
		}
	}
}

