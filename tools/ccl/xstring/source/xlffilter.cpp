//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : xlffilter.cpp
// Description : XLIFF Filter
//
// See http://docs.oasis-open.org/xliff/xliff-core/xliff-core.pdf
//
//************************************************************************************************

#include "xlffilter.h"

using namespace CCL;
using namespace XString;

/*
<? xml version="1.0" ?>
<xliff version="1.0">
  <file original="sample.html" 
        source-language="en" 
        datatype="HTML Page">
    <header>
      <skl>
        <external-file href="sample.skl"/>
      </skl>
    </header>
    <body>
      <trans-unit id="%%%1%%%">
        <source xml:lang="en">A Title</source>
      </trans-unit>
      <trans-unit id="%%%2%%%">
        <source xml:lang="en">One paragraph</source>
      </trans-unit>
    </body>
  </file>
</xliff>
*/

namespace Xliff
{
	static const String xliff ("xliff");
	static const String file ("file");
	static const String header ("header");
	static const String body ("body");
	static const String transunit ("trans-unit");
	static const String source ("source");
}

//************************************************************************************************
// XliffFilter
//************************************************************************************************

XliffFilter::XliffFilter (Bundle& bundle, UrlRef path)
: XmlFilter (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XliffFilter::createNode ()
{
	XmlNode* rootNode = NEW XmlNode (Xliff::xliff);

	XmlNode* fileNode = NEW XmlNode (Xliff::file);
	rootNode->addChild (fileNode);

	XmlNode* headerNode = NEW XmlNode (Xliff::header);
	fileNode->addChild (headerNode);

	XmlNode* bodyNode = NEW XmlNode (Xliff::body);
	fileNode->addChild (bodyNode);

	ForEach (bundle, Translated, t)
		XmlNode* unitNode = NEW XmlNode (Xliff::transunit);
		
		// Notes on references:
		// see http://www.oasis-open.org/apps/group_public/email/xliff//200805/msg00002.html

		XmlNode* sourceNode = NEW XmlNode (Xliff::source);
		sourceNode->setText (String (t->getKey ()));
		unitNode->addChild (sourceNode);

		bodyNode->addChild (unitNode);
	EndFor

	return rootNode;
}
