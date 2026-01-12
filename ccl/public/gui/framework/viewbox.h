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
// Filename    : ccl/public/gui/framework/viewbox.h
// Description : View Box
//
//************************************************************************************************

#ifndef _ccl_viewbox_h
#define _ccl_viewbox_h

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/ivisualstyle.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

interface ITheme;
interface IWindow;
interface IForm;
interface IParameter;
interface IGraphicsLayer;

//************************************************************************************************
// ViewBox macros
//************************************************************************************************

/** Iterate child views (bottom-up). */
#define ForEachChildView(parent, var) \
{ CCL::AutoPtr<CCL::IViewIterator> __iter = (parent) ? (parent)->getChildren ().createIterator () : nullptr; \
  if(__iter) while(!__iter->done ()) { \
	  CCL::ViewBox var = __iter->next ();

/** Iterate child views in reverse order (top-down). */
#define ForEachChildViewReverse(parent, var) \
{ CCL::AutoPtr<CCL::IViewIterator> __iter = (parent) ? (parent)->getChildren ().createIterator () : nullptr; \
  if(__iter)__iter->last (); \
  if(__iter) while(!__iter->done ()) { \
	  CCL::ViewBox var = __iter->previous ();

//************************************************************************************************
// ViewBox
/** Helper class for convenient access to IView members ("box" around view).
	\ingroup gui_view */
//************************************************************************************************

class ViewBox
{
public:
	ViewBox (IView* view = nullptr)
	: view (view)
	{}

	ViewBox (IUnknown* unknown);

	/**	Create and initialize new view of given class.
		Be careful, ViewBox does not own the newly created view object! */
	ViewBox (UIDRef classID, RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	/** Set theme of current module. */
	static void setModuleTheme (ITheme* theme);

	/** Get theme of current module. */
	static ITheme* getModuleTheme ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	UIDRef getClassID () const;

	void setName (StringRef name);
	String getName () const;

	void setTitle (StringRef title);
	String getTitle () const;

	void setTooltip (StringRef title);
	String getTooltip () const;
	void setTooltipTrackingEnabled (bool state);
	bool isTooltipTrackingEnabled () const;

	StyleRef getStyle () const;
	void setStyle (StyleRef style);

	ITheme& getTheme () const;
	void setTheme (ITheme* theme);

	const IVisualStyle& getVisualStyle () const;
	void setVisualStyle (IVisualStyle* visualStyle);
	void setVisualStyle (const IVisualStyle& visualStyle);

	IUnknown* getController () const;

	bool isAttached () const;
	IWindow* getWindow () const;

	int getSizeMode () const;
	void setSizeMode (int flags);
	void disableSizeMode (bool state = true);

	bool isEnabled () const;
	void enable (bool state);

	int getMouseState () const;
	void setMouseState (int state);

	int getThemeElementState () const;

	bool wantsFocus () const;
	void wantsFocus (bool state);

	bool isLayerBackingEnabled () const;
	void setLayerBackingEnabled (bool state);
	IGraphicsLayer* getGraphicsLayer () const;

	bool isAccessibilityEnabled () const;
	void setAccessibilityEnabled (bool state);

	/** Get view-specific attribute via IObject. */
	bool getAttribute (Variant& value, const char* id) const;

	/** Set view-specific attribute via IObject. */
	bool setAttribute (const char* id, VariantRef value);

	bool setHelpIdentifier (StringRef id);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Size
	//////////////////////////////////////////////////////////////////////////////////////////////

	RectRef getSize () const;
	void setSize (RectRef size);
	Rect& getClientRect (Rect&) const;
	bool getVisibleClient (Rect& r) const;
	Coord getWidth () const;
	Coord getHeight () const;
	Point getPosition () const;
	void setPosition (PointRef pos);
	void setSizeLimits (const SizeLimit& sizeLimits);
	const SizeLimit& getSizeLimits () const;
	void setZoomFactor (float factor);
	float getZoomFactor () const;

	Point& clientToWindow (Point& p) const;
	Point& windowToClient (Point& p) const;
	Point& clientToScreen (Point& p) const;
	Point& screenToClient (Point& p) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Nesting
	//////////////////////////////////////////////////////////////////////////////////////////////

	IView* getParent () const;
	IView* getParent (UIDRef cid) const;
	IView* getViewAt (int index) const;
	int getViewIndex (const IView* childView) const;
	IViewChildren& getChildren () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	void autoSize (tbool horizontal = true, tbool vertical = true);
	void redraw ();
	void invalidate ();
	void invalidate (RectRef rect);
	void updateClient ();
	void updateClient (RectRef rect);
	void scrollClient (RectRef rect, PointRef delta);
	bool makeVisible (RectRef rect, tbool relaxed = false);
	bool takeFocus ();
	bool killFocus ();
	void setCursor (IMouseCursor* cursor);
	bool detectDrag (const MouseEvent& event);
	bool detectDoubleClick (const MouseEvent& event);

	ViewBox& operator = (IView* view);
	operator IView* ();
	IView* operator -> ();

	/** Cast to specified interface. */
	template <class T> T* as ();

	struct StyleModifier;

protected:
	static ITheme* moduleTheme;
	IView* view;

	void construct (RectRef size, StyleRef style, StringRef title);
};

//************************************************************************************************
// ViewBox::StyleModifier
/** Simplifies changing a view style.
	Example:
		ViewBox::StyleModifier (viewBox).setCommonStyle (Styles::kHorizontal);
	\ingroup gui_view */
//************************************************************************************************

struct ViewBox::StyleModifier: public StyleFlags
{
	explicit inline StyleModifier (ViewBox& view)
	: StyleFlags (view.getStyle ()), view (view)
	{}

	inline ~StyleModifier ()
	{ view.setStyle (*this); }

private:
	ViewBox& view;
};

//************************************************************************************************
// ControlBox
/** Box for controls.
	\ingroup gui_view */
//************************************************************************************************

class ControlBox: public ViewBox
{
public:
	ControlBox (IView* view = nullptr);

	/**	Create and initialize new control of given class.
		Be careful, ControlBox does not own the newly created view object! */
	ControlBox (UIDRef classID, IParameter* param = nullptr, RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	void setParameter (IParameter* param);
	IParameter* getParameter () const;
};

//************************************************************************************************
// FormBox
/** Box for form views. A form can create a window by itself.
	\ingroup gui_view */
//************************************************************************************************

class FormBox: public ViewBox
{
public:
	FormBox (IView* view = nullptr);

	/** Construct new form with given size, window style and title. */
	FormBox (RectRef size, StyleRef windowStyle = 0, StringRef title = nullptr);

	IForm* getForm () const;

	static bool isForm (IView* view);

	StyleFlags getWindowStyle () const;
	void setWindowStyle (StyleRef style);

	void setController (IUnknown* controller);

	IWindow* openWindow ();
	void closeWindow ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ViewBox inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ViewBox::isAttached () const
{ return getWindow () != nullptr; }

inline RectRef ViewBox::getSize () const
{ return view->getSize (); }

inline void ViewBox::setSize (RectRef size)
{ view->setSize (size); }

inline bool ViewBox::getVisibleClient (Rect& r) const
{ return view->getVisibleClient (r) != 0; }

inline Coord ViewBox::getWidth () const
{ return getSize ().getWidth (); }

inline Coord ViewBox::getHeight () const
{ return getSize ().getHeight (); }

inline Point ViewBox::getPosition () const
{ return getSize ().getLeftTop (); }

inline void ViewBox::autoSize (tbool horizontal, tbool vertical)
{ view->autoSize (horizontal, vertical); }

inline void ViewBox::setSizeLimits (const SizeLimit& sizeLimits)
{ view->setSizeLimits (sizeLimits); }

inline const SizeLimit& ViewBox::getSizeLimits () const
{ return view->getSizeLimits (); }

inline StyleRef ViewBox::getStyle () const
{ return view->getStyle (); }

inline const IVisualStyle& ViewBox::getVisualStyle () const
{ return view->getVisualStyle (); }

inline IViewChildren& ViewBox::getChildren () const
{ return view->getChildren (); }

inline IView* ViewBox::getParent () const
{ return view->getParentView (); }

inline IView* ViewBox::getParent (UIDRef cid) const
{ return view->getParentByClass (cid); }

inline void ViewBox::redraw ()
{ view->redraw (); }

inline void ViewBox::invalidate (RectRef rect)
{ view->invalidate (rect); }

inline void ViewBox::updateClient (RectRef rect)
{ view->updateClient (rect); }

inline bool ViewBox::takeFocus ()
{ return view->takeFocus () != 0; }

inline bool ViewBox::killFocus ()
{ return view->killFocus () != 0; }

inline void ViewBox::setCursor (IMouseCursor* cursor)
{ view->setCursor (cursor); }

inline bool ViewBox::detectDrag (const MouseEvent& event)
{ return view->detectDrag (event) != 0;  }

inline bool ViewBox::detectDoubleClick (const MouseEvent& event)
{ return view->detectDoubleClick (event) != 0; }

inline void ViewBox::setZoomFactor (float factor)
{ view->setZoomFactor (factor); }

inline float ViewBox::getZoomFactor () const
{ return view->getZoomFactor (); }

inline ViewBox& ViewBox::operator = (IView* _view)
{ view = _view; return *this; }

inline ViewBox::operator IView* ()
{ return view; }

inline IView* ViewBox::operator -> ()
{ return view; }

template <class T> T* ViewBox::as ()
{ return UnknownPtr<T> (view); }

/** Specialization: cast to IView. */
template<> inline IView* ViewBox::as ()
{ return view; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_viewbox_h
