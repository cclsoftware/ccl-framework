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
// Filename    : ccl/gui/blocks/blockcontent.h
// Description : Block Content
//
//************************************************************************************************

#ifndef _ccl_blockcontent_h
#define _ccl_blockcontent_h

#include "ccl/public/gui/framework/iblockcontent.h"

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/graphics/formattedtext.h"

namespace CCL {

class BlockContentRoot;

//************************************************************************************************
// BlockContentNode
//************************************************************************************************

class BlockContentNode: public Object,
						public IBlockContentNode,
						public IBlockContentChildren
{
public:
	DECLARE_CLASS (BlockContentNode, Object)

	BlockContentNode ();

	BlockContentNode* getParent () const;
	virtual BlockContentRoot* getRoot () const;
	const ObjectArray& getChildArray () const;

	StringID getEffectiveCustomStyleID () const;

	// IBlockContentNode
	IBlockContentChildren& CCL_API getChildren () override;
	IBlockContentNode* CCL_API getParentNode () override;
	void CCL_API setCustomStyleID (StringID id) override;
	StringID CCL_API getCustomStyleID () const override;

	// IBlockContentChildren
	IUnknownIterator* CCL_API createIterator () const override;
	tresult CCL_API insertNode (IBlockContentNode* node, int index = -1) override;
	tresult CCL_API removeNode (IBlockContentNode* node) override;
	void CCL_API removeNodes () override;

	CLASS_INTERFACE2 (IBlockContentNode, IBlockContentChildren, Object)

protected:
	BlockContentNode* parent;
	ObjectArray children;
	MutableCString customStyleId;

	BlockContentNode (const BlockContentNode&);

	void nodeChanged ();
};

//************************************************************************************************
// BlockContentRoot
//************************************************************************************************

class BlockContentRoot: public BlockContentNode,
						public IBlockContentRoot
{
public:
	DECLARE_CLASS (BlockContentRoot, BlockContentNode)

	void nodeChanged (BlockContentNode* node);

	DECLARE_STRINGID_MEMBER (kMakeNodeVisible)

	// BlockContentNode
	BlockContentRoot* getRoot () const override;

	// IBlockContentRoot
	IBlockContentChildren& CCL_API getChildren () override;
	IBlockContentNode* CCL_API getParentNode () override;
	void CCL_API setCustomStyleID (StringID id) override;
	StringID CCL_API getCustomStyleID () const override;
	IBlockContentBuilder* CCL_API createBuilder () override;
	void CCL_API makeNodeVisible (IBlockContentNode* node) override;

	CLASS_INTERFACE (IBlockContentRoot, BlockContentNode)
};

//************************************************************************************************
// TextContentNode
//************************************************************************************************

class TextContentNode: public BlockContentNode
{
public:
	DECLARE_CLASS (TextContentNode, BlockContentNode)

	PROPERTY_SHARED_AUTO (FormattedText, formattedText, FormattedText)

	void setText (StringRef text);
	StringRef getText () const;

	void addFormatRange (int start, int length, TextNodeType type, VariantRef argument = Variant ());
	const ObjectArray& getFormatRanges () const;
};

//************************************************************************************************
// ImageContentNode
//************************************************************************************************

class ImageContentNode: public BlockContentNode
{
public:
	DECLARE_CLASS (ImageContentNode, BlockContentNode)

	PROPERTY_SHARED_AUTO (IImage, contentImage, ContentImage)
};

//************************************************************************************************
// ContainerContentNode
//************************************************************************************************

class ContainerContentNode: public BlockContentNode
{
public:
	DECLARE_CLASS (ContainerContentNode, BlockContentNode)

	ContainerContentNode (TextNodeType nodeType = Text::kNodeTypeUnknown);

	PROPERTY_VARIABLE (TextNodeType, nodeType, NodeType);
	PROPERTY_STRING (marker, Marker)
};

//************************************************************************************************
// BlockContentBuilder
//************************************************************************************************

class BlockContentBuilder: public Object,
						   public IBlockContentBuilder
{
public:
	BlockContentBuilder (BlockContentRoot& root);

	TextContentNode* createPlainTextNode (StringRef string);
	TextContentNode* createCCLMarkupNode (StringRef string);
	BlockContentNode* createNodeFromMarkdown (IStream& stream);
	ImageContentNode* createImageNode (IImage* image);

	tresult loadAndInsertContent (VariantRef data, StringID contentType);

	// IBlockContentBuilder
	tresult CCL_API loadContent (IBlockContentNode*& newNode, VariantRef data, StringID contentType) override;

	CLASS_INTERFACE (IBlockContentBuilder, Object)

protected:
	BlockContentRoot& root;
};

} // namespace CCL

#endif // _ccl_blockcontent_h
