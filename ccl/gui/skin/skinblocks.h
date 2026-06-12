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
// Filename    : ccl/gui/skin/skinblocks.h
// Description : Block Skin Elements
//
//************************************************************************************************

#ifndef _ccl_skinblocks_h
#define _ccl_skinblocks_h

#include "ccl/gui/skin/skinmodel.h"

namespace CCL {
namespace SkinElements {

//************************************************************************************************
// BlockContentElement
//************************************************************************************************

class BlockContentElement: public ResourceObjectElement
{
public:
	DECLARE_SKIN_ELEMENT (BlockContentElement, ResourceObjectElement)

	PROPERTY_MUTABLE_CSTRING (contentType, ContentType)
	PROPERTY_STRING (markupData, MarkupData)

	// ResourceObjectElement
	bool setAttributes (const SkinAttributes& a) override;
	void appendCharacterData (const uchar* data, int length) override;
	bool loadObject (SkinModel& model) override;
};

//************************************************************************************************
// BlockViewElement
//************************************************************************************************

class BlockViewElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (BlockViewElement, ViewElement)

	PROPERTY_MUTABLE_CSTRING (verticalScrollName, VerticalScrollName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinblocks_h
