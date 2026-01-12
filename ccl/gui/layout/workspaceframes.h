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
// Filename    : ccl/gui/layout/workspaceframes.h
// Description : Workspace FrameItem classes
//
//************************************************************************************************

#ifndef _ccl_workspaceframes_h
#define _ccl_workspaceframes_h

#include "ccl/gui/layout/dockpanel.h"
#include "ccl/gui/windows/windowbase.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/iviewstate.h"

namespace CCL {

class RootFrameItem;
class DetachedFrameItem;
class Perspective;
class Workspace;
class FrameView;
class WindowClass;
class AnchorLayoutView;
class Form;
class Parameter;
class AliasParam;
typedef const WindowClass& WindowClassRef;
interface IObjectFilter;

//************************************************************************************************
// FrameItem
//************************************************************************************************

class FrameItem: public DockPanelItem,
				 public IViewFactory
{
public:
	DECLARE_CLASS (FrameItem, DockPanelItem)
	DECLARE_STYLEDEF (customStyles)

	FrameItem (int style = 0);
	FrameItem (const FrameItem&);
	~FrameItem ();

	enum FrameStyles
	{
		kDividers	= 1<<1,		///< automatically insert dividers between childs
		kPopup		= 1<<2,		///< opens content in a floating popup window
		kMultiple	= 1<<3,		///< can be opened multiple times with different content
		kPinnable	= 1<<4,		///< can be pinned: the current content may not be replaced then
		kEmbedded	= 1<<5,		///< is embedded (inside an EmbeddedFrame)
		kDetached	= 1<<6,		///< is a target for detaching window classes
		kFill		= 1<<7,		///< frame is preferred to fill fill the container
		kRequired	= 1<<8,		///< frame is required
		kNoActivate	= 1<<9,		///< the frame's view (WindowBase) can't get activated
		kOtherMonitor = 1<<10,	///< for popup / detached frames: try to open on another monitor than the main window
		kMaximizable  = 1<<11,	///< for popup / detached frames: window can be maximized
		kFullscreen   = 1<<12,	///< for popup / detached frames: window can be fullscreen
		kFocusFrame   = 1<<13,	///< draw a rectangle around this frame when it's active
		kSystem		  = 1<<14,	///< opens content in a special system area specified by the frame's name (e\.g\. "StatusBar")
		kVolatile	  = 1<<15,	///< frame is inside an EmbeddedFrame that is not under control of the workspace
		kShared		  = 1<<16,	///< for detached frames: multiple detached frames can share a common popup, whose name is given as the first group name
		kHorizontal = 1<<17,	///< frame orientation horizontal
		kVertical = 1<<18		///< frame orientation vertical
	};

	void addGroupID (StringRef id);
	bool hasGroupID (StringRef id);

	virtual View* openView (WindowClassRef windowClass);
	bool isViewOpen (WindowClassRef windowClass);
	View* getViewForClass (WindowClassRef windowClass);
	bool isReallyVisible ();
	void signalWindowState (bool state);
	bool checkCondition (StringRef groupID) const;

	const WindowClass* getCurrentWindowClass ();
	IUnknown* getViewController () const;

	PROPERTY_MUTABLE_CSTRING (windowID, WindowID)
	PROPERTY_MUTABLE_CSTRING (defaultWindowID, DefaultWindowID)
	PROPERTY_MUTABLE_CSTRING (condition, Condition)
	PROPERTY_MUTABLE_CSTRING (decor, Decor)
	PROPERTY_STRING (friendID, FriendID)
	PROPERTY_OBJECT (Point, pos, Position) ///< only for PopupFrameItem
	PROPERTY_VARIABLE (float, fillFactor, FillFactor)

	PROPERTY_VARIABLE (int, style, Style)
	PROPERTY_FLAG (style, kDividers, hasDividers)
	PROPERTY_FLAG (style, kPopup, isPopup)
	PROPERTY_FLAG (style, kMultiple, isMultiple)
	PROPERTY_FLAG (style, kPinnable, isPinnable)
	PROPERTY_FLAG (style, kEmbedded, isEmbedded)
	PROPERTY_FLAG (style, kDetached, isDetachedFrame)
	PROPERTY_FLAG (style, kFill, isFill)
	PROPERTY_FLAG (style, kRequired, isRequired)
	PROPERTY_FLAG (style, kNoActivate, isNoActivate)
	PROPERTY_FLAG (style, kOtherMonitor, isOtherMonitor)
	PROPERTY_FLAG (style, kMaximizable, isMaximizable)
	PROPERTY_FLAG (style, kFullscreen, isFullscreen)
	PROPERTY_FLAG (style, kFocusFrame, isFocusFrame)
	PROPERTY_FLAG (style, kSystem, isSystem)
	PROPERTY_FLAG (style, kVolatile, isVolatile)
	PROPERTY_FLAG (style, kShared, isShared)
	PROPERTY_FLAG (style, kHorizontal, isHorizontal)
	PROPERTY_FLAG (style, kVertical, isVertical)

	RootFrameItem* getRootFrame () const;
	Perspective* getPerspective () const;
	Workspace* getWorkspace () const;
	FrameItem* findChildFrame (IRecognizer& recognizer);
	void collectChildFrames (Container& container, IObjectFilter& filter);

	void setPinned (bool state);
	virtual bool isPinned () const;
	bool wasPinned (StringID windowID) const;

	virtual Parameter* getPinnedParam ();
	virtual Parameter* getCloseParam ();
	virtual Parameter* getTitleParam ();

	void restoreView ();
	bool saveViewState ();
	void saveSize (PointRef size);
	ObjectList& getViewStates ();
	void initViewState (StringID windowID, StringID attribID, VariantRef value);

	IAttributeList* getLayoutState (StringID id, bool create);

	// DockPanelItem
	View* createView (Theme& theme) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	tbool CCL_API paramChanged (IParameter* param) override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	static FrameItem* createItem (int style);

	PROPERTY_FLAG (state, 1<<(kLastDockPanelItemFlag + 1), savingState)
	PROPERTY_FLAG (state, 1<<(kLastDockPanelItemFlag + 2), viewIsAppearing)
	PROPERTY_FLAG (state, 1<<(kLastDockPanelItemFlag + 3), viewIsLocked)

	CLASS_INTERFACE (IViewFactory, DockPanelItem)

protected:
	View* createMissingViews (ObjectList& items);
	bool applyFillFactor (FrameView* frameView, AnchorLayoutView* parentLayoutView);
	static void adjustChildView (View& childView, AnchorLayoutView& parentView);

	ObjectList groupIDs;
	ObjectList viewStates;
	bool restoringView;

	class ViewState: public Object, public IViewState
	{
	public:
		DECLARE_CLASS (ViewState, Object)

		ViewState (StringID windowID = nullptr);
		ViewState (const ViewState& other);

		PROPERTY_MUTABLE_CSTRING (windowID, WindowID)
		PROPERTY_OBJECT (Point, size, Size)
		PROPERTY_OBJECT (Point, pos, Position)
		PROPERTY_FLAG (flags, 1<<0, isPinned)	// flag values must not change! (saved as int)
		PROPERTY_FLAG (flags, 1<<1, isMaximized)
		PROPERTY_FLAG (flags, 1<<2, isFullscreen)
		PROPERTY_SHARED_AUTO (Attributes, viewState, ViewState)

		bool load (const Storage& storage) override;
		bool save (const Storage& storage) const override;

		// IViewState
		PointRef CCL_API getViewSize () const override { return size; }
		void CCL_API setViewSize (PointRef s) override { size = s; }

		CLASS_INTERFACE (IViewState, Object)

	private:
		int flags;
	};

	void setVisibleState (bool state);
	bool getSavedSize (Point& size);
	String getContentTitle () const;
	ViewState* lookupViewState (WindowClassRef windowClass, bool create, bool mayUseDefault = false);
	ViewState* lookupViewState (StringID viewStateID, bool create, bool mayUseDefault = false);

	static DockPanelItem* getNextVisible (DockPanelItem* parent, int startIndex, int direction);
	static void checkNeighbourDivider (DockPanelItem* parent, int startIndex, int direction, bool show = true);

	bool tryReuseFrameView ();
	void restoreViewState (WindowClass& windowClass);
	View* createViewInternal (WindowClass& windowClass);
	int getViewIndex (FrameItem* searchItem) const;

	friend class EmbeddedFrameItem;
	void resetContent ();

	virtual void restoreViews (Container& popupFrames);
	virtual void onChildHidden (FrameItem& child);
	virtual void onViewHidden ();
	virtual void saveViewStateInternal (ViewState& state);

	WindowClass* windowClass;
	Parameter* closeParam;
	Parameter* titleParam;
};

//************************************************************************************************
// FrameGroupItem
//************************************************************************************************

class FrameGroupItem: public FrameItem
{
public:
	DECLARE_CLASS (FrameGroupItem, FrameItem)

