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
// Filename    : core/portable/gui/corestaticview.h
// Description : Static view class
//
//************************************************************************************************

#ifndef _corestaticview_h
#define _corestaticview_h

#include "core/portable/coretypeinfo.h"
#include "core/portable/gui/coreviewshared.h"

#include "core/portable/corevalues.h"

namespace Core {
namespace Portable {

class StaticContainerView;
class StaticRootView;
class StaticListView;

//************************************************************************************************
// StaticThemePainter
/** \ingroup core_gui */
//************************************************************************************************

class StaticThemePainter: public ThemePainterBase,
						  public StaticSingleton<StaticThemePainter>
{
};

//************************************************************************************************
// StaticView
/** \ingroup core_gui */
//************************************************************************************************

class StaticView: public TypedObject,
				  public TViewBase<StaticView>
{
public:
	DECLARE_CORE_CLASS ('StVw', StaticView, TypedObject)

	StaticView (RectRef size = Rect ());

	virtual StaticContainerView* asContainer ();
	virtual StaticRootView* getRootView () const;
};

//************************************************************************************************
// StaticViewFilter
/** \ingroup core_gui */
//************************************************************************************************

struct StaticViewFilter
{
	virtual	bool matches (const StaticView* view) const = 0;
};

//************************************************************************************************
// StaticViewNameFilter
/** \ingroup core_gui */
//************************************************************************************************

struct StaticViewNameFilter: StaticViewFilter
{
	CStringPtr name;
	StaticViewNameFilter (CStringPtr name): name (name) {}

	// StaticViewFilter
	bool matches (const StaticView* view) const override { return view->getName () == name; }
};

//************************************************************************************************
// StaticContainerView
/** \ingroup core_gui */
//************************************************************************************************

class StaticContainerView: public ContainerViewBase<StaticView>
{
public:
	DECLARE_CORE_CLASS ('StCV', StaticContainerView, StaticView)

	StaticContainerView (RectRef size = Rect ());

	StaticView* findView (const StaticViewFilter& filter, bool deep = true) const;
	template <class T> T* findView (CStringPtr name, bool deep = true) const;

	// StaticView
	StaticContainerView* asContainer () override;
};

//************************************************************************************************
// StaticRootView
/** \ingroup core_gui */
//************************************************************************************************

class StaticRootView: public StaticContainerView,
					  public RootViewBase,
					  public IValueObserver
{
public:
	DECLARE_CORE_CLASS ('StRV', StaticRootView, StaticContainerView)

	StaticRootView (RectRef size = Rect (), BitmapPixelFormat pixelFormat = kBitmapRGBAlpha, RenderMode renderMode = kOffscreenMode);
	~StaticRootView ();

	void initOffscreen (Bitmap* offscreen1, Bitmap* offscreen2 = nullptr); // does _not_ take ownership of bitmaps!
	void setController (RootValueController* controller);

	// IValueObserver
	void valueChanged (ValueController* controller, int paramTag) override;

	// StaticView / RootViewBase
	StaticRootView* getRootView () const override;
	using StaticView::invalidate;
	void invalidate (RectRef rect) override;
	void draw (const DrawEvent& e) override;

	StaticView* getFocusView () const;
	virtual void setFocusView (StaticView* view);
	void findNextFocusView (StaticContainerView* container, bool forward, TypeID viewType = 0);

protected:
	RootValueController* rootController;
	StaticView* focusView;

	void updateView (StaticView* view, ValueController* controller, int paramTag);
};

//************************************************************************************************
// StaticViewConnector
/** \ingroup core_gui */
//************************************************************************************************

class StaticViewConnector
{
public:
	StaticViewConnector (ValueController* initialController);

	void connect (StaticView* view, bool state = true);

protected:
	ValueController* initialController;
};

//************************************************************************************************
// StaticLabel
/** \ingroup core_gui */
//************************************************************************************************

class StaticLabel: public StaticView
{
public:
	DECLARE_CORE_CLASS ('StLb', StaticLabel, StaticView)

	StaticLabel (RectRef size = Rect ());

	PROPERTY_VARIABLE (CStringPtr, title, Title)
	PROPERTY_FLAG (options, Skin::kLabelAppearanceColorize, isColorize)

	// StaticView
	void draw (const DrawEvent& e) override;
};

//************************************************************************************************
// StaticImageView
/** \ingroup core_gui */
//************************************************************************************************

class StaticImageView: public StaticContainerView
{
public:
	DECLARE_CORE_CLASS ('StIV', StaticImageView, StaticContainerView)

	StaticImageView (RectRef size = Rect ());

	PROPERTY_POINTER (Bitmap, image, Image)
	PROPERTY_FLAG (options, Skin::kImageViewAppearanceColorize, isColorize)
	PROPERTY_VARIABLE (float, imageAlpha, ImageAlpha)

	// StaticView
	void draw (const DrawEvent& e) override;
};

//************************************************************************************************
// StaticControlBase
/** \ingroup core_gui */
//************************************************************************************************

class StaticControlBase: public ITypedObject
{
public:
	DECLARE_CORE_CLASS_ ('StCB', StaticControlBase)

	StaticControlBase ();
	virtual ~StaticControlBase () {}

	PROPERTY_POINTER (ValueController, controller, Controller)
	PROPERTY_VARIABLE (int, paramTag, ParamTag)

