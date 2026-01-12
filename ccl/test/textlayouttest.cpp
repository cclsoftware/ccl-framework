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
// Filename    : textlayouttest.cpp
// Description : GUI Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TextLayoutTest
//************************************************************************************************

CCL_TEST (TextLayoutTest, TestProcessLicenseTextDuration)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	Url legalFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (legalFolder, CCL_FRAMEWORK_DIRECTORY "build", "identities/ccl/legal");

	String searchPattern ("*3rd*.txt");
	String text;
	ForEachFile (File::findFiles (legalFolder, searchPattern), path)
		String licenseText = TextUtils::loadString (*path);
		if(!licenseText.isEmpty ())
		{
			if(!text.isEmpty ())
			{
				String mergedText (text);
				mergedText << "\n\n";
				mergedText << licenseText;
				text = mergedText;
			}
			else
				text = licenseText;
		}
	EndFor

	textLayout->construct (text, 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());
	ITextLayout::Range range (0, 0);

	double startCount = System::GetProfileTime ();
	textLayout->getWordRange (range, 0);
	double delta = 1000 * (System::GetProfileTime () - startCount); // in ms
	CCL_TEST_ASSERT (delta < 200);
}
