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
// Filename    : ccl/gui/skin/skinblocks.cpp
// Description : Block Skin Elements
//
//************************************************************************************************

#include "ccl/gui/skin/skinblocks.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skincontrols.h"

#include "ccl/gui/blocks/blockview.h"

#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/icontroller.h"

#include "ccl/base/storage/url.h"

namespace CCL {
namespace SkinElements {

//************************************************************************************************
// BlockContentResource
//************************************************************************************************

class BlockContentResource: public Object,
							public IBlockContentResource
{
public:
	DECLARE_CLASS_ABSTRACT (BlockContentResource, Object)

	BlockContentResource (const BlockContentElement& contentElement);

	// IBlockContentResource
	IBlockContentRoot* CCL_API createContent () override;

	CLASS_INTERFACE (IBlockContentResource, Object)

protected:
	const BlockContentElement& contentElement;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinBlocks () {} // force linkage of this file

//************************************************************************************************
// BlockContentResource
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (BlockContentResource, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockContentResource::BlockContentResource (const BlockContentElement& contentElement)
: contentElement (contentElement)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBlockContentRoot* CCL_API BlockContentResource::createContent ()
{
	MutableCString contentType (contentElement.getContentType ());

	auto* content = NEW BlockContentRoot;
	BlockContentBuilder builder (*content);

	if(!contentElement.getUrl ().isEmpty ())
	{
		Url contentUrl;
		contentElement.makeSkinUrl (contentUrl, contentElement.getUrl ());
		
		if(contentType.isEmpty ())
		{
			if(contentUrl.getFileType () == FileTypes::Markdown ())
				contentType = kMarkdown;
			else
				contentType = kCCLMarkup;
		}

		builder.loadAndInsertContent (static_cast<IUrl*> (&contentUrl), contentType);
	}
	else
	{
		if(contentType.isEmpty ())
			contentType = kCCLMarkup;

		builder.loadAndInsertContent (contentElement.getMarkupData (), contentType);
	}

	return content;
}

//************************************************************************************************
// BlockContentElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (BlockContentElement, ResourceObjectElement, TAG_BLOCKCONTENT, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TYPE, TYPE_STRING) ///< markup type, defaults to CCL Markup when empty
END_SKIN_ELEMENT_WITH_MEMBERS (BlockContentElement)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (BlockContentElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
END_SKIN_ELEMENT_ATTRIBUTES (BlockContentElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockContentElement::setAttributes (const SkinAttributes& a)
{
	contentType = a.getCString (ATTR_TYPE);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockContentElement::appendCharacterData (const uchar* data, int length)
{
	markupData.append (data, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockContentElement::loadObject (SkinModel& model)
{
	object = NEW BlockContentResource (*this);
	return true;
}

//************************************************************************************************
// BlockViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (BlockViewElement, ViewElement, TAG_BLOCKVIEW, DOC_GROUP_CONTROLS, BlockView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VSCROLLNAME, TYPE_STRING)	   ///< name of the vertical scroll parameter
END_SKIN_ELEMENT_WITH_MEMBERS (BlockViewElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockViewElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);
	verticalScrollName = a.getString (ATTR_VSCROLLNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockViewElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options);
	a.setString (ATTR_VSCROLLNAME, verticalScrollName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* BlockViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		view = NEW BlockView (size, options);

		SharedPtr<BlockContentRoot> content;
		if(UnknownPtr<IController> controller = args.controller)
			content = unknown_cast<BlockContentRoot> (controller->getObject (name, ccl_typeid<BlockContentRoot> ().getClassID ()));
		
		if(!content)
		{
			if(auto* resource = ccl_cast<BlockContentResource> (args.wizard.getRoot ().getResource (name)))
			{
				AutoPtr<IBlockContentRoot> newContent = resource->createContent ();
				content = unknown_cast<BlockContentRoot> (newContent);
			}
		}

		if(!content)
			SKIN_WARNING (this, "Block content not found: '%s'", name.str ())

		static_cast<BlockView*> (view)->setContent (content);
	}

	// scroll parameters
	if(!verticalScrollName.isEmpty ())
	{
		IParameter* scrollParam = ControlElement::getParameter (args, verticalScrollName, this);
		if(scrollParam)
			static_cast<BlockView*> (view)->setVScrollParam (scrollParam);
	}

	return SuperClass::createView (args, view);
}

} // namespace SkinElements
} // namespace CCL