	virtual void valueChanged () = 0;
};

//************************************************************************************************
// StaticVariantView
/** \ingroup core_gui */
//************************************************************************************************

class StaticVariantView: public StaticContainerView,
						 public StaticControlBase
{
public:
	BEGIN_CORE_CLASS ('StVV', StaticVariantView)
		ADD_CORE_CLASS_ (StaticControlBase)
	END_CORE_CLASS (StaticContainerView)

	StaticVariantView (RectRef size = Rect ());

	const ViewChildren& getVariants () const;

	// StaticContainerView + StaticControlBase
	void addView (StaticView* view) override;
	void valueChanged () override;

protected:
	ViewChildren variants;

	int getCurrentVariant () const;
	void selectVariant (int index);
};

//************************************************************************************************
// StaticControl
/** \ingroup core_gui */
//************************************************************************************************

class StaticControl: public StaticView,
					 public StaticControlBase
{
public:
	BEGIN_CORE_CLASS ('StCt', StaticControl)
		ADD_CORE_CLASS_ (StaticControlBase)
	END_CORE_CLASS (StaticView)

	StaticControl (RectRef size = Rect ());

	// StaticControlBase
	void valueChanged () override;
};

//************************************************************************************************
// StaticTextBox
/** \ingroup core_gui */
//************************************************************************************************

class StaticTextBox: public StaticControl
{
public:
	DECLARE_CORE_CLASS ('StTB', StaticTextBox, StaticControl)

	StaticTextBox (RectRef size = Rect ());

	typedef CString256 TextValue;

	// StaticControl
	void draw (const DrawEvent& e) override;

protected:
	void getText (TextValue& text) const;
	void drawText (Graphics& graphics, RectRef textRect);
};

//************************************************************************************************
// StaticButton
/** \ingroup core_gui */
//************************************************************************************************

class StaticButton: public StaticControl
{
public:
	DECLARE_CORE_CLASS ('StBt', StaticButton, StaticControl)

	StaticButton (RectRef size = Rect ());

	PROPERTY_VARIABLE (CStringPtr, title, Title)

	// StaticView
	void draw (const DrawEvent& e) override;
};

//************************************************************************************************
// StaticValueBar
/** \ingroup core_gui */
//************************************************************************************************

class StaticValueBar: public StaticControl
{
public:
	DECLARE_CORE_CLASS ('StVB', StaticValueBar, StaticControl)

	StaticValueBar (RectRef size = Rect ());

	PROPERTY_POINTER (Bitmap, background, Background)
	PROPERTY_POINTER (Bitmap, image, Image)
	PROPERTY_FLAG (options, Skin::kValueBarAppearanceVertical, isVertical)
	PROPERTY_FLAG (options, Skin::kValueBarAppearanceFilmstrip, isFilmstrip)

	// StaticControl
	void draw (const DrawEvent& e) override;

protected:
	float getNormalizedValue () const;
};

//************************************************************************************************
// IStaticViewPainter
/** \ingroup core_gui */
//************************************************************************************************

struct IStaticViewPainter: ITypedObject
{
	DECLARE_CORE_CLASS_ ('ISVP', IStaticViewPainter)
	
	virtual void drawView (const StaticView& view, const DrawEvent& e) = 0;
};

//************************************************************************************************
// StaticViewPainter
/** \ingroup core_gui */
//************************************************************************************************

class StaticViewPainter: public TypedObject,
						 public IStaticViewPainter
{
public:
	BEGIN_CORE_CLASS ('SVPt', StaticViewPainter)
		ADD_CORE_CLASS_ (IStaticViewPainter)
	END_CORE_CLASS (StaticViewPainter)
};

//************************************************************************************************
// StaticCustomView
/** \ingroup core_gui */
//************************************************************************************************

class StaticCustomView: public StaticView
{
public:
	DECLARE_CORE_CLASS ('SCst', StaticCustomView, StaticView)

	StaticCustomView (RectRef size = Rect ());

	PROPERTY_POINTER (IStaticViewPainter, painter, Painter)

	// StaticView
	void draw (const DrawEvent& e) override;
};

//************************************************************************************************
// StaticListViewModel
/** \ingroup core_gui */
//************************************************************************************************

class StaticListViewModel: public TypedObject,
						   public ListViewModelBase
{
public:
	DECLARE_CORE_CLASS ('StLM', StaticListViewModel, TypedObject)

	StaticListViewModel ();

	PROPERTY_POINTER (StaticListView, view, View)

	void changed ();
	void invalidate ();
};

//************************************************************************************************
// TStaticListViewModel
/** \ingroup core_gui */
//************************************************************************************************

template<int kMaxItems, int kMaxLength>
class TStaticListViewModel: public StaticListViewModel
{
public:
	bool addItem (CStringPtr title)
	{
		return items.add (title);
	}

	bool removeItemAt (int index)
	{
		return items.removeAt (index);
	}

	void removeAll ()
	{
		items.removeAll ();
	}

	// StaticListViewModel
	int getItemCount () const override
	{
		return items.count ();
	}

	CStringPtr getItemTitle (int index) const override
	{
		return items.at (index).str ();
	}

protected:
	typedef CStringBuffer<kMaxLength> Item;
	FixedSizeVector<Item, kMaxItems> items;
};

//************************************************************************************************
// StaticListView
/** \ingroup core_gui */
//************************************************************************************************

class StaticListView: public StaticView,
					  public ListViewStyle
{
public:
	DECLARE_CORE_CLASS ('StLV', StaticListView, StaticView)

	StaticListView (RectRef size = Rect ());

	void setModel (StaticListViewModel* model);
	void modelChanged ();

	void selectItem (int index);
	void makeSelectedItemVisible ();
	void makeItemVisible (int index);

	void scrollBy (int delta);
	void scrollTo (int index);
	int getScrollPosition () const;

	// StaticView
	void setSize (RectRef newSize) override;
	void draw (const DrawEvent& e) override;

protected:
	ListViewPainter painter;
	StaticListViewModel* model;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// StaticContainerView
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline T* StaticContainerView::findView (CStringPtr name, bool deep) const
{
	return core_cast<T> (findView (StaticViewNameFilter (name), deep));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _corestaticview_h