	FrameGroupItem (int style = 0);
	FrameGroupItem (const FrameGroupItem&);

	// DockPanelItem
	View* createView (Theme& theme) override;
	tbool CCL_API addItem (IDockPanelItem* item) override;
};

//************************************************************************************************
// RootFrameItem
//************************************************************************************************

class RootFrameItem: public FrameGroupItem
{
public:
	DECLARE_CLASS (RootFrameItem, FrameGroupItem)

	RootFrameItem ();
	RootFrameItem (const RootFrameItem&);

	void restoreViews ();
	void saveItemStates ();
	bool isHidingAll () const;

	// FrameItem
	void hideAll () override;

	PROPERTY_POINTER (Perspective, perspective, Perspective)

	void registerDetachedFrame (DetachedFrameItem* frame);
	void unregisterDetachedFrame (DetachedFrameItem* frame);
	DetachedFrameItem* findDetachedFrame (IRecognizer& recognizer); ///< find a detached frame (only when detached)

private:
	ObjectList detachedFrames;
	bool hidingAll;
};

//************************************************************************************************
// DividerItem
//************************************************************************************************

class DividerItem: public DockPanelItem
{
public:
	DECLARE_CLASS (DividerItem, DockPanelItem)

	DividerItem ();
	~DividerItem ();

