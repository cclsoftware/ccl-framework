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
// Filename    : ccl/gui/layout/yogashared.h
// Description : Flexbox layout implementation with Facebook's Yoga library
//
//************************************************************************************************

#ifndef _ccl_yogashared_h
#define _ccl_yogashared_h

#include "ccl/gui/layout/flexboxshared.h"

#include <yoga/Yoga.h>

namespace CCL {

//************************************************************************************************
// YogaNode
//************************************************************************************************

class YogaNode: public FlexNode
{
public:
	YogaNode ();
	~YogaNode ();

	operator YGNodeRef () const;

	// FlexNode
	void applyContainerData (const FlexContainerData& flexData, const FlexItemData& flexItemData) override;
	void applyItemData (const FlexItemData& flexItemData) override;
	void applySizeLimits (const FlexItemData& flexItemData) override;
	void applySize (CoordF width, CoordF height) override;
	void calculateLayout (const CoordF* availableWidth = nullptr, const CoordF* availableHeight = nullptr) override;
	bool hasNewLayout () const override;
	void setHasNewLayout (bool state) override;
	PointF getLayoutPosition () const override;
	CoordF getLayoutWidth () const override;
	CoordF getLayoutHeight () const override;
	bool insertNode (int index, FlexNode* child) override;
	bool removeNode (FlexNode* child) override;

protected:
	YGNodeRef node;

	void applyNodeWidth (const FlexItemData& flexItemData);
	void applyNodeHeight (const FlexItemData& flexItemData);
};

} // namespace CCL

#endif // _ccl_yogashared_h