//************************************************************************************************
//
// CCL Spy
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
// Filename    : doceditor.cpp
// Description : Documentation Editor
//
//************************************************************************************************

#include "doceditor.h"

#include "ccl/base/storage/xmltree.h"
#include "ccl/base/collections/container.h"

#include "ccl/public/base/istream.h"

#include "ccl/public/gui/iparameter.h"

using namespace Spy;
using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum DocumentationEditorTags
	{
		kTitle = 100,
		kSummary
	};
}

//************************************************************************************************
// DocumentationFile
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentationFile, TextResource)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentationFile::load (IStream& stream)
{
	// load as plain text
	if(!TextResource::load (stream))
		return false;

	// parse XML for preview
	ASSERT (stream.isSeekable ())
	stream.rewind ();
	XmlTreeParser parser;
	parser.setTextEnabled (true);
	if(!parser.parse (stream))
		return false;

	XmlNode* root = parser.getRoot ();
	ASSERT (root != nullptr)
	if(XmlNode* titleNode = root->findNodeCString ("title"))
		setTitle (titleNode->getText ());

	summary.empty ();
	summarize (*root);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool isProgramListing (const XmlNode& node)
{
	static const String programListing ("programlisting");
	if(node.getName () == programListing)
		return true;

	if(XmlNode* parentNode = node.getParentNode ())
		return isProgramListing (*parentNode);
	return false;
}

void DocumentationFile::summarize (const XmlNode& parent)
{
	static const String blank (" ");
	static const String nl (String::getLineEnd ());
	static const String cr (String::getLineEnd (Text::kCRLineFormat));
	static const String lf (String::getLineEnd (Text::kLFLineFormat));
	ForEach (parent, XmlNode, node)
		String text (node->getText ());
		if(isProgramListing (*node))
			summary << text;
		else
		{
			text.replace (cr, blank); // remove line breaks inside text
			text.replace (lf, blank); // remove line breaks inside text
			text.trimWhitespace ();
			if(!text.isEmpty ())
				summary << text << nl << nl;
		}
		summarize (*node);
	EndFor
}

//************************************************************************************************
// DocumentationEditor
//************************************************************************************************

DocumentationEditor::DocumentationEditor ()
: Component ("DocumentationEditor")
{
	paramList.addString ("title", Tag::kTitle);
	paramList.addString ("summary", Tag::kSummary);

	// TODO: add XML edit mode!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationEditor::setFile (DocumentationFile* file)
{
	paramList.byTag (Tag::kTitle)->fromString (file ? file->getTitle () : String::kEmpty);
	paramList.byTag (Tag::kSummary)->fromString (file ? file->getSummary () : String::kEmpty);
}