	PROPERTY_BOOL (autoShow, AutoShow)
	PROPERTY_OBJECT (StyleFlags, style, Style)
	PROPERTY_VARIABLE (Coord, width, Width)
	PROPERTY_VARIABLE (Coord, outreach, Outreach)

	Parameter* getDividerParam ();
	void checkSyncSlaves ();

	// DockPanelItem
	View* createView (Theme& theme) override;
	IParameter* CCL_API findParameter (StringID name) const override;

private:
	Parameter* dividerParam;
};

//************************************************************************************************
// MultiFrameItem
/** Creates and removes child frames dynamicaly when needed. */
//************************************************************************************************

class MultiFrameItem: public FrameItem
{
public:
	DECLARE_CLASS (MultiFrameItem, FrameItem)

	static void suspendReuse (bool state);

	MultiFrameItem (int style = 0);

	FrameItem& newChildItem ();
	DetachedFrameItem* updateDetachedChilds ();
	void restoreDetachedChildState ();

	// FrameItem
	View* openView (WindowClassRef windowClass) override;
	void onChildHidden (FrameItem& child) override;
	void CCL_API show () override {}
	void CCL_API hide () override {}
	void hideAll () override;

private:
	bool inHideAll;
	int childCounter;
	static bool reuseSuspended;
};

//************************************************************************************************
// PopupFrameItem
//************************************************************************************************

class PopupFrameItem: public FrameItem,
					  public CCL::IWindowEventHandler
{
public:
	DECLARE_CLASS (PopupFrameItem, FrameItem)

	PopupFrameItem (int style = 0);
	PopupFrameItem (const PopupFrameItem&);
	~PopupFrameItem ();

	static PopupFrameItem* fromWindow (Window* window);

	Window* getWindow ();
	PROPERTY_VARIABLE (int, zIndex, ZIndex)

	// FrameItem
	void CCL_API show () override;
	void CCL_API hide () override;
	Parameter* getPinnedParam () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	void saveViewStateInternal (ViewState& state) override;
	void restoreViews (Container& popupFrames) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onViewHidden () override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	// Dockpanelitem
	tbool CCL_API paramChanged (IParameter* param) override;

	// Object
	int compare (const Object& obj) const override;

	CLASS_INTERFACE (IWindowEventHandler, FrameItem)

private:
	AliasParam* getTitleAlias ();
	Parameter* getMaximizedParam ();
	Parameter* getFullscreenParam ();
	void setTitleParam (IParameter* param);
	void maximize (bool state);
	void setFullscreen (bool state);
	void tryOtherMonitor (Form& form);

	Parameter* pinned;
	Parameter* maximized;
	Parameter* fullscreen;
	AliasParam* titleAlias;
};

//************************************************************************************************
// DetachedFrameItem
//************************************************************************************************

class DetachedFrameItem: public PopupFrameItem
{
public:
	DECLARE_CLASS (DetachedFrameItem, PopupFrameItem)

