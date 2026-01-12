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
// Filename    : ccl/gui/layout/layoutview.h
// Description : A set of base classes for layout views
//
//************************************************************************************************

#ifndef _ccl_layoutview_h
#define _ccl_layoutview_h

#include "ccl/gui/views/view.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/base/singleton.h"
#include "ccl/base/typelib.h"

namespace CCL {

class LayoutItem;
class LayoutAlgorithm;
class LayoutView;

//************************************************************************************************
// LayoutContext
/** Functionality shared between LayoutViews and LayoutAlgorithms */
//************************************************************************************************

class LayoutContext: public Object
{
public:
	DECLARE_CLASS (LayoutContext, Object)

	LayoutContext (LayoutView* parentView);

	/** A layout's context must provide access to it's layout items */
	virtual ObjectArray& getLayoutItems () const;
	virtual LayoutItem* findLayoutItem (View* view) const;

	/** A layout's context must provide the layout's size and size mode */
	virtual RectRef getLayoutRect () const;
	virtual Coord getLayoutWidth () const;
	virtual Coord getLayoutHeight () const;
	virtual int getSizeMode () const;

	/** Layouts may request the context to perform an action */
	virtual void requestAutoSize (tbool horizontal = true, tbool vertical = true) const;
	virtual void requestResetSizeLimits (bool checkExplicit = true) const;

	/** Layout's may try to hide items if there is not enough space, or show items which have previously been hidden. */
	virtual void hideItem (LayoutItem* item) const;
	virtual void showItem (LayoutItem* item) const;

protected:
	LayoutView* parentView;

	LayoutContext ();
};

//************************************************************************************************
// Layout
/** Implementing classes arrange layout items according to an algorithm. @see LayoutItem, @see LayoutAlgorithm */
//************************************************************************************************

class Layout: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Layout, Object)

	/** Skins can define attributes in order to configure the layout algorithm */
	virtual bool setAttributes (const SkinAttributes& a) = 0;
	virtual bool getAttributes (SkinAttributes& a) const = 0;

	/** Layouts can create specialized layout items, algorithms & contexts. */
	virtual LayoutItem* createItem (View* view = nullptr) = 0;
	virtual LayoutContext* createContext (LayoutView* parent) = 0;
	virtual LayoutAlgorithm* createAlgorithm (LayoutContext* context) = 0;

	// IObject
	virtual tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	virtual tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// LayoutView
/** A view that arranges its subviews using a layout implementation, @see Layout */
//************************************************************************************************

class LayoutView: public View,
				  public ILayoutView
{
public:
	DECLARE_CLASS (LayoutView, View)
	
	LayoutView (const Rect& size, StyleRef style = 0, Layout* layout = nullptr);
	~LayoutView ();

	/** The layout used to size and position this view's children */
	Layout* getLayout ();
	virtual void setLayout (Layout* layout);

	/** Access to the layout context used by this view */
	LayoutContext* getContext ();
	
	/** The view size needed in order to fit it's child items	 */
	const Point& getPreferredSize ();
	
	/** Called after the view has been created and sized */
	void onViewCreated ();
	
	/** Access to the view's layout items */
	ObjectArray& getLayoutItems ();
	LayoutItem* findLayoutItem (View* view) const;

	// ILayoutView
	tbool CCL_API getLayoutAttributes (IAttributeList& properties) const override;
	tbool CCL_API getChildLayoutAttributes (IAttributeList& properties, IView* view) const override;
	
	// View
	void onSize (const Point& delta) override;
	void onViewsChanged () override;
	void onChildSized (View* child, const Point& delta) override;
	void onChildLimitsChanged (View* child) override;
	void calcSizeLimits () override {}
	void calcAutoSize (Rect& rect) override;
	void passDownSizeLimits () override;
	bool addView (View* view) override;
	bool insertView (int index, View* view) override;
	bool removeView (View* view) override;
	bool isAttached () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	class ViewIterator;

	CLASS_INTERFACE (ILayoutView, View)
	
protected:
	Rect initialSize;
	ObjectArray layoutItems;

	AutoPtr<LayoutContext> context;
	AutoPtr<Layout> layout; // Shared object, can be owned by us or something else (e.g. skin's LayoutElements)
	AutoPtr<LayoutAlgorithm> algorithm;

	bool internalCall;

	enum PrivateFlags
	{
		kAttachSuspended = 1<<(kLastPrivateFlag + 1)
	};

	PROPERTY_FLAG (privateFlags, kAttachSuspended, isAttachSuspended)

	class AttachSuspender;

	LayoutView ();

	LayoutAlgorithm* getAlgorithm ();
	void setAlgorithm (LayoutAlgorithm* newAlgorithm);
	void doLayout ();

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// LayoutView::ViewIterator
//************************************************************************************************

class LayoutView::ViewIterator: public ObjectArrayIterator
{
public:
	ViewIterator (LayoutView& layoutView);
	View* getView (Object* item);
	Object* next () override;
	Object* previous () override;
};

//************************************************************************************************
// LayoutItem
/** Wrapper around a child view which defines how it is to be arranged within a layout according to attributes and properties */
//************************************************************************************************

class LayoutItem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (LayoutItem, Object)

	LayoutItem ();
	LayoutItem (View* view);

	PROPERTY_FLAG (flags, 1 << 0, isHidden)

	PROPERTY_BY_REFERENCE (Rect, initialSize, InitialSize)
	PROPERTY_POINTER (View, view, View)
	Rect& getInitialSize ();
	
	/** Layout items can have attributes depending on the layout algorithm in use */
	virtual bool setAttributes (const SkinAttributes& a) = 0;
	virtual bool getAttributes (SkinAttributes& a) const = 0;

	// Object
	using Object::setProperty; // Property access should be public
	using Object::getProperty; // Property access should be public
	
protected:
	Rect initialSize;
	int flags;
};

//************************************************************************************************
// LayoutAlgorithm
/** Base class for layout algorithms */
//************************************************************************************************

class LayoutAlgorithm: public Object
{
public:
	DECLARE_CLASS (LayoutAlgorithm, Object)

	virtual const Point& getPreferredSize ();

	virtual void doLayout () {}
	virtual void onChildSized (View* child, const Point& delta) {}
	virtual void onChildLimitsChanged (View* child) {}
	virtual void onSize (const Point& delta) {}

	/** Layouts can react on item changes */
	virtual void onItemAdded (LayoutItem* item) {}
	virtual void onItemInserted (int index, LayoutItem* item) {}
	virtual void onItemRemoved (LayoutItem* item) {}
	virtual void onItemChanged (LayoutItem* item) {}

protected:
	Point preferredSize;
};

//************************************************************************************************
// LayoutFactory
/** Factory of layouts classes. Layouts can be created by their name. */
//************************************************************************************************

class LayoutFactory: public EnumTypeInfo
{
public:
	LayoutFactory ();

	static LayoutFactory& instance ();

	Layout* createLayout (StringID layoutName); ///< create a layout algorithm by name
	StringID getLayoutName (Layout* layout); /// get the name of an existing layout
	void registerLayout (StringID layoutName, MetaClassRef metaClass); ///< register a layout class

private:
	ObjectList layoutClasses;

	// EnumTypeInfo
	int CCL_API getEnumeratorCount () const override;
	tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override;
};

} // namespace CCL

#endif // _ccl_layoutview_h
