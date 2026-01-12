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
// Filename    : ccl/network/web/xmlnewsreader.cpp
// Description : Atom/RSS Reader
//
//************************************************************************************************

#include "ccl/network/web/xmlnewsreader.h"

#include "ccl/base/storage/xmltree.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Atom 1.0 Feed Example
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	<feed xmlns="http://www.w3.org/2005/Atom" xml:lang="xx-XX">
		<link rel="self" href="..."/>

		<id>...</id>
		<title> ... </title>
		<author>
			<name> ... </name>
			<email> ... </email>
		</author>
		<updated>yyyy-mm-ddThh:mm:ssZ</updated>

		<entry>
			<link rel="alternate" type="..." href="..."/>
			<link rel="enclosure" type="..." href="..."/>

			<id> ... </id>
			<title> ... </title>
			<category term="..." label="..."/>
			<updated> ... </updated>
			<summary> ... </summary>
			<content> ... </content>
		</entry>
	</feed>
*/

//////////////////////////////////////////////////////////////////////////////////////////////////
// RSS 2.0 Feed Example
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0">
  <channel>
    <title> </title>
    <link> </link>
    <description> </description>
    <generator></generator>

    <item>
      <title> </title>
      <description> </description>
      <link> </link>
      <guid isPermaLink="false">{92AF6BC2-8948-4A95-80AB-EFC7387335CD}</guid>
      <pubDate>Sun, 20 Apr 2008 13:25:01 +0200</pubDate>
    </item>
  </channel>
</rss>
*/

//************************************************************************************************
// XmlNewsReader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (XmlNewsReader, WebNewsReader)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlNewsReader::parseFeed (IStream& stream)
{
	XmlTreeParserWithPI parser; // evaluate processing instructions
	parser.setTextEnabled (true);
	if(!parser.parse (stream))
		return false;

	XmlNode* feedNode = parser.getRoot ();
	if(feedNode == nullptr)
		return false;

	#if (0 && DEBUG)
	feedNode->dump (true);
	#endif

	if(feedNode->getNameCString () == "feed")
		return parseAtom (feedNode);

	if(feedNode->getNameCString () == "rss")
		return parseRSS (feedNode);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNewsReader::setItemAttribute (WebNewsItem& item, StringID id, XmlNode* parent, StringID tagName)
{
	XmlNode* node = parent->findNodeCString (tagName);
	if(node)
	{
		String text (node->getText ());
		text.trimWhitespace ();
		item.addAttribute (id, text);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Atom
//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlNewsReader::parseAtom (XmlNode* feedNode)
{
	// set feed attributes
	WebNewsFeed& feed = getFeed ();

	updateAtomItem (feed, feedNode);

	String language = feedNode->getAttribute ("xml:lang");
	if(!language.isEmpty ())
		feed.addAttribute (Feed::kLanguage, language);

	// collect entries
	ForEach (*feedNode, XmlNode, node)
		if(node->getNameCString () == "entry")
		{
			WebNewsItem& item = *NEW WebNewsItem;
			updateAtomItem (item, node);

			setItemAttribute (item, Feed::kSummary, node, "summary");
			setItemAttribute (item, Feed::kContent, node, "content");

			feed.addItem (&item);
		}
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNewsReader::updateAtomItem (WebNewsItem& item, XmlNode* parent)
{
	setItemAttribute (item, Feed::kID, parent, "id");
	setItemAttribute (item, Feed::kTitle, parent, "title");

	XmlNode* authorNode = parent->findNodeCString ("author");
	if(authorNode)
	{
		setItemAttribute (item, Feed::kAuthorName, authorNode, "name");
		setItemAttribute (item, Feed::kAuthorEmail, authorNode, "email");
	}

	XmlNode* updatedNode = parent->findNodeCString ("updated");
	if(updatedNode)
	{
		String text (updatedNode->getText ());
		text.trimWhitespace ();

		MutableCString string (text);
		int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
		::sscanf (string, "%4d-%2d-%2dT%2d:%2d%2d", &year, &month, &day, &hour, &minute, &second);

		item.setLastUpdated (DateTime (Date (year, month, day), Time (hour, minute, second)));
	}

	XmlNode* categoryNode = parent->findNodeCString ("category");
	if(categoryNode)
	{
		item.addAttribute (Feed::kCategoryTerm, categoryNode->getAttribute (CCLSTR ("term")));
		item.addAttribute (Feed::kCategoryLabel, categoryNode->getAttribute (CCLSTR ("label")));
	}

	// Links
	ForEach (*parent, XmlNode, node)
		if(node->getNameCString () == "link")
		{
			WebNewsLink* link = NEW WebNewsLink;

			link->setRelation (node->getAttribute ("rel"));
			link->setType (node->getAttribute ("type"));
			link->setHRef (node->getAttribute ("href"));

			// non-standard attribute for high-DPI images
			StringRef scalingHint = node->getAttribute ("device-pixel-ratio");
			if(!scalingHint.isEmpty ())
				link->setDevicePixelRatio (scalingHint);

			item.addLink (link);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// RSS
//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlNewsReader::parseRSS (XmlNode* feedNode)
{
	// set feed attributes
	WebNewsFeed& feed = getFeed ();

	updateRSSItem (feed, feedNode);
	setItemAttribute (feed, Feed::kLanguage, feedNode, "language");

	// collect entries
	XmlNode* channelNode = (XmlNode*)feedNode->lookupChild ("channel");
	if(channelNode == nullptr)
		return 0;

	ForEach (*channelNode, XmlNode, node)
		if(node->getNameCString () == "item")
		{
			WebNewsItem& item = *NEW WebNewsItem;
			updateRSSItem (item, node);

			setItemAttribute (item, Feed::kSummary, node, "description");
			feed.addItem (&item);
		}
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNewsReader::updateRSSItem (WebNewsItem& item, XmlNode* parent)
{
	setItemAttribute (item, Feed::kID, parent, "guid");
	setItemAttribute (item, Feed::kTitle, parent, "title");
	setItemAttribute (item, Feed::kAuthorEmail, parent, "author");

	// TODO parse date format, i.e. Sat, 07 Sep 2002 00:00:01 GMT
}