	DetachedFrameItem (int style = 0);
	DetachedFrameItem (const DetachedFrameItem&);
	~DetachedFrameItem ();

	bool isDetached () const;		/// tells if this frame is in detached state (can be the case even if the window is closed)
	void setDetached (bool state);	/// enable/disable detached state

	// FrameItem
	void onViewHidden () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	Parameter* detachedParam;
	bool detached;

	Parameter* getDetachedParam ();
};

//************************************************************************************************
// SharedDetachedFrameItem
//************************************************************************************************

class SharedDetachedFrameItem: public DetachedFrameItem
{
public:
	DECLARE_CLASS (SharedDetachedFrameItem, DetachedFrameItem)

	using DetachedFrameItem::DetachedFrameItem;

	// DetachedFrameItem
	View* openView (WindowClassRef windowClass) override;
	void onViewHidden () override;

private:
	SharedDetachedFrameItem* findOpenItem ();
};

//************************************************************************************************
// EmbeddedFrameItem
//************************************************************************************************

class EmbeddedFrameItem: public FrameItem
{
public:
	DECLARE_CLASS (EmbeddedFrameItem, FrameItem)

	EmbeddedFrameItem (int style = 0);
	EmbeddedFrameItem (const EmbeddedFrameItem&);

	MutableCString getParentClassID () const;
	void setParentClassID (StringID classID);

	void onFrameViewAttached (View* frameView);
	void onFrameViewRemoved ();

	// FrameItem
	tbool CCL_API addItem (IDockPanelItem* item) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

private:
	static const CString kPropertyPrefix;

	MutableCString parentClassID;
	class MarkAsEmbedded;
	class ResetViewPointers;
	class UpdateWindowState;
	static bool initDefaultContent (FrameItem& item);
};

//************************************************************************************************
// EmbeddedFrameView
//************************************************************************************************

class EmbeddedFrameView: public View
{
public:
	DECLARE_CLASS_ABSTRACT (EmbeddedFrameView, View)

	EmbeddedFrameView (const Rect& size);

	PROPERTY_MUTABLE_CSTRING (workspaceID, WorkspaceID)
	StringRef getFrameID () const { return getName (); }

	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)

	// View
	void attached (View* parent) override;
	void removed (View* parent) override;
	void calcSizeLimits () override;

private:
	SharedPtr<EmbeddedFrameItem> frameItem;
};

//************************************************************************************************
// SystemFrameItem
//************************************************************************************************

class SystemFrameItem: public FrameItem
{
public:
	DECLARE_CLASS (SystemFrameItem, FrameItem)

	SystemFrameItem (int style = 0);
	SystemFrameItem (const SystemFrameItem&);

	// FrameItem
	void CCL_API show () override;
	void CCL_API hide () override;

private:
	SystemFrameItem* findCounterpartInAppWorkspace () const;
};

//************************************************************************************************
// FrameView
//************************************************************************************************

class FrameView: public WindowBase,
				 public ILayoutStateProvider
{
public:
	DECLARE_CLASS_ABSTRACT (FrameView, WindowBase)

	FrameView (FrameItem* frameItem, const Rect& size);

	FrameItem* getFrameItem () const;

	void setContent (View* view);
	void signalOnActivate ();

	PROPERTY_OBJECT (Point, originalViewSize, OriginalViewSize)

	static Form* findContentForm (const View& outerView);

	// WindowBase
	bool canActivate () const override;
	bool onMouseDown (const MouseEvent& event) override;
	void calcSizeLimits () override;
	void onChildLimitsChanged (View* child) override;
	void onActivate (bool state) override;
	void draw (const UpdateRgn& updateRgn) override;

	// ILayoutStateProvider
	IAttributeList* CCL_API getLayoutState (StringID id, tbool create = false) override;

	CLASS_INTERFACE (ILayoutStateProvider, WindowBase)

private:
	SharedPtr<FrameItem> frameItem;
	Coord frameWidth;
	Color frameColor;

	void updateStyle ();
	static StringRef findHelpIdentifierDeep (View& view);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool RootFrameItem::isHidingAll () const { return hidingAll; }

inline bool DetachedFrameItem::isDetached () const { return detached; }

} // namespace CCL

#endif // _ccl_workspaceframes_h
