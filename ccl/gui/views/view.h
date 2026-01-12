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
// Filename    : ccl/gui/views/view.h
// Description : View class
//
//************************************************************************************************

#ifndef _ccl_view_h
#define _ccl_view_h

#include "ccl/gui/views/graphicsport.h"
#include "ccl/gui/theme/theme.h"

#include "ccl/base/collections/linkablelist.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"

namespace CCL {

class Window;
class Iterator;
class MouseHandler;
class MouseCursor;
class AccessibilityProvider;
interface ITouchHandler;
interface IDragHandler;
interface IRecognizer;
struct WindowUpdateInfo;

//************************************************************************************************
// View macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEachView : iterate thru sub-views
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachView(parent, var) \
{ CCL::ViewIterator __iter (parent); \
  while(CCL::View* var = (CCL::View*)__iter.next ()) {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEachViewFast : iterate thru sub-views.
// - faster than ForEachView
// - but does not allow to remove the current view during iteration!
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachViewFast(parent, var) \
{ CCL::FastViewIterator __iter (parent); \
  while(CCL::View* var = (CCL::View*)__iter.next ()) {

#define ForEachViewFastReverse(parent, var) \
{ CCL::FastViewIterator __iter (parent); \
  while(CCL::View* var = (CCL::View*)__iter.previous ()) {

//************************************************************************************************
// View
//************************************************************************************************

class View: public Linkable,
			public IView,
			public IViewChildren,
			public IGraphicsLayerContent,
			public IVisualStyleClient
{
public:
	DECLARE_CLASS (View, Linkable)

	View (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~View ();

	DECLARE_STYLEDEF (commonStyles)
	DECLARE_STYLEDEF (resizeStyles)
	DECLARE_STYLEDEF (propertyNames)
	DECLARE_METHOD_NAMES (View)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Nesting
	//////////////////////////////////////////////////////////////////////////////////////////////

	View* getParent () const;
	View* getParent (MetaClassRef typeId) const;
	template <class T> T* getParent () const;
	Iterator* newIterator () const;
	View* getFirst () const;
	View* getLast () const;
	int index (View* view) const;
	View* getChild (int index);

	virtual Window* getWindow ();
	virtual Window* getWindowForUpdate (WindowUpdateInfo& updateInfo);
	virtual bool isAttached ();

	virtual bool addView (View* view);
	virtual bool insertView (int index, View* view);
	virtual bool removeView (View* view);
	virtual bool moveViewBefore (View* view, View* before);
	virtual void onViewsChanged ();				///< sub-views have been added or removed

	virtual bool toFront (View* view);
	virtual bool toBack (View* view);

	virtual void attached (View* parent);
	virtual void removed (View* parent);

	virtual void onActivate (bool state);

	tbool CCL_API isEmpty () const override; ///< IViewChildren
	void CCL_API removeAll () override; ///< IViewChildren

	tbool CCL_API isChildView (IView* view, tbool deep = false) const override; ///< IViewChildren
	bool isChild (View* view, bool deep = false) const;
	View* findView (const Point& where, bool deep = false) const;
	void findAllViews (Container& cont, const Point& where, bool deep = false) const;
	View* findView (const IRecognizer& recognizer) const; ///< find matching child view deep

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Size
	//////////////////////////////////////////////////////////////////////////////////////////////

	Rect& getClientRect (Rect& r) const;
	RectRef CCL_API getSize () const override;								///< IView
	void CCL_API setSize (RectRef size, tbool invalidate = true) override;	///< IView
	tbool CCL_API getVisibleClient (Rect& r) const override;				///< IView
	tbool getVisibleClientForUpdate (Rect& r) const;						///< uses full layer size for layer backed views
	bool isResizing () const;
	void CCL_API setZoomFactor (float factor) override;						///< IView
	float CCL_API getZoomFactor () const override;							///< IView

	int getWidth () const;
	int getHeight () const;

	void setPosition (const Point& pos);
	Point& getPosition (Point& pos) const;
	Point getPosition () const;

	void setSizeMode (int flags);
	int getSizeMode () const;
	void disableSizeMode (bool state = true);
	bool isSizeModeDisabled () const;

	virtual void onSize (const Point& delta);
	virtual void onMove (const Point& delta);
	virtual void onChildSized (View* child, const Point& delta);
	virtual void onChildLimitsChanged (View* child);

	virtual void CCL_API autoSize (tbool horizontal = true, tbool vertical = true) override; ///< IView
	virtual void calcAutoSize (Rect& rect);

	void CCL_API setSizeLimits (const SizeLimit& sizeLimits) override; ///< IView
	const SizeLimit& CCL_API getSizeLimits () override; ///< IView
	tbool CCL_API hasExplicitSizeLimits () const override; ///< IView
	virtual void calcSizeLimits ();
	void resetSizeLimits ();
	void checkSizeLimits (); ///< changes size if necessary
	virtual void constrainSize (Rect& rect) const; ///< adjust proposed size if necessary
	virtual void flushLayout ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Attributes
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	StringRef getName () const;
	virtual void setName (StringRef name);

	StringRef getTitle () const;
	virtual void setTitle (StringRef title);

	StyleRef CCL_API getStyle () const override;
	virtual void setStyle (StyleRef style);

	Theme& getTheme () const;
	virtual void setTheme (Theme* theme);

	const IVisualStyle& CCL_API getVisualStyle () const override;
	virtual void setVisualStyle (VisualStyle* visualStyle);
	void onVisualStyleChanged () override; ///< IVisualStyleClient; also called internally by setVisualStyle (), to be called directly when making changes to the style itself
	bool hasVisualStyle () const;
	VisualStyle* getVisualStyleDirect () const;

	void setTooltip (StringRef tooltip);
	StringRef getTooltip () const;

	virtual StringRef getHelpIdentifier () const;
	virtual bool setHelpIdentifier (StringRef id); ///< only supported by some derived classes

	virtual IUnknown* CCL_API getController () const override;
	virtual tbool CCL_API setController (IUnknown* controller);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Drawing
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual int getThemeElementState () const;

	virtual GraphicsDevice* setGraphicsDevice (GraphicsDevice* device);	///< internal (returns old device)!
	virtual GraphicsDevice* getGraphicsDevice (Point& offset);			///< internal (recursive)!
	
	virtual void invalidate ();
	virtual void CCL_API invalidate (RectRef rect) override; ///< IView

	virtual void draw (const UpdateRgn& updateRgn);

	virtual void updateClient ();
	virtual void CCL_API updateClient (RectRef rect) override; ///< IView
	virtual void CCL_API redraw () override; ///< IView

	void CCL_API scrollClient (RectRef rect, PointRef delta) override; ///< IView

	PROPERTY_FLAG (privateFlags, kHasBeenDrawn, hasBeenDrawn)
	PROPERTY_FLAG (privateFlags, kIsHidden, isHidden)

	PROPERTY_FLAG (privateFlags, kTiledLayerMode, isTiledLayerMode)
	void setLayerBackingEnabled (bool state);
	bool isLayerBackingEnabled () const;
	IGraphicsLayer* getGraphicsLayer () const;
	
	virtual IGraphicsLayer* CCL_API getParentLayer (Point& offset) const override;

	void renderTo (GraphicsDevice& device, const UpdateRgn& updateRgn, PointRef offset = Point ()); ///< render view to given graphics device

	virtual void onDisplayPropertiesChanged (const DisplayChangedEvent& event); ///< points to pixel scaling changed, etc.
	virtual void onColorSchemeChanged (const ColorSchemeEvent& event);
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Mouse Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool isEnabled () const;
	void enable (bool state);

	virtual bool onMouseDown (const MouseEvent& event);
	virtual bool onMouseUp (const MouseEvent& event);
	virtual bool onMouseEnter (const MouseEvent& event);
	virtual bool onMouseMove (const MouseEvent& event);
	virtual bool onMouseLeave (const MouseEvent& event);

	virtual MouseHandler* createMouseHandler (const MouseEvent& event);

	virtual bool onMouseWheel (const MouseWheelEvent& event);

	virtual bool onContextMenu (const ContextMenuEvent& event);

	int getMouseState () const;
	bool setMouseState (int state);
	
	PROPERTY_FLAG (mouseState, kMouseDown, isMouseDown)
	PROPERTY_FLAG (mouseState, kMouseOver, isMouseOver)

	PROPERTY_FLAG (privateFlags, kTrackTooltip, isTooltipTrackingEnabled)

	virtual bool onTrackTooltip (const TooltipEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Multitouch Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual ITouchHandler* createTouchHandler (const TouchEvent& event);

	virtual bool onGesture (const GestureEvent& event); ///< position is in client coordinates

	PROPERTY_FLAG (privateFlags, kSuppressChildTouch, suppressesChildTouch)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Keyboard Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_FLAG (privateFlags, kWantsFocus, wantsFocus)
	PROPERTY_FLAG (privateFlags, kIgnoresFocus, ignoresFocus)
	PROPERTY_FLAG (privateFlags, kIsFocused, isFocused)
	PROPERTY_FLAG (privateFlags, kNoFocusOnContextMenu, noFocusOnContextMenu)
	
	virtual bool onFocus (const FocusEvent& event);
	virtual bool onKeyDown (const KeyEvent& event);
	virtual bool onKeyUp (const KeyEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Drag & Drop
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool onDragEnter (const DragEvent& event);
	virtual bool onDragOver (const DragEvent& event);
	virtual bool onDragLeave (const DragEvent& event);
	virtual bool onDrop (const DragEvent& event);

	virtual IDragHandler* createDragHandler (const DragEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Accessibility
	//////////////////////////////////////////////////////////////////////////////////////////////

	void setAccessibilityEnabled (bool state);
	bool isAccessibilityEnabled () const;
	virtual AccessibilityProvider* getAccessibilityProvider ();
	virtual AccessibilityProvider* getParentAccessibilityProvider ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Coordinates
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual Point& CCL_API clientToWindow (Point& p) const override;
	virtual Point& CCL_API windowToClient (Point& p) const override;
	virtual Point& CCL_API clientToScreen (Point& p) const override;
	virtual Point& CCL_API screenToClient (Point& p) const override;

	bool isInsideClient (const Point& where) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Misc. utilities
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual View* enterMouse (const MouseEvent& event, View* currentMouseView);	///< internal!
	View* dragEnter (const DragEvent& event);			///< internal!
	View* findFocusView (const MouseEvent& event);		///< internal!
	View* findTooltipView (const MouseEvent& event);	///< internal!
	bool tryMouseHandler (const MouseEvent& event);		///< internal!

	void setCursor (MouseCursor* cursor);
	tbool CCL_API detectDrag (const MouseEvent& event) override; ///< IView
	tbool CCL_API detectDoubleClick (const MouseEvent& event) override; ///< IVew

	tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) override; ///< IView
	tbool CCL_API takeFocus (tbool directed = true) override;	///< IView
	tbool CCL_API killFocus () override;	///< IView

	// Object
	void CCL_API addObserver (IObserver* observer) override;

	#if DEBUG
	void log (const char* indent = nullptr, int direction = 0);   // 1 childs, -1 parent, 0 just this
	#endif

	static Theme& getDefaultTheme ();
	static bool isRendering ();

	struct StyleModifier;

	CLASS_INTERFACE3 (IView, IViewChildren, IGraphicsLayerContent, Object)

protected:
	enum PrivateFlags
	{
		kDisabled			  = 1<<0,		///< the view is disabled
		kWantsFocus			  = 1<<1,		///< the view wants to receive the focus
		kIgnoresFocus		  = 1<<2,		///< clicking on this view does not steal another view's focus
		kIsFocused			  = 1<<3,		///< the view currently has the focus
		kNoFocusOnContextMenu = 1<<4,		///< the view does not want to receive the focus when context menu is opened
		kSizeLimitsValid	  = 1<<5,		///< sizeLimits were calculated or set explicitly
		kExplicitSizeLimits   = 1<<6,		///< sizeLimits were set explicitly from outside (e.g. from skin)
		kTrackTooltip		  = 1<<7,		///< view wants tooltip tracking events
		kActive				  = 1<<8,		///< the parent WindowBase is active
		kResizing			  = 1<<9,		///< the view is currently resizing
		kHasBeenDrawn		  = 1<<10,		///< view has been drawn
		kLayerBacking		  = 1<<11,		///< view wants to be backed by a graphics layer
		kTiledLayerMode		  = 1<<12,		///< view wants to be drawn by tiles (for layers only)
		kSuppressChildTouch	  = 1<<13,		///< child views are not queried for touch handlers
		kWasObserved		  = 1<<14,		///< addObserver was called at least once
		kIsHidden			  = 1<<15,		///< draw calls suppressed when view is hidden
		kAccessible			  = 1<<16,		///< accessibility support for view is enabled
		kWasDestroyed         = 1<<17,      ///< set in destructor to avoid side effects when children are removed
		kLastPrivateFlag	  = 17
	};

	enum PrivateConstants
	{
		kAttachDisabled		= 1<<(kLastSizeMode + 1),	///< disable size mode
		kAttachDisabledOnce	= 1<<(kLastSizeMode + 2)	///< disable size mode once, resets kAttachDisabled after onSize ()
	};

	friend class SizeLimitsMemento;
	friend class ViewIterator;
	friend class FastViewIterator;

	View* parent;
	LinkableList views;
	Rect size;
	SizeLimit sizeLimits;
	String name;
	String title;
	String tooltip;
	StyleFlags style;
	int sizeMode;
	int mouseState;
	int privateFlags;
	float zoomFactor;
	SharedPtr<Theme> theme;
	SharedPtr<VisualStyle> visualStyle;
	AutoPtr<IGraphicsLayer> graphicsLayer;
	GraphicsDevice* graphicsDevice;
	AccessibilityProvider* accessibilityProvider;

	class ViewIterator: public Object,
						public IViewIterator
	{
	public:
		ViewIterator (const View& view)
		: iter (view.newIterator ())
		{}

		// IViewIterator
		tbool CCL_API done () const override { return iter->done (); }
		IView* CCL_API next () override { return (View*)iter->next (); }
		IView* CCL_API previous () override { return (View*)iter->previous (); }
		void CCL_API last () override { return iter->last (); }

		CLASS_INTERFACE (IViewIterator, Object)

	protected:
		AutoPtr<Iterator> iter;
	};

	virtual void passDownSizeLimits ();
	void checkFitSize ();
	void checkInvalidate (const Point& delta);
	void propertyChanged (StringID propertyId);
	IGraphicsLayer* addGraphicsSublayer (IUnknown* content);
	void makeGraphicsLayer (bool state);
	void invalidateSubLayers ();
	virtual void makeAccessibilityProvider (bool state);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// IView
	tbool CCL_API getViewAttribute (Variant& value, AttrID id) const override;
	tbool CCL_API setViewAttribute (AttrID id, VariantRef value) override;
	IView* CCL_API getParentView () const override { return getParent (); }
	IView* CCL_API getParentByClass (UIDRef cid) const override;
	IViewChildren& CCL_API getChildren () const override { return *const_cast<View*> (this); }
	IWindow* CCL_API getIWindow () const override;
	void CCL_API setCursor (IMouseCursor* cursor) override;

	// IViewChildren
	tbool CCL_API add (IView* view) override;
	tbool CCL_API insert (int index, IView* view) override;
	tbool CCL_API remove (IView* view) override;
	tbool CCL_API moveBefore (IView* view, IView* before) override;
	IView* CCL_API getFirstView () const override { return getFirst (); }
	IView* CCL_API getLastView () const override { return getLast (); }
	IViewIterator* CCL_API createIterator () const override { return NEW ViewIterator (*this); }
	IView* CCL_API findChildView (PointRef where, tbool deep = false) const override;
	tbool CCL_API delegateEvent (const GUIEvent& event) override;

	// IGraphicsLayerContent
	void CCL_API drawLayer (IGraphics& graphics, const UpdateRgn& updateRgn, PointRef offset) override;
	LayerHint CCL_API getLayerHint () const override;
};

//************************************************************************************************
// FastViewIterator
//************************************************************************************************

class FastViewIterator: public FastLinkableListIterator
{
public:
	inline FastViewIterator (const View& parent): FastLinkableListIterator (parent.views) {}
};

//************************************************************************************************
// ViewIterator
//************************************************************************************************

class ViewIterator: public LinkableListIterator
{
public:
	inline ViewIterator (const View& parent): LinkableListIterator (parent.views) {}
};

//************************************************************************************************
// ThemeSelector
//************************************************************************************************

struct ThemeSelector
{
	ThemeSelector (Theme* newTheme)
	: oldTheme (currentTheme)
	{ currentTheme = newTheme; }

	ThemeSelector (Theme& newTheme)
	: oldTheme (currentTheme)
	{ currentTheme = &newTheme; }

	~ThemeSelector ()
	{ currentTheme = oldTheme; }
	
	Theme* oldTheme;
	static Theme* currentTheme;
};

//************************************************************************************************
// StyleModifier
/** Simplifies changing a view style.
	Example: 
		View::StyleModifier (*view).setCommonStyle (Styles::kHorizontal);     */
//************************************************************************************************

struct View::StyleModifier: public StyleFlags
{
	explicit inline StyleModifier (View& view)
	: StyleFlags (view.getStyle ()), view (view)
	{}

	inline ~StyleModifier ()
	{ view.setStyle (*this); }

private:
	View& view;
};

//************************************************************************************************
// SizeLimitsMemento
/** Allows storing & restoring the sizeLimits state of a view.*/
//************************************************************************************************

class SizeLimitsMemento
{
public:
	SizeLimitsMemento ();
	SizeLimitsMemento (View& view);

	void store (View& view);
	void restore (View& view);

	operator const SizeLimit& ();

private:
	SizeLimit limits;
	bool isExplicit;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Get an interface from a view or one of it's parents.
//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* GetViewInterfaceUpwards (UIDRef iid, View* view); ///< (implemented in usercontrolhost.cpp)
template <class T> T* GetViewInterfaceUpwards (View* view) { return (T*) GetViewInterfaceUpwards (ccl_iid<T> (), view); }

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
#define LOG_VIEW(view, indent, deep) { if(view) view->log (indent, deep); else CCL::Debugger::printf ("%s0\n", indent); }
#else
#define LOG_VIEW(view, indent, deep) {}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// View inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline View* View::getParent () const					{ return parent; }
template <class T> T* View::getParent () const			{ return (T*) getParent (ccl_typeid<T> ());}
inline Iterator* View::newIterator () const				{ return views.newIterator (); }
inline View* View::getFirst () const					{ return (View*)views.getFirst (); }
inline View* View::getLast () const						{ return (View*)views.getLast (); }
inline int View::index (View* view)	const				{ return views.index(view); }
inline View* View::getChild (int index)                 { return (View*)views.at (index); }
inline int View::getWidth () const						{ return size.getWidth (); }
inline int View::getHeight () const						{ return size.getHeight (); }
inline int View::getSizeMode () const					{ return sizeMode; }
inline void View::setSizeMode (int flags)				{ sizeMode = flags; }
inline StringRef View::getName () const					{ return name; }
inline StringRef View::getTitle () const				{ return title; }
inline StyleRef View::getStyle () const					{ return style; }
inline bool View::hasVisualStyle () const				{ return visualStyle != nullptr; }
inline VisualStyle* View::getVisualStyleDirect () const { return visualStyle; }
inline void View::setTooltip (StringRef _tooltip)		{ tooltip = _tooltip; }
inline StringRef View::getTooltip () const				{ return tooltip; }
inline bool View::isEnabled () const					{ return (privateFlags & kDisabled) == 0; }
inline void View::enable (bool state)					{ if(state) privateFlags &= ~kDisabled; else privateFlags |= kDisabled; }
inline bool View::isResizing () const					{ return (privateFlags & kResizing) != 0; }
inline bool View::isSizeModeDisabled () const			{ return (sizeMode & kAttachDisabled) != 0; }
inline bool View::isLayerBackingEnabled () const		{ return (privateFlags & kLayerBacking) != 0; }
inline IGraphicsLayer* View::getGraphicsLayer () const	{ return graphicsLayer; }
inline bool View::isAccessibilityEnabled () const		{ return (privateFlags & kAccessible) != 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// SizeLimitsMemento inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline SizeLimitsMemento::SizeLimitsMemento () : isExplicit (true) {}
inline SizeLimitsMemento::SizeLimitsMemento (View& view)	{ store (view); }
inline SizeLimitsMemento::operator const SizeLimit& ()		{ return limits; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_view_h
