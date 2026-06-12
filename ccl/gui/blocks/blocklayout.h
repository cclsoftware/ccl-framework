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
// Filename    : ccl/gui/blocks/blocklayout.h
// Description : Block Layout
//
//************************************************************************************************

#ifndef _ccl_blocklayout_h
#define _ccl_blocklayout_h

#include "ccl/gui/blocks/blockcontent.h"

#include "ccl/gui/layout/flexboxshared.h"
#include "ccl/gui/theme/renderatom.h"

namespace CCL {

class BlockLayoutRoot;

//************************************************************************************************
// BlockLayoutNode
//************************************************************************************************

class BlockLayoutNode: public Object
{
public:
	DECLARE_CLASS (BlockLayoutNode, Object)

	BlockLayoutNode ();
	~BlockLayoutNode ();

	FlexNode* getFlexNode ();
	BlockLayoutNode* getParent () const;
	virtual BlockLayoutRoot* getRoot () const;
	const ObjectArray& getChildArray () const;

	PROPERTY_SHARED_AUTO (BlockContentNode, contentNode, ContentNode)

	virtual void applyStyle (const IVisualStyleData& data);
	virtual void resizeToContent ();
	virtual void onLayoutChanged ();

	virtual void getBounds (RectF& bounds) const;

	virtual PointFRef localToRoot (PointF& position) const;
	virtual PointFRef rootToLocal (PointF& position) const;

	enum FindNodeFlags { kFindNodeDeep = 1<<0, kFindNodeAcceptPrevious = 1<<1 };
	BlockLayoutNode* findNode (PointFRef where, int flags) const;
	BlockLayoutNode* findLayoutNodeForContent (const BlockContentNode* contentNode) const;

	virtual void drawNode (IGraphics& graphics, RectFRef updateRect) const;

	template<class Lambda>
	bool visitChildren (const Lambda& visitNode, bool recursive) const;

	void insertNode (BlockLayoutNode* node, int index = -1);
	bool removeNode (BlockLayoutNode* node);
	void removeNodes ();

	virtual void nodeAttached ();
	virtual void nodeDetached ();
	virtual bool isNodeAttached () const;

protected:
	static const FlexContainerData& getDefaultFlexData ();

	FlexContainerData flexData;
	FlexItemData flexItemData;
	FlexNode* flexNode;
	BlockLayoutNode* parent;
	ObjectArray children;

	BlockLayoutNode (const BlockLayoutNode&);

	void updateFlexNode ();
	void updateFlexSize (CoordF width, CoordF height);
};

//************************************************************************************************
// BlockLayoutRoot
//************************************************************************************************

class BlockLayoutRoot: public BlockLayoutNode
{
public:
	DECLARE_CLASS (BlockLayoutRoot, BlockLayoutNode)

	BlockLayoutRoot ();

	void setLayoutWidth (CoordF newWidth);
	CoordF getLayoutWidth () const;
	void updateLayout ();

	void setStyle (VisualStyle* newStyle);
	const VisualStyle& getStyle () const;

	void invalidateNode (BlockLayoutNode* node);

	DECLARE_STRINGID_MEMBER (kInvalidateNode)
	
	// BlockLayoutNode
	BlockLayoutRoot* getRoot () const override;
	PointFRef localToRoot (PointF& position) const override;
	void nodeAttached () override;
	void nodeDetached () override;
	bool isNodeAttached () const override;

protected:
	CoordF layoutWidth;
	SharedPtr<VisualStyle> style;
	bool rootAttached;
};

//************************************************************************************************
// DecoratedLayoutNode
//************************************************************************************************

class DecoratedLayoutNode: public BlockLayoutNode
{
public:
	DECLARE_CLASS (DecoratedLayoutNode, BlockLayoutNode)

	DecoratedLayoutNode ();

	// BlockLayoutNode
	void applyStyle (const IVisualStyleData& data) override;
	void drawNode (IGraphics& graphics, RectFRef updateRect) const override;

private:
	mutable RenderAtom background;
};

//************************************************************************************************
// TextLayoutNode
//************************************************************************************************

class TextLayoutNode: public DecoratedLayoutNode
{
public:
	DECLARE_CLASS (TextLayoutNode, DecoratedLayoutNode)

	TextLayoutNode ();

	PROPERTY_SHARED_AUTO (ITextLayout, textLayout, TextLayout)

	const FormattedText* getFormattedText () const;

	// DecoratedLayoutNode
	void resizeToContent () override;
	void onLayoutChanged () override;
	void applyStyle (const IVisualStyleData& data) override;
	void drawNode (IGraphics& graphics, RectFRef updateRect) const override;

private:
	CoordF currentWidth;
	CoordF currentHeight;
	CoordF textMargin;
	CoordF textMaxWidth;
	Brush textBrush;
};

//************************************************************************************************
// ImageLayoutNode
//************************************************************************************************

class ImageLayoutNode: public DecoratedLayoutNode
{
public:
	DECLARE_CLASS (ImageLayoutNode, DecoratedLayoutNode)

	ImageLayoutNode ();

	IImage* getImage () const;

	// DecoratedLayoutNode
	void resizeToContent () override;
	void applyStyle (const IVisualStyleData& data) override;
	void nodeAttached () override;
	void nodeDetached () override;
	void drawNode (IGraphics& graphics, RectFRef updateRect) const override;

	DECLARE_STRINGID_MEMBER (kImageFrame)

private:
	CoordF currentWidth;
	CoordF currentHeight;
	CoordF imageMargin;
	int currentImageFrame;
	bool animationStarted;

	void startAnimation ();
	void stopAnimation ();

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ContainerLayoutNode
//************************************************************************************************

class ContainerLayoutNode: public BlockLayoutNode
{
public:
	DECLARE_CLASS (ContainerLayoutNode, BlockLayoutNode)

	ContainerLayoutNode ();

	void setMarkerTextLayout (ITextLayout* textLayout);

	// BlockLayoutNode
	void applyStyle (const IVisualStyleData& data) override;
	void drawNode (IGraphics& graphics, RectFRef updateRect) const override;

private:
	CoordF indent;		///< left indent (of marker) from parent block
	CoordF spacing;		///< between marker and child content
	CoordF textMargin;	///< top margin (for drawing marker)
	CoordF markerWidth;
	Brush textBrush;
	AutoPtr<ITextLayout> markerTextLayout;
};

//************************************************************************************************
// BlockLayoutBuilder
//************************************************************************************************

class BlockLayoutBuilder
{
public:
	BlockLayoutBuilder (BlockLayoutRoot& layout);

	void buildLayout (BlockContentRoot& content);

protected:
	BlockLayoutRoot& layout;

	void buildChildren (BlockLayoutNode& layoutParentNode, BlockContentNode& contentParentNode);
	void initNode (BlockLayoutNode& layoutNode, BlockContentNode& contentNode) const;
	ITextLayout* createTextLayout (StringRef string) const;
	ITextLayout* createTextLayout (const FormattedText& formattedText) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// BlockLayoutNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Lambda>
bool BlockLayoutNode::visitChildren (const Lambda& visitNode, bool recursive) const
{
	for(auto* child : iterate_as<BlockLayoutNode> (children))
	{
		if(!visitNode (*child))
			return false;

		if(recursive)
			if(!child->visitChildren (visitNode, recursive))
				return false;
	}
	return true;
}

} // namespace CCL

#endif // _ccl_blocklayout_h
