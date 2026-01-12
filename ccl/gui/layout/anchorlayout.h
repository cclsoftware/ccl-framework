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
// Filename    : ccl/gui/layout/anchorlayout.h
// Description : Base classes for anchor layouts
//
//************************************************************************************************

#ifndef _ccl_anchorlayout_h
#define _ccl_anchorlayout_h

#include "ccl/gui/layout/layoutview.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

namespace CCL {

class Divider;
class AnchorLayoutView;
class AnchorLayoutItem;
class AnchorLayoutAlgorithm;
struct AnchorLayoutData;

//************************************************************************************************
// AnchorLayoutContext
//************************************************************************************************

class AnchorLayoutContext: public LayoutContext
{
public:
	DECLARE_CLASS (AnchorLayoutContext, LayoutContext)

	using LayoutContext::LayoutContext;

	float getZoomFactor () const;
	bool isSizeModeDisabled () const;
	StyleRef getStyle () const;
	StringRef getTitle () const;
	void requestAlgorithm (LayoutAlgorithm* newAlgorithm);
};

//************************************************************************************************
// AnchorLayoutData
//************************************************************************************************

struct AnchorLayoutData
{
	int margin = kMinCoord;
	int spacing = kMinCoord;
};

//************************************************************************************************
// AnchorLayout
//************************************************************************************************

class AnchorLayout: public Layout
{
public:
	DECLARE_CLASS_ABSTRACT (AnchorLayout, Layout)

	AnchorLayoutData& getLayoutData ();
	virtual const StyleDef* getCustomStyles () const;

	// Layout
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	LayoutItem* createItem (View* view = nullptr) override;
	LayoutContext* createContext (LayoutView* parent) override;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	AnchorLayoutData layoutData;
};

//************************************************************************************************
// AnchorLayoutView
//************************************************************************************************

class AnchorLayoutView: public LayoutView
{
public:
	DECLARE_CLASS (AnchorLayoutView, LayoutView)

	AnchorLayoutView (const Rect& size, StyleRef style = 0, AnchorLayout* layout = nullptr);

	void makeCurrentSizesPreferred ();
	void forceSize (View* view, PointRef size); ///< try to size all views so that view gets the given size
	void onManipulationDone ();
	bool hasSavedState ();
	bool isLayoutSuspended () const;
	void setLayoutSuspended (bool state);
	void replaceAlgorithm (LayoutAlgorithm* newAlgorithm);

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// LayoutView
	void setLayout (Layout* layout) override;
	void attached (View* parent) override;
	void onChildLimitsChanged (View* child) override;
	void calcSizeLimits () override;
	void constrainSize (Rect& rect) const override;
	void flushLayout () override;
	void passDownSizeLimits () override;
	void setStyle (StyleRef style) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

protected:
	bool layoutSuspended;

	AnchorLayoutView ();

	IAttributeList* getLayoutState (tbool create);
	Divider* findNearDivider (MouseEvent& event);
	void doLayout ();

	void initAlgorithm ();
};

//************************************************************************************************
// BoxLayoutView
//************************************************************************************************

class BoxLayoutView: public AnchorLayoutView
{
public:
	DECLARE_CLASS (BoxLayoutView, AnchorLayoutView)

	BoxLayoutView (const Rect& rect = Rect (), StyleRef style = Styles::kHorizontal);

	void setMargin (int v) { setProperty (ATTR_MARGIN, v); }

	int getMargin () const
	{
		Variant v (kMinCoord);
		getProperty (v, ATTR_MARGIN);
		return v.asInt ();
	}

	void setSpacing (int v) { setProperty (ATTR_SPACING, v); }

	int getSpacing () const
	{
		Variant v (kMinCoord);
		getProperty (v, ATTR_SPACING);
		return v.asInt ();
	}
};

//************************************************************************************************
// AnchorLayoutAlgorithm
//************************************************************************************************

class AnchorLayoutAlgorithm: public LayoutAlgorithm
{
public:
	DECLARE_CLASS_ABSTRACT (AnchorLayoutAlgorithm, LayoutAlgorithm)

	AnchorLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData);

	virtual AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) = 0;
	virtual AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) = 0;

	virtual void flushLayout () {}
	virtual void constrainSize (Rect& rect) {}
	virtual void calcSizeLimits (SizeLimit& limits) {}

	// LayoutAlgorithm
	void onItemAdded (LayoutItem* item) override;
	void onItemInserted (int index, LayoutItem* item) override;
	void onItemRemoved (LayoutItem* item) override;

protected:
	AnchorLayoutContext* context;
	AnchorLayoutData& layoutData;

	bool isSizeMode (int flags) const;
};

//************************************************************************************************
// AnchorLayoutItem
//************************************************************************************************

class AnchorLayoutItem: public LayoutItem
{
public:
	DECLARE_CLASS (AnchorLayoutItem, LayoutItem)
	
	AnchorLayoutItem ();
	AnchorLayoutItem (View* view);

	SizeLimit sizeLimits;
	Point preferredSize;
	Rect workRect;	  ///< work rect when calculating layout
	float fillFactor; ///< how much does the view want to be stretched (grow & shrink); relative factor among sibling views
	int priority;
	int flags;

	PROPERTY_FLAG (flags, 1 << 1, preferredSizeLocked)
	PROPERTY_FLAG (flags, 1 << 2, isGroupDecorItem)

	void updateSize ();
	bool updateSizeLimits ();
	void updatePreferredSize ();

	// LayoutItem
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

private:
	void setSizeLimits (const SizeLimit& newLimits);
};

} // namespace CCL

#endif // _ccl_anchorlayout_h
