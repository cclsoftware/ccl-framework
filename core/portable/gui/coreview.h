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
// Filename    : core/portable/gui/coreview.h
// Description : View class
//
//************************************************************************************************

#ifndef _coreview_h
#define _coreview_h

#include "core/portable/coretypeinfo.h"
#include "core/portable/gui/coreviewshared.h"

#include "core/public/coreenumdef.h"
#include "core/public/gui/coremultitouch.h"
#include "core/public/gui/coreviewinterface.h"

namespace Core {
namespace Portable {

class RootView;
class ContainerView;
class TouchInputState;

//************************************************************************************************
// TouchEvent
/** \ingroup core_gui */
//************************************************************************************************

struct TouchEvent
{
	enum Type
	{
		kDown,
		kMove,
		kUp
	};
	
	Type type;
	Point where;
	
	TouchEvent (Type type, PointRef where)
	: type (type),
	  where (where)
	{}
};

//************************************************************************************************
// GestureEvent
/** \ingroup core_gui */
//************************************************************************************************

struct GestureEvent
{
	int type;
	Point where;
	float amountX;
	float amountY;
	int& userData;
	
	GestureEvent (int& userData, int type = kGestureSingleTap|kGestureBegin, PointRef where = Point (), float amountX = 1., float amountY = 1.)
	: type (type),
	  where (where),
	  amountX (amountX),
	  amountY (amountY),
	  userData (userData)
	{}

	int getType () const { return type & kGestureTypeMask; }
	int getState () const { return type & kGestureStatesMask; }
};

typedef FixedSizeVector<int, 8> GestureVector;

//************************************************************************************************
// WheelEvent
/** \ingroup core_gui */
//************************************************************************************************

struct WheelEvent
{
	enum Axis
	{
		kHorizontal,
		kVertical
	};

	int delta;
	Axis axis;

	WheelEvent (int delta, Axis _axis = kHorizontal)
	: delta (delta),
	  axis (_axis)
	{}
};

//************************************************************************************************
// VirtualKeyEvent
/** \ingroup core_gui */
//************************************************************************************************

struct VirtualKeyEvent
{
	enum Type
	{
		kPrev,
		kNext
	};

	Type type;

	VirtualKeyEvent (Type type)
	: type (type)
	{}
};

//************************************************************************************************
// StyleManager
/** \ingroup core_gui */
//************************************************************************************************

class StyleManager: public StaticSingleton<StyleManager>
{
public:
	StyleManager ();
	~StyleManager ();

	/** Load styles from package defined in 'styles.json/.ubj' file. */
	int loadStyles (FilePackage& package);

	const Attributes* getStyle (CStringPtr name) const;

	static void addInheritedStyleAttributes (Attributes* style);
	static void preprocessStyleAttributes (Attributes& styleAttributes);

protected:
	HashMap<uint32, Attributes*> styleMap;

	void addStyle (CStringPtr name, Attributes* style);
};

//************************************************************************************************
// ThemePainter
/** \ingroup core_gui */
//************************************************************************************************

class ThemePainter: public ThemePainterBase,
					public StaticSingleton<ThemePainter>
{
public:
	void updateStyle ();
	
protected:
	void updateStyle (const Attributes& a);
};

//************************************************************************************************
// ViewAttributes
//************************************************************************************************

namespace ViewAttributes
{
	using namespace Skin::ViewAttributes;
	
	using ResourceAttributes::getSize;

	bool parseColor (Color& color, CStringPtr colorString);
	Color getColor (const Attributes& a, CStringPtr name, Color defaultColor);
	void decodeColor (Color& color, const AttributeValue& a);
	int getAlign (const Attributes& a, CStringPtr name, int defAlign = Alignment::kLeftCenter);
	void getStyle (Style& style, const Attributes& a);
	int getOptions (const Attributes& a, const EnumInfo info[], CStringPtr name = kOptions);
	int getExlusiveOption (const Attributes& a, const EnumInfo info[], CStringPtr name, int defaultValue);
	int getInt (const Attributes& a, CStringPtr name, int defValue);

	const Attributes* getStyleAttributes (const Attributes& a); ///< resolves shared styles via StyleManager

