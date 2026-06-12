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
// Filename    : ccl/gui/blocks/markdownblockbuilder.h
// Description : Markdown Block Content Builder
//
//************************************************************************************************

#ifndef _ccl_markdownblockbuilder_h
#define _ccl_markdownblockbuilder_h

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectstack.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/imarkdownparser.h"

#include "ccl/public/base/variant.h"

namespace CCL {

class BlockContentNode;
class BlockContentBuilder;
class ContainerContentNode;
class FormattedText;

//************************************************************************************************
// MarkdownBlockContentBuilder
//************************************************************************************************

class MarkdownBlockContentBuilder: public Unknown,
								   public IMarkdownContentHandler
{
public:
	MarkdownBlockContentBuilder (BlockContentBuilder& blockBuilder, BlockContentNode& parentNode);

	// IMarkdownContentHandler
	tresult CCL_API startNode (const IMarkdownNode& node) override;
	tresult CCL_API endNode (const IMarkdownNode& node) override;

	CLASS_INTERFACE (IMarkdownContentHandler, Unknown)

private:
	BlockContentBuilder& blockBuilder;
	BlockContentNode& parentNode;
	String plainText;
	int lineStartPos;

	class NodeItem: public Object
	{
	public:
		NodeItem (TextNodeType nodeType, int argument = 0)
		: nodeType (nodeType), argument (argument), startPos (0), endPos (0)
		{}

		PROPERTY_VARIABLE (TextNodeType, nodeType, NodeType)
		PROPERTY_OBJECT (Variant, argument, Argument)
		PROPERTY_VARIABLE (int, startPos, StartPos)
		PROPERTY_VARIABLE (int, endPos, EndPos)

		int getLength () const { return getEndPos () - getStartPos (); }
	};

	ObjectArray nodeItems;
	ObjectStack openNodes;
	ObjectStack openParentNodes;

	BlockContentNode& getCurrentParentNode () const;
	int getCurrentTextPosition () const;

	void startNewLine (bool force = false, StringRef prefix = nullptr);
	void startNewParagraph ();
	void addText (StringRef text);
	void addNodeText (const IMarkdownNode& node);

	NodeItem& openNode (TextNodeType nodeType);
	void closeNode (TextNodeType nodeType);
	NodeItem* findOpenNode (TextNodeType type) const;

	ContainerContentNode& openContainerNode (TextNodeType nodeType);
	void closeContainerNode (TextNodeType nodeType);

	void applyFormatting (FormattedText& formattedText, const NodeItem& nodeItem);
	void flushBlock ();
};

} // namespace CCL

#endif // _ccl_markdownblockbuilder_h