//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/blocks/markdownblockbuilder.cpp
// Description : Markdown Block Content Builder
//
//************************************************************************************************

#include "ccl/gui/blocks/markdownblockbuilder.h"
#include "ccl/gui/blocks/blockcontent.h"

#include "ccl/base/storage/url.h"

using namespace CCL;

#define MARKDOWN_NOT_IMPL(type) CCL_NOT_IMPL ("Markdown node type "#type" not implemented\n")

//************************************************************************************************
// MarkdownTextBlockBuilder
//************************************************************************************************

MarkdownBlockContentBuilder::MarkdownBlockContentBuilder (BlockContentBuilder& blockBuilder,
														  BlockContentNode& parentNode)
: blockBuilder (blockBuilder),
  parentNode (parentNode),
  lineStartPos (0)
{
	nodeItems.objectCleanup ();

	openParentNodes.push (&parentNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentNode& MarkdownBlockContentBuilder::getCurrentParentNode () const
{
	auto* currentParentNode = static_cast<BlockContentNode*> (openParentNodes.peek ());
	ASSERT (currentParentNode)
	if(!currentParentNode)
		currentParentNode = &parentNode;

	return *currentParentNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MarkdownBlockContentBuilder::getCurrentTextPosition () const
{
	return plainText.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::startNewLine (bool force, StringRef prefix)
{
	if(force || lineStartPos != getCurrentTextPosition ())
	{
		if(!plainText.isEmpty ())
			plainText << String::getLineEnd ();

		plainText << prefix;
		lineStartPos = getCurrentTextPosition ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::startNewParagraph ()
{
	if(!plainText.isEmpty ())
	{
		startNewLine ();

		auto* nodeItem = static_cast<NodeItem*> (openNodes.peek ());
		if(!nodeItem || nodeItem->getNodeType () != Text::kListItem)
			plainText << String::getLineEnd ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::addText (StringRef text)
{
	plainText << text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::addNodeText (const IMarkdownNode& node)
{
	String text;
	node.getText (text);
	addText (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkdownBlockContentBuilder::NodeItem& MarkdownBlockContentBuilder::openNode (TextNodeType nodeType)
{
	auto* nodeItem = NEW NodeItem (nodeType);
	nodeItem->setStartPos (getCurrentTextPosition ());

	nodeItems.add (nodeItem);
	openNodes.push (nodeItem);
	return *nodeItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::closeNode (TextNodeType nodeType)
{
	auto* nodeItem = static_cast<NodeItem*> (openNodes.peek ());
	ASSERT (nodeItem && nodeItem->getNodeType () == nodeType)
	if(nodeItem && nodeItem->getNodeType () == nodeType)
	{
		openNodes.pop ();

		int currentPos = getCurrentTextPosition ();
		nodeItem->setEndPos (currentPos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkdownBlockContentBuilder::NodeItem* MarkdownBlockContentBuilder::findOpenNode (TextNodeType type) const
{
	return static_cast<NodeItem*> (openNodes.findIf ([&] (const Object* obj)
	{
		return static_cast<const NodeItem*> (obj)->getNodeType () == type;
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerContentNode& MarkdownBlockContentBuilder::openContainerNode (TextNodeType nodeType)
{
	startNewLine ();
	flushBlock ();
	openNode (nodeType);

	auto* containerNode = NEW ContainerContentNode (nodeType);
	getCurrentParentNode ().getChildren ().insertNode (containerNode);
	openParentNodes.push (containerNode);

	return *containerNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::closeContainerNode (TextNodeType nodeType)
{
	closeNode (nodeType);
	flushBlock ();
	ASSERT (ccl_cast<ContainerContentNode> (&getCurrentParentNode ()))
	openParentNodes.pop ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::flushBlock ()
{
	// remove trailing line endings
	while(plainText.endsWith (String::getLineEnd (Text::kCRLineFormat)) || plainText.endsWith (String::getLineEnd (Text::kLFLineFormat)))
		plainText.remove (plainText.length () -1, 1);

	if(!plainText.isEmpty ())
	{
		AutoPtr<FormattedText> formattedText = NEW FormattedText (plainText);

		int position = getCurrentTextPosition ();

		ObjectArray toRemove;
		for(auto* nodeItem : iterate_as<NodeItem> (nodeItems))
		{
			if(openNodes.contains (nodeItem))
			{
				// offset positions of remaining nodes
				nodeItem->setStartPos (nodeItem->getStartPos () - position);
				nodeItem->setEndPos (nodeItem->getEndPos () - position);
			}
			else
			{
				applyFormatting (*formattedText, *nodeItem);
				toRemove.add (nodeItem);
			}
		}

		for(auto* nodeItem : iterate_as<NodeItem> (toRemove))
			if(nodeItems.remove (nodeItem))
				nodeItem->release ();

		auto* newNode = NEW TextContentNode;
		newNode->setFormattedText (formattedText);
		getCurrentParentNode ().getChildren ().insertNode (newNode);

		plainText.empty ();
		lineStartPos = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkdownBlockContentBuilder::applyFormatting (FormattedText& formattedText, const NodeItem& nodeItem)
{
	switch(nodeItem.getNodeType ())
	{
	case Text::kHeading :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kHeading, nodeItem.getArgument ());
		break;

	case Text::kBlockQuote :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kStyleSpan, CCLSTR (TextStyles::kBlockQuote));
		break;

	case Text::kCodeBlock :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kStyleSpan, CCLSTR (TextStyles::kCodeBlock));
		break;

	case Text::kCodeInline :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kStyleSpan, CCLSTR (TextStyles::kCode));
		break;

	case Text::kEmphasis :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kItalic, true);
		break;

	case Text::kStrong :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kBold, true);
		break;

	case Text::kLink :
		formattedText.addFormatRange (nodeItem.getStartPos (), nodeItem.getLength (), Text::kLink, nodeItem.getArgument ()); // argument is url
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkdownBlockContentBuilder::startNode (const IMarkdownNode& node)
{
	static const String kSpace (CCLSTR (" "));
	static const String kListBullet (Text::kUTF8, u8"\u2022");

	TextNodeType nodeType = node.getNodeType ();

	switch(nodeType)
	{
	case Text::kDocument :
		break;

	case Text::kBlockQuote :
		openContainerNode (nodeType);
		break;

	case Text::kList :
		{
			startNewParagraph ();
			NodeItem& nodeItem = openNode (nodeType);
			if(node.getListType () == Text::kOrderedList)
				nodeItem.setArgument (node.getListStart ());
			else
				nodeItem.setArgument (-1);
		}
		break;

	case Text::kListItem :
		{
			String marker;

			NodeItem* listNodeItem = findOpenNode (Text::kList);
			int number = listNodeItem ? listNodeItem->getArgument ().asInt () : -1;
			if(number >= 0)
			{
				marker << number << ".";
				if(number < 10)
					marker << kSpace; // reserve space for at least 2 digits
				listNodeItem->setArgument (number + 1); // increase for next item
			}
			else
				marker << kListBullet;

			openContainerNode (nodeType).setMarker (marker);
		}
		break;

	case Text::kCodeBlock :
		openContainerNode (nodeType);
		addNodeText (node);
		closeContainerNode (nodeType);
		break;

	case Text::kParagraph :
		startNewParagraph ();
		break;

	case Text::kHeading :
		startNewParagraph ();
		openNode (nodeType).setArgument (node.getHeadingLevel ());
		break;

	case Text::kPlainText :
		addNodeText (node);
		break;

	case Text::kSoftBreak :
		startNewLine ();
		break;

	case Text::kLineBreak :
		startNewLine ();
		break;

	case Text::kCodeInline :
		openNode (nodeType);
		addNodeText (node);
		closeNode (nodeType);
		break;

	case Text::kEmphasis :
	case Text::kStrong :
		openNode (nodeType);
		break;

	case Text::kLink :
	case Text::kImage :
		{
			NodeItem& nodeItem = openNode (nodeType);

			// note: title is typically used as tooltip (link text is in a nested text node)
			String urlString;
			String title;
			node.getUrl (urlString);
			node.getTitle (title);

			Url url;
			if(url.fromDisplayString (urlString))
			{
				AutoPtr<Url> argument;
				if(title.isEmpty ())
					argument = NEW Url (url);
				else
					argument = NEW UrlWithTitle (url, title);

				nodeItem.setArgument (Variant (argument->asUnknown (), true));
			}
		}
		break;

	case Text::kThematicBreak :
		// could later add a node that will create a horizontal line in the layout
		addText (" ");
		startNewLine ();
		break;

	case Text::kHtmlBlock :		MARKDOWN_NOT_IMPL ("HtmlBlock") break;
	case Text::kCustomBlock :	MARKDOWN_NOT_IMPL ("CustomBlock") break;
	case Text::kHtmlInline :	MARKDOWN_NOT_IMPL ("HTMLInline") break;
	case Text::kCustomInline :	MARKDOWN_NOT_IMPL ("CustomInline") break;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkdownBlockContentBuilder::endNode (const IMarkdownNode& node)
{
	TextNodeType nodeType = node.getNodeType ();

	switch(nodeType)
	{
	case Text::kDocument :
		flushBlock ();
		break;

	case Text::kBlockQuote :
		closeContainerNode (nodeType);
		break;

	case Text::kList :
		closeNode (nodeType);
		break;

	case Text::kListItem :
		closeContainerNode (nodeType);
		break;

	case Text::kParagraph :
		break;

	case Text::kHeading :
		closeNode (nodeType);
		startNewLine ();
		break;

	case Text::kEmphasis :
	case Text::kStrong :
		closeNode (nodeType);
		break;

	case Text::kLink :
	case Text::kImage :
		closeNode (nodeType);
		break;

	case Text::kHtmlBlock :		MARKDOWN_NOT_IMPL ("HtmlBlock") break;
	case Text::kCustomBlock :	MARKDOWN_NOT_IMPL ("CustomBlock") break;
	case Text::kCustomInline :	MARKDOWN_NOT_IMPL ("CustomInline") break;

	// no exit event for "leaf" nodes (can't have children)
	case Text::kThematicBreak :
	case Text::kPlainText :
	case Text::kSoftBreak :
	case Text::kLineBreak :
	case Text::kCodeInline :
	case Text::kCodeBlock :
	case Text::kHtmlInline :
		ASSERT (false)
		break;
	}
	return kResultOk;
}