	BitmapReference getBitmap (const Attributes& a, CStringPtr name = kImage);
	bool autoSizeToBitmap (Rect& size, Bitmap* bitmap);
}

//************************************************************************************************
// ViewClasses
//************************************************************************************************

namespace ViewClasses
{
	using namespace Skin::ViewClasses;
}

#define DECLARE_CORE_VIEWCLASS(className) \
	CStringPtr getClassName () const override { return className; }

//************************************************************************************************
// View
/** \ingroup core_gui */
//************************************************************************************************

class View: public TypedObject,
			public TViewBase<View>,
			public ICoreView
{
public:
	DECLARE_CORE_CLASS ('View', View, TypedObject)

	View (RectRef size = Rect ());
	~View ();
	
	virtual CStringPtr getClassName () const { return ViewClasses::kView; }

	virtual ContainerView* asContainer ();
	virtual RootView* getRootView () const;
		
	bool isEnabled () const;
	void enable (bool state);
	
	PROPERTY_FLAG (options, kWantsTouch, wantsTouch)
	
	virtual bool onTouchInput (const TouchEvent& e);
	virtual bool onWheelInput (const WheelEvent& e);
	virtual bool onKeyInput (const VirtualKeyEvent& e);

	virtual bool onGestureInput (const GestureEvent& e);
	virtual void getHandledGestures (GestureVector& gestures, PointRef where);
	
	virtual void onIdle ();

	// TViewBase
	void setStyle (Style* style) override;

	// used by ViewBuilder:
	virtual void setAttributes (const Attributes& a);
	virtual CStringPtr getConnectionType () const;
	virtual void connect (void* object);

protected:
	enum Options
	{
		kDisabled = Skin::kViewBehaviorDisabled,
		kAlwaysDisabled = 1<<(kLastViewBaseFlag+1),
		kWantsTouch = 1<<(kLastViewBaseFlag+2),
	};

	PROPERTY_FLAG (options, kAlwaysDisabled, isAlwaysDisabled)

	// ICoreView
	void getProperty (Property& value) override;
	void setProperty (const Property& value) override { ASSERT (0) }
	void release () override { ASSERT (0) }
	int countSubViews () const override { return 0; }
	ICoreView* getSubViewAt (int index) const override { return nullptr; }
};

//************************************************************************************************
// ViewFilter
/** \ingroup core_gui */
//************************************************************************************************

struct ViewFilter
{
	virtual	bool matches (const View* view) const = 0;
};

//************************************************************************************************
// ViewNameFilter
/** \ingroup core_gui */
//************************************************************************************************

struct ViewNameFilter: ViewFilter
{
	CStringPtr name;
	ViewNameFilter (CStringPtr name): name (name) {}

	// ViewFilter
	bool matches (const View* view) const override { return view->getName () == name; }
};

//************************************************************************************************
// IViewOwner
/** \ingroup core_gui */
//************************************************************************************************

struct IViewOwner
{
	virtual	void viewDestroyed (View* view) = 0;
};

//************************************************************************************************
// ContainerView
/** \ingroup core_gui */
//************************************************************************************************

class ContainerView: public ContainerViewBase<View>
{
public:
	DECLARE_CORE_CLASS ('CntV', ContainerView, View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kContainerView)

	ContainerView (RectRef size = Rect ());
	~ContainerView ();

	PROPERTY_POINTER (IViewOwner, owner, Owner)

	virtual void removeAll ();
	virtual void resizeToChildren ();

	View* findView (const Point& where, bool deep = true, const ViewFilter* filter = nullptr) const;
	View* findView (const ViewFilter& filter, bool deep = true) const;
	template <class T> T* findView (CStringPtr name, bool deep = true) const;
	bool isChildView (const View* view, bool deep = true) const;

	// View / ContainerViewBase
	void addView (View* view) override;
	void removeView (View* view) override;
	ContainerView* asContainer () override;
	bool onTouchInput (const TouchEvent& e) override;
	void onIdle () override;

	// used by ViewBuilder:
	#if CORE_DEBUG_INTERNAL
	PROPERTY_CSTRING_BUFFER (64, sourceFile, SourceFile)
	#endif

protected:	
	// ICoreView
	void getProperty (Property& value) override;
	int countSubViews () const override;
	ICoreView* getSubViewAt (int index) const override;
};

//************************************************************************************************
// RootView
/** \ingroup core_gui */
//************************************************************************************************

class RootView: public ContainerView,
				public RootViewBase
{
public:
	DECLARE_CORE_CLASS ('RtVw', RootView, ContainerView)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kRootView)

	RootView (RectRef size = Rect (), BitmapPixelFormat pixelFormat = kBitmapRGBAlpha, RenderMode renderMode = kOffscreenMode);
	~RootView ();

	static void enableGestures (bool state);

	PROPERTY_BOOL (sizable, Sizable) ///< true if view is sizable (default is false)
	PROPERTY_POINTER (View, touchInputView, TouchInputView)

	bool scrollClient (RectRef rect, PointRef delta);

	View* getModalView () const;
	void setModalView (View* view);
	void resetModalViewDeferred ();

	View* getFocusView () const;
	virtual void setFocusView (View* view);
	void findFirstFocusView ();

	virtual void viewRemoved (View* view);

	bool receiveTouchInput (const TouchEvent& e);

	// ContainerView / RootViewBase
	RootView* getRootView () const override;
	using ContainerView::invalidate;
	void setSize (RectRef newSize) override;
	void invalidate (RectRef rect) override;
	void draw (const DrawEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	bool onWheelInput (const WheelEvent& e) override;
	bool onKeyInput (const VirtualKeyEvent& e) override;
	bool onGestureInput (const GestureEvent& e) override;
	void getHandledGestures (GestureVector& gestures, PointRef where) override;
	void onIdle () override;
		
protected:
	static bool gesturesEnabled;
	TouchInputState* touchInput;
	View* modalView;
	View* focusView;
	View* savedFocusView;
	bool modalResetPending;

	void setupOffscreenList ();
	void killModalView ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ContainerView
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline T* ContainerView::findView (CStringPtr name, bool deep) const
{
	return core_cast<T> (findView (ViewNameFilter (name), deep));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coreview_h
