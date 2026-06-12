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
// Filename    : ccl/gui/blocks/blockcontent.cpp
// Description : Block Content
//
//************************************************************************************************

#include "ccl/gui/blocks/blockcontent.h"
#include "ccl/gui/blocks/markdownblockbuilder.h"
#include "ccl/gui/graphics/markupsupport.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// BlockContentNode
//************************************************************************************************

DEFINE_CLASS (BlockContentNode, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentNode::BlockContentNode ()
: parent (nullptr)
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentNode::BlockContentNode (const BlockContentNode& other)
: parent (nullptr)
{
	children.objectCleanup (true);	
	children.add (other.children, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentNode* BlockContentNode::getParent () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentRoot* BlockContentNode::getRoot () const
{
	if(parent)
		return parent->getRoot ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& BlockContentNode::getChildArray () const
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockContentNode::nodeChanged ()
{
	if(BlockContentRoot* root = getRoot ())
		root->nodeChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentChildren& CCL_API BlockContentNode::getChildren ()
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentNode* CCL_API BlockContentNode::getParentNode ()
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockContentNode::setCustomStyleID (StringID id)
{
	if(id != customStyleId)
	{
		customStyleId = id;
		nodeChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API BlockContentNode::getCustomStyleID () const
{
	return customStyleId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID BlockContentNode::getEffectiveCustomStyleID () const
{
	if(!customStyleId.isEmpty ())
		return customStyleId;
	if(parent)
		return parent->getEffectiveCustomStyleID ();
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API BlockContentNode::createIterator () const
{
	return children.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BlockContentNode::insertNode (IBlockContentNode* node, int index)
{
	auto* child = unknown_cast<BlockContentNode> (node);
	if(!child)
		return kResultInvalidArgument;

	if(index < 0 || !children.isValidIndex (index))
		index = children.count ();
	
	ASSERT (child->parent == nullptr)
	child->parent = this;
	children.insertAt (index, child);

	nodeChanged ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BlockContentNode::removeNode (IBlockContentNode* node)
{
	auto* child = unknown_cast<BlockContentNode> (node);
	if(!child)
		return kResultInvalidArgument;
	
	ASSERT (child->parent == this)
	if(child->parent != this)
		return kResultFailed;

	child->parent = nullptr;
	children.remove (child);

	nodeChanged ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockContentNode::removeNodes ()
{
	if(!children.isEmpty ())
	{
		for(auto* child : iterate_as<BlockContentNode> (children))
			child->parent = nullptr;

		children.removeAll ();
		
		nodeChanged ();
	}
}

//************************************************************************************************
// BlockContentRoot
//************************************************************************************************

DEFINE_CLASS (BlockContentRoot, BlockContentNode)
DEFINE_CLASS_UID (BlockContentRoot, 0xdc637d05, 0x6999, 0x4aa9, 0xa9, 0x3f, 0x34, 0x92, 0x20, 0x60, 0x90, 0x33)
DEFINE_STRINGID_MEMBER_ (BlockContentRoot, kMakeNodeVisible, "makeNodeVisible")

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockContentRoot::nodeChanged (BlockContentNode* node)
{
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentRoot* BlockContentRoot::getRoot () const
{
	return const_cast<BlockContentRoot*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentChildren& CCL_API BlockContentRoot::getChildren ()
{
	return SuperClass::getChildren ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentNode* CCL_API BlockContentRoot::getParentNode ()
{
	return SuperClass::getParentNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockContentRoot::setCustomStyleID (StringID id)
{
	SuperClass::setCustomStyleID (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API BlockContentRoot::getCustomStyleID () const
{
	return SuperClass::getCustomStyleID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentBuilder* CCL_API BlockContentRoot::createBuilder ()
{
	return NEW BlockContentBuilder (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockContentRoot::makeNodeVisible (IBlockContentNode* node)
{
	signal (Message (kMakeNodeVisible, node));
}

//************************************************************************************************
// TextContentNode
//************************************************************************************************

DEFINE_CLASS (TextContentNode, BlockContentNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextContentNode::setText (StringRef text)
{
	setFormattedText (AutoPtr<FormattedText> (NEW FormattedText (text)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TextContentNode::getText () const
{
	return formattedText ? formattedText->getText () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextContentNode::addFormatRange (int start, int length, TextNodeType type, VariantRef argument)
{
	ASSERT (formattedText)
	if(formattedText)
		formattedText->addFormatRange (start, length, type, argument);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& TextContentNode::getFormatRanges () const
{
	ASSERT (formattedText)
	if(!formattedText)
		ccl_const_cast (this)->formattedText = NEW FormattedText;

	return formattedText->getFormatRanges ();
}

//************************************************************************************************
// ImageContentNode
//************************************************************************************************

DEFINE_CLASS (ImageContentNode, BlockContentNode)

//************************************************************************************************
// ContainerContentNode
//************************************************************************************************

DEFINE_CLASS (ContainerContentNode, BlockContentNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerContentNode::ContainerContentNode (TextNodeType nodeType)
: nodeType (nodeType)
{}

//************************************************************************************************
// BlockContentBuilder
//************************************************************************************************

BlockContentBuilder::BlockContentBuilder (BlockContentRoot& root)
: root (root)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextContentNode* BlockContentBuilder::createPlainTextNode (StringRef string)
{
	TextContentNode* node = NEW TextContentNode;
	node->setText (string);
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextContentNode* BlockContentBuilder::createCCLMarkupNode (StringRef string)
{
	TextContentNode* node = NEW TextContentNode;

	MarkupParser parser (string);
	node->setText (parser.getPlainText ());
	for(auto* range : iterate_as<FormattedText::FormatRange> (parser.getFormatInstructions ()))
		node->addFormatRange (range->getStart (), range->getLength (), range->getType (), range->getArgument ());

	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentNode* BlockContentBuilder::createNodeFromMarkdown (IStream& stream)
{
	auto* parentNode = NEW BlockContentNode;

	AutoPtr<IMarkdownParser> markdownParser = System::CreateMarkdownParser ();
	MarkdownBlockContentBuilder builder (*this, *parentNode);
	markdownParser->setHandler (&builder);
	markdownParser->parse (stream);

	return parentNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageContentNode* BlockContentBuilder::createImageNode (IImage* image)
{
	ImageContentNode* node = NEW ImageContentNode;
	node->setContentImage (image);
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult BlockContentBuilder::loadAndInsertContent (VariantRef data, StringID contentType)
{
	IBlockContentNode* newNode = nullptr;
	tresult result = loadContent (newNode, data, contentType);
	if(newNode)
		root.getChildren ().insertNode (newNode);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BlockContentBuilder::loadContent (IBlockContentNode*& newNode, VariantRef data, StringID contentType)
{
	newNode = nullptr;

	ASSERT (!contentType.isEmpty ()) // expected to be set explicitly by caller
	const CString kImagePrefix ("image/");

	if(data.isString ())
	{
		String string (data.asString ());

		if(contentType == kPlainText)
		{
			newNode = createPlainTextNode (string);
		}
		else if(contentType == kCCLMarkup)
		{
			newNode = createCCLMarkupNode (string);
		}
		else if(contentType == kMarkdown)
		{
			AutoPtr<IStream> stream = System::GetFileUtilities ().createStringStream (string, Text::kUTF8, IFileUtilities::kSuppressByteOrderMark);
			if(!stream)
				return kResultOutOfMemory;

			newNode = createNodeFromMarkdown (*stream);
		}
	}
	else if(UnknownPtr<IStream> stream = data.asUnknown ())
	{
		if(contentType == kCCLMarkup || contentType == kPlainText)
		{
			String string = TextUtils::loadRawString (*stream);
			if(contentType == kCCLMarkup)
				newNode = createCCLMarkupNode (string);
			else
				newNode = createPlainTextNode (string);
		}		
		else if(contentType == kMarkdown)
		{
			newNode = createNodeFromMarkdown (*stream);
		}
		else if(contentType.startsWith (kImagePrefix))
		{
			if(const FileType* format = System::GetFileTypeRegistry ().getFileTypeByMimeType (String (contentType)))
			{
				AutoPtr<Image> image = Image::loadImage (*stream, *format);
				if(!image)
					return kResultFailed;

				newNode = createImageNode (image);
			}
		}
	}
	else if(UnknownPtr<IUrl> path = data.asUnknown ())
	{
		if(contentType.startsWith (kImagePrefix))
		{
			AutoPtr<Image> image = Image::loadImage (*path);
			if(!image)
				return kResultFailed;

			newNode = createImageNode (image);
		}
		else
		{
			AutoPtr<IMemoryStream> stream = File::loadBinaryFile (*path);
			if(!stream)
				return kResultFailed;

			if(contentType == kCCLMarkup || contentType == kPlainText)
			{
				String string = TextUtils::loadRawString (*stream);
				if(contentType == kCCLMarkup)
					newNode = createCCLMarkupNode (string);
				else
					newNode = createPlainTextNode (string);
			}
			else if(contentType == kMarkdown)
			{
				newNode = createNodeFromMarkdown (*stream);
			}
		}
	}
	else if(UnknownPtr<IImage> image = data.asUnknown ())
	{
		newNode = createImageNode (image);
	}

	return newNode ? kResultOk : kResultInvalidArgument;
}
