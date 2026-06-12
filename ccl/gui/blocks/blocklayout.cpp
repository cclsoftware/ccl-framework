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
// Filename    : ccl/gui/blocks/blocklayout.cpp
// Description : Block Layout
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_DRAW_BLOCKNODE_LAYOUT (0 && DEBUG)

#include "ccl/gui/blocks/blocklayout.h"

#include "ccl/gui/graphics/textlayoutbuilder.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/system/animation.h"
#include "ccl/gui/theme/theme.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug Drawing
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_DRAW_BLOCKNODE_LAYOUT
static bool suspendDebugDraw = false;

static Color getDebugColor (BlockLayoutNode* node)
{
	if(ccl_cast<TextLayoutNode> (node))
		return Colors::kGreen;
	else if(ccl_cast<ContainerLayoutNode> (node))
		return Colors::kYellow;
	else
		return Colors::kRed;
}

static CoordF getDebugInset (BlockLayoutNode* node)
{
	if(ccl_cast<TextLayoutNode> (node))
		return 2;
	else if(ccl_cast<ContainerLayoutNode> (node))
		return 1;
	else
		return 0;
}
#endif

//************************************************************************************************
// BlockLayoutNode
//************************************************************************************************

const FlexContainerData& BlockLayoutNode::getDefaultFlexData ()
{
	static FlexContainerData defaultData;
	static bool initialized = false;
	if(!initialized)
	{
		defaultData.direction = FlexDirection::kColumn; // default to vertical layout
		initialized = true;
	}
	return defaultData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (BlockLayoutNode, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode::BlockLayoutNode ()
: flexNode (FlexNodeFactory::createNode ()),
  parent (nullptr)
{
	flexData = getDefaultFlexData ();
	flexNode->applyContainerData (flexData, flexItemData);

	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode::BlockLayoutNode (const BlockLayoutNode& other)
: flexNode (FlexNodeFactory::createNode ()),
  parent (nullptr),
  flexData (other.flexData),
  flexItemData (other.flexItemData)
{
	flexNode->applyContainerData (flexData, flexItemData);

	children.objectCleanup (true);	
	children.add (other.children, Container::kClone);
	
	int index = 0;
	for(auto* child : iterate_as<BlockLayoutNode> (children))
	{
		child->parent = this;
		flexNode->insertNode (index++, child->getFlexNode ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode::~BlockLayoutNode ()
{
	delete flexNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::applyStyle (const IVisualStyleData& data)
{
	const FlexContainerData& flexDefault = getDefaultFlexData ();
	FlexShared::setAttributes (flexData, data, &flexDefault);
	FlexShared::setAttributes (flexItemData, data);
	updateFlexNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::resizeToContent ()
{
	RectF contentSize;
	for(auto* child : iterate_as<BlockLayoutNode> (children))
	{
		RectF childSize;
		child->getBounds (childSize);
		contentSize.join (childSize);
	}
	updateFlexSize (contentSize.getWidth (), contentSize.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::updateFlexSize (CoordF width, CoordF height)
{
	if(flexItemData.alignSelf != FlexAlignSelf::kStretch)
		flexItemData.width = {DesignCoord::kCoord, width};
	flexItemData.height = {DesignCoord::kCoord, height};
	flexNode->applyItemData (flexItemData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::onLayoutChanged ()
{
	flexNode->setHasNewLayout (false);

	for(auto* child : iterate_as<BlockLayoutNode> (children))
		child->onLayoutChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::getBounds (RectF& bounds) const
{
	flexNode->getLayoutSize (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointFRef BlockLayoutNode::localToRoot (PointF& position) const
{
	position += flexNode->getLayoutPosition ();
	if(parent)
		parent->localToRoot (position);

	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointFRef BlockLayoutNode::rootToLocal (PointF& position) const
{
	PointF offset;
	localToRoot (offset);

	position -= offset;
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::drawNode (IGraphics& graphics, RectFRef updateRect) const
{
	for(auto* child : iterate_as<BlockLayoutNode> (children))
	{
		RectF childSize;
		child->getBounds (childSize);
		if(childSize.intersect (updateRect))
		{
			#if DEBUG_DRAW_BLOCKNODE_LAYOUT
			if(!suspendDebugDraw)
				graphics.drawRect (RectF (childSize).contract (getDebugInset (child)), Pen (getDebugColor (child)));
			#endif

			// translate for drawing in child coordinates
			Transform transform;
			transform.translate (childSize.left, childSize.top);
			graphics.saveState ();
			graphics.addTransform (transform);

			RectF childUpdateRect (updateRect);
			childUpdateRect.offset (-childSize.left, -childSize.top);
			child->drawNode (graphics, childUpdateRect);

			graphics.restoreState ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FlexNode* BlockLayoutNode::getFlexNode ()
{
	return flexNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockLayoutNode::getParent () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutRoot* BlockLayoutNode::getRoot () const
{
	if(parent)
		return parent->getRoot ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& BlockLayoutNode::getChildArray () const
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockLayoutNode::findNode (PointFRef where, int flags) const
{
	auto findDeepChild = [&] (BlockLayoutNode* child, RectFRef childSize) -> BlockLayoutNode*
	{
		if(flags & kFindNodeDeep)
		{
			PointF where2 (where);
			where2.offset (- childSize.left, - childSize.top);
			return child->findNode (where2, flags);
		}
		return nullptr;
	};

	BlockLayoutNode* childBefore = nullptr;

	for(auto* child : iterate_as<BlockLayoutNode> (children))
	{
		RectF childSize;
		child->getBounds (childSize);
		if(childSize.pointInside (where))
		{
			if(BlockLayoutNode* deepChild = findDeepChild (child, childSize))
				return deepChild;

			return child;
		}
		else if(flags & kFindNodeAcceptPrevious)
			if(where.y > childSize.top)
				childBefore = child;
	}

	if(childBefore)
	{
		ASSERT (childBefore->getParent () == this)

		RectF childSize;
		childBefore->getBounds (childSize);
		if(BlockLayoutNode* deepChild = findDeepChild (childBefore, childSize))
			return deepChild;
	}
	return childBefore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockLayoutNode::findLayoutNodeForContent (const BlockContentNode* contentNode) const
{
	BlockLayoutNode* layoutNode = nullptr;
	
	auto visitNode = [&] (BlockLayoutNode& node)
	{
		if(node.getContentNode () == contentNode)
		{
			layoutNode = &node;
			return false;
		}
		return true;
	};

	visitChildren (visitNode, true);
	return layoutNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::updateFlexNode ()
{
	flexNode->applyContainerData (flexData, flexItemData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::insertNode (BlockLayoutNode* child, int index)
{
	if(index < 0 || !children.isValidIndex (index))
		index = children.count ();
	
	flexNode->insertNode (index, child->getFlexNode ());

	ASSERT (child->parent == nullptr)
	child->parent = this;

	children.insertAt (index, child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockLayoutNode::removeNode (BlockLayoutNode* child)
{
	ASSERT (child && child->parent == this)
	if(!child || child->parent != this)
		return false;

	child->parent = nullptr;
	children.remove (child);

	flexNode->removeNode (child->getFlexNode ());
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::removeNodes ()
{
	if(!children.isEmpty ())
	{
		for(auto* child : iterate_as<BlockLayoutNode> (children))
		{
			child->parent = nullptr;
			flexNode->removeNode (child->getFlexNode ());
		}

		children.removeAll ();	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::nodeAttached ()
{
	for(auto* child : iterate_as<BlockLayoutNode> (children))
		child->nodeAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutNode::nodeDetached ()
{
	for(auto* child : iterate_as<BlockLayoutNode> (children))
		child->nodeDetached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockLayoutNode::isNodeAttached () const
{
	if(BlockLayoutRoot* root = getRoot ())
		return root->isNodeAttached ();

	return false;
}

//************************************************************************************************
// BlockLayoutRoot
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BlockLayoutRoot, BlockLayoutNode)
DEFINE_STRINGID_MEMBER_ (BlockLayoutRoot, kInvalidateNode, "invalidateNode")

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutRoot::BlockLayoutRoot ()
: layoutWidth (CoordF(kMaxCoord)),
  rootAttached (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::setLayoutWidth (CoordF newWidth)
{
	if(newWidth == layoutWidth)
		return;

	layoutWidth = newWidth;
	updateLayout ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

CoordF BlockLayoutRoot::getLayoutWidth () const
{
	return layoutWidth;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::updateLayout ()
{
	CCL_PRINTF ("[BlockLayoutRoot::updateLayout] Layout width %.2f\n", layoutWidth)

	flexNode->calculateLayout (&layoutWidth); // calculate to update item widths
	onLayoutChanged ();
	flexNode->calculateLayout (&layoutWidth); // calculate layout again (with updated item heights)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::invalidateNode (BlockLayoutNode* node)
{
	signal (Message (kInvalidateNode, node->asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::setStyle (VisualStyle* newStyle)
{
	style = newStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const VisualStyle& BlockLayoutRoot::getStyle () const
{
	if(style)
		return *style;
	
	if(VisualStyle* standardStyle = ThemePainter::getStandardStyle (ThemePainter::kBlockViewStyle))
		return *standardStyle;

	return VisualStyle::emptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutRoot* BlockLayoutRoot::getRoot () const
{
	return const_cast<BlockLayoutRoot*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointFRef BlockLayoutRoot::localToRoot (PointF& position) const
{
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::nodeAttached ()
{
	if(rootAttached == false)
	{
		rootAttached = true;
		SuperClass::nodeAttached ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutRoot::nodeDetached ()
{
	if(rootAttached == true)
	{
		rootAttached = false;
		SuperClass::nodeDetached ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockLayoutRoot::isNodeAttached () const
{
	return rootAttached;
}

//************************************************************************************************
// DecoratedLayoutNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DecoratedLayoutNode, BlockLayoutNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DecoratedLayoutNode::DecoratedLayoutNode ()
: background (AtomName::kBackground, RectF ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DecoratedLayoutNode::applyStyle (const IVisualStyleData& data)
{
	SuperClass::applyStyle (data);

	background.setImage (StyleDataReader (data).getBackgroundImage ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DecoratedLayoutNode::drawNode (IGraphics& graphics, RectFRef updateRect) const
{
	SuperClass::drawNode (graphics, updateRect);

	RectF size (0, 0, flexNode->getLayoutWidth (), flexNode->getLayoutHeight ());
	background.setSize (size);
	background.draw (graphics, UpdateRgn (rectFToInt (updateRect)));
}

//************************************************************************************************
// TextLayoutNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TextLayoutNode, DecoratedLayoutNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextLayoutNode::TextLayoutNode ()
: currentWidth (0.f),
  currentHeight (0.f),
  textMargin (0.f),
  textMaxWidth (0.f),
  textBrush (SolidBrush ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FormattedText* TextLayoutNode::getFormattedText () const
{
	auto* textContentNode = ccl_cast<TextContentNode> (getContentNode ());
	return textContentNode ? textContentNode->getFormattedText () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutNode::resizeToContent ()
{
	if(textLayout)
	{
		RectF bounds;
		textLayout->getBounds (bounds);
		currentWidth = ccl_round<0, float> (bounds.getWidth () + (2 * textMargin));
		currentHeight = ccl_round<0, float> (bounds.getHeight () + (2 * textMargin));
	}
	else
	{
		currentWidth = currentHeight = 0.f;
	}

	CCL_PRINTF ("[TextLayoutNode::adjustSize] Applying flex node size %.2f x %.2f\n",
				currentWidth, currentHeight)

	updateFlexSize (currentWidth, currentHeight);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutNode::onLayoutChanged ()
{
    if(textLayout && flexNode->hasNewLayout ())
    {
		// Resize the textLayout to fill all available space
		CoordF rootLayoutWidth (kMaxCoord);
		if(BlockLayoutRoot* root = getRoot ())
			rootLayoutWidth = root->getLayoutWidth () - flexItemData.inset.left.value - flexItemData.inset.right.value;
		CoordF width = ccl_min (((textMaxWidth > 0) ? textMaxWidth : flexNode->getLayoutWidth ()), rootLayoutWidth);
		CoordF height = flexNode->getLayoutHeight ();

		if((width > 0 && width != currentWidth) || height != currentHeight)
        {
			currentWidth = width;
			currentHeight = height;

			CCL_PRINTF ("[TextLayoutNode::onLayoutChanged] Resizing text layout width to %.2f\n", currentWidth)

			textLayout->resize (currentWidth - (2 * textMargin), CoordF(kMaxCoord));

			resizeToContent ();
        }
    }
    SuperClass::onLayoutChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutNode::applyStyle (const IVisualStyleData& data)
{
	SuperClass::applyStyle (data);

	textMargin = StyleDataReader (data).getMetric<float> ("text.margin", textMargin);
	textMaxWidth = StyleDataReader (data).getMetric<float> ("text.maxwidth", textMaxWidth);
	textBrush = StyleDataReader (data).getTextBrush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutNode::drawNode (IGraphics& graphics, RectFRef updateRect) const
{
	if(textLayout)
	{
		SuperClass::drawNode (graphics, updateRect);

		PointF textPosition (textMargin, textMargin);
		graphics.drawTextLayout (textPosition, textLayout, textBrush);
	}
}

//************************************************************************************************
// ImageLayoutNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ImageLayoutNode, DecoratedLayoutNode)
DEFINE_STRINGID_MEMBER_ (ImageLayoutNode, kImageFrame, "imageFrame")

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageLayoutNode::ImageLayoutNode ()
: currentWidth (0.f),
  currentHeight (0.f),
  imageMargin (0.f),
  currentImageFrame (0),
  animationStarted (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageLayoutNode::getImage () const
{
	auto* contentNode = ccl_cast<ImageContentNode> (getContentNode ());
	return contentNode ? contentNode->getContentImage () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::resizeToContent ()
{
	if(IImage* contentImage = getImage ())
	{
		currentWidth = ccl_round<0, float> (contentImage->getWidth () + (2 * imageMargin));
		currentHeight = ccl_round<0, float> (contentImage->getHeight () + (2 * imageMargin));
	}
	else
	{
		currentWidth = currentHeight = 0.f;
	}

	CCL_PRINTF ("[ImageLayoutNode::resizeToContent] Applying flex node size %.2f x %.2f\n",
				currentWidth, currentHeight)

	updateFlexSize (currentWidth, currentHeight);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::applyStyle (const IVisualStyleData& data)
{
	SuperClass::applyStyle (data);

	imageMargin = StyleDataReader (data).getMetric<float> ("image.margin", imageMargin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::nodeAttached ()
{
	startAnimation ();
	SuperClass::nodeAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::nodeDetached ()
{
	stopAnimation ();
	SuperClass::nodeDetached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::startAnimation ()
{
	ASSERT (animationStarted == false)

	if(Filmstrip* filmstrip = unknown_cast<Filmstrip> (getImage ()))
		if(filmstrip->getDuration () > 0.)
		{
			CCL_PRINTLN ("[ImageLayoutNode::startAnimation]")

			BasicAnimation a;
			a.setStartValue (0);
			a.setEndValue (filmstrip->getFrameCount ());
			a.setDuration (filmstrip->getDuration ());
			a.setRepeatCount (Animation::kRepeatForever);
			
			AnimationManager::instance ().addAnimation (this, kImageFrame, a.asInterface ());
			animationStarted = true;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::stopAnimation ()
{
	if(animationStarted)
	{
		CCL_PRINTLN ("[ImageLayoutNode::stopAnimation]")

		AnimationManager::instance ().removeAnimation (this, kImageFrame);
		animationStarted = false;
		currentImageFrame = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageLayoutNode::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kImageFrame)
	{
		int frame = var.asInt ();
		if(currentImageFrame != frame)
		{
			currentImageFrame = frame;

			if(BlockLayoutRoot* root = getRoot ())
				root->invalidateNode (this);
		}
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageLayoutNode::drawNode (IGraphics& graphics, RectFRef updateRect) const
{
	SuperClass::drawNode (graphics, updateRect);

	if(IImage* contentImage = getImage ())
	{
		contentImage->setCurrentFrame (currentImageFrame); // in case it is animated

		Rect srcRect (0, 0, contentImage->getWidth (), contentImage->getHeight ());
		Rect dstRect (srcRect);
		Rect foregroundSize (0, 0, flexNode->getLayoutWidth (), flexNode->getLayoutHeight ());
		dstRect.center (foregroundSize);

		ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);
		graphics.drawImage (contentImage, srcRect, dstRect, &mode);
	}
}

//************************************************************************************************
// ContainerLayoutNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ContainerLayoutNode, BlockLayoutNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerLayoutNode::ContainerLayoutNode ()
: indent (FrameworkTheme::instance ().getThemeMetric (ThemeElements::kLayoutSpacing)),
  spacing (FrameworkTheme::instance ().getThemeMetric (ThemeElements::kLayoutSpacing)),
  textMargin (0.f),
  markerWidth (0.f),
  textBrush (SolidBrush ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerLayoutNode::setMarkerTextLayout (ITextLayout* textLayout)
{
	markerTextLayout.share (textLayout);

	if(markerTextLayout)
	{
		RectF bounds;
		markerTextLayout->getBounds (bounds);
		markerWidth = bounds.getWidth ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerLayoutNode::applyStyle (const IVisualStyleData& data)
{
	indent = StyleDataReader (data).getMetric<float> ("listitem.indent", indent);
	spacing = StyleDataReader (data).getMetric<float> ("listitem.spacing", spacing);
	textMargin = StyleDataReader (data).getMetric<float> ("text.margin", textMargin);
	textBrush = StyleDataReader (data).getTextBrush ();

	// ignore flex attributes from visual style (see SuperClass) for this internal node
	// note: child TextLayoutNode can have an additonal textMargin

	// layout: | indent | marker | spacing | child |
	flexData.padding.left = {DesignCoord::kCoord, indent + markerWidth + spacing};
	updateFlexNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerLayoutNode::drawNode (IGraphics& graphics, RectFRef updateRect) const
{
	// draw list marker string
	if(markerTextLayout)
	{
		PointF textPosition (indent, textMargin);
		graphics.drawTextLayout (textPosition, markerTextLayout, textBrush);
	}

	// draw child nodes
	SuperClass::drawNode (graphics, updateRect);
}

//************************************************************************************************
// BlockLayoutBuilder
//************************************************************************************************

BlockLayoutBuilder::BlockLayoutBuilder (BlockLayoutRoot& layout)
: layout (layout)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutBuilder::buildLayout (BlockContentRoot& content)
{
	initNode (layout, content);
	buildChildren (layout, content);
	layout.updateLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutBuilder::buildChildren (BlockLayoutNode& layoutParentNode, BlockContentNode& contentParentNode)
{
	for(auto* contentChildNode : iterate_as<BlockContentNode> (contentParentNode.getChildArray ()))
	{
		BlockLayoutNode* newChildLayoutNode = nullptr;

		if(auto* textContentNode = ccl_cast<TextContentNode> (contentChildNode))
		{
			if(FormattedText* formattedText = textContentNode->getFormattedText ())
			{
				AutoPtr<ITextLayout> textLayout = createTextLayout (*formattedText);

				auto* textLayoutNode = NEW TextLayoutNode;
				initNode (*textLayoutNode, *textContentNode);

				textLayoutNode->setTextLayout (textLayout);
				textLayoutNode->resizeToContent ();

				newChildLayoutNode = textLayoutNode;
			}
		}
		else if(auto* imageContentNode = ccl_cast<ImageContentNode> (contentChildNode))
		{
			auto* imageLayoutNode = NEW ImageLayoutNode ();
			initNode (*imageLayoutNode, *imageContentNode);
			imageLayoutNode->resizeToContent ();
			newChildLayoutNode = imageLayoutNode;
		}
		else if(auto* containerContentNode = ccl_cast<ContainerContentNode> (contentChildNode))
		{
			AutoPtr<ITextLayout> textLayout (createTextLayout (containerContentNode->getMarker ()));

			auto* containerLayoutNode = NEW ContainerLayoutNode;
			containerLayoutNode->setMarkerTextLayout (textLayout);
			initNode (*containerLayoutNode, *containerContentNode);

			newChildLayoutNode = containerLayoutNode;
		}
		else
		{
			newChildLayoutNode = NEW BlockLayoutNode;
			initNode (*newChildLayoutNode, *contentChildNode);
		}

		if(newChildLayoutNode)
		{
			layoutParentNode.insertNode (newChildLayoutNode);
			buildChildren (*newChildLayoutNode, *contentChildNode);
		}
		else
			buildChildren (layoutParentNode, *contentChildNode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockLayoutBuilder::initNode (BlockLayoutNode& layoutNode, BlockContentNode& contentNode) const
{
	layoutNode.setContentNode (&contentNode);

	const IVisualStyleData* data = nullptr;

	CString styleId = contentNode.getEffectiveCustomStyleID ();
	if(!styleId.isEmpty ())
		data = layout.getStyle ().getStyleCondition (styleId);

	if(data == nullptr)
		data = &layout.getStyle ().asData ();

	#if DEBUG_LOG
	CCL_PRINTF ("[BlockLayoutBuilder] Apply style '%s' to node %s\n", 
				 data->getName ().str (),
				 layoutNode.myClass ().getPersistentName ())
	#endif

	layoutNode.applyStyle (*data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* BlockLayoutBuilder::createTextLayout (StringRef string) const
{
	const VisualStyle& vs = layout.getStyle ();
	CoordF layoutWidth = layout.getLayoutWidth ();
	
	ITextLayout* textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
	TextFormat textFormat (Alignment::kLeftTop, TextFormat::kWordBreak);
	textLayout->construct (string, layoutWidth, CoordF(kMaxCoord), vs.getTextFont (), ITextLayout::kMultiLine, textFormat);
	return textLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* BlockLayoutBuilder::createTextLayout (const FormattedText& formattedText) const
{
	ITextLayout* textLayout = createTextLayout (formattedText.getText ());

	TextLayoutBuilder builder (*textLayout, &layout.getStyle ());
	formattedText.applyTo (builder);
	return textLayout;
}
