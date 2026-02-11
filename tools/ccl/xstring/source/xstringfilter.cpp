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
// Filename    : xstringfilter.cpp
// Description : Format Filter
//
//************************************************************************************************

#include "xstringfilter.h"

using namespace CCL;
using namespace XString;

//************************************************************************************************
// Filter
//************************************************************************************************

Filter::Filter (Bundle& bundle, UrlRef path)
: bundle (bundle),
  path (path)
{}

//************************************************************************************************
// XmlFilter
//************************************************************************************************

XmlFilter::XmlFilter (Bundle& bundle, UrlRef path)
: Filter (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlFilter::create ()
{
	AutoPtr<XmlNode> xmlNode = createNode ();
	ASSERT (xmlNode != nullptr)
	if(xmlNode == nullptr)
		return false;

	XmlTreeWriter writer;
	writer.setTextEnabled (true);
	return writer.writeDocument (path, *xmlNode);
}

//************************************************************************************************
// ReferenceFilter
//************************************************************************************************

ReferenceFilter::ReferenceFilter (Bundle& bundle, UrlRef path)
: XmlFilter (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* ReferenceFilter::createNode ()
{
	static const String kBundle (CCLSTR ("TranslationReferences"));
	static const String kTranslated (CCLSTR ("Translated"));
	static const String kReference (CCLSTR ("Reference"));

	static const CString kKey (CSTR ("key"));
	static const CString kScope (CSTR ("scope"));
	static const CString kLine (CSTR ("line"));
	static const String kFile (CCLSTR ("file"));

	XmlNode* bundleNode = NEW XmlNode (kBundle);
	
	ForEach (bundle, Translated, t)
		XmlNode* tNode = NEW XmlNode (kTranslated);
		bundleNode->addChild (tNode);
		tNode->setAttributeCString (kKey, t->getKey ());
		
		IterForEach (t->getReferences (), Reference, r)
			XmlNode* refNode = NEW XmlNode (kReference);
			tNode->addChild (refNode);

			if(!r->getScope ().isEmpty ())
				refNode->setAttributeCString (kScope, r->getScope ());
			if(!r->getFileName ().isEmpty ())
				refNode->setAttribute (kFile, r->getFileName ());
			if(r->getLineNumber () != 0)
			{
				MutableCString line;
				line.appendFormat ("%d", r->getLineNumber ());
				refNode->setAttributeCString (kLine, line);
			}
		EndFor
	EndFor

	return bundleNode;
}

//************************************************************************************************
// PrototypeFilter
//************************************************************************************************

PrototypeFilter::PrototypeFilter (Bundle& bundle, UrlRef path)
: XmlFilter (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* PrototypeFilter::createNode ()
{
	static const String kTranslations (CCLSTR ("Translations"));
	static const String kTranslation (CCLSTR ("T"));
	
	static const CString kKey (CSTR ("key"));
	static const CString kText (CSTR ("text"));

	XmlNode* rootNode = NEW XmlNode (kTranslations);

	ForEach (bundle, Translated, t)
		String comment;
		Translated::ScopeList scopes;
		t->getScopes (scopes);
		VectorForEach (scopes, MutableCString, scope)
			if(!comment.isEmpty ())
				comment << ", ";
			comment << scope;
		EndFor

		XmlNode* tNode = NEW XmlNode (kTranslation);
		tNode->setComment (comment);
		tNode->setAttributeCString (kKey, t->getKey ());
		tNode->setAttributeCString (kText, nullptr);
		rootNode->addChild (tNode);
	EndFor

	return rootNode;
}
