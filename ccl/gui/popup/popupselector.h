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
// Filename    : ccl/gui/popup/popupselector.h
// Description : Popup Selector
//
//************************************************************************************************

#ifndef _ccl_popupselector_h
#define _ccl_popupselector_h

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/popupwindow.h"
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/popup/inativepopup.h"

namespace CCL {

class Menu;
class PopupSelectorClient;
class PopupSelectorWindow;
class AsyncOperation;

//************************************************************************************************
// IPopupSelectorWindow
/** Common interface for modal and non-modal popup selector window. */
//************************************************************************************************

interface IPopupSelectorWindow: IUnknown
{
	virtual IWindow* getParentWindow () const = 0;

	virtual IPopupSelectorClient* getClient () const = 0;

	virtual IPopupSelectorClient::Result getPopupResult () const = 0;

	virtual void setPopupResult (IPopupSelectorClient::Result result) = 0;

	virtual void closePopup () = 0;
	
	DECLARE_IID (IPopupSelector)
};

//************************************************************************************************
// PopupSelector
//************************************************************************************************

class PopupSelector: public Object,
					 public IPopupSelector,
					 public IWindowEventHandler
{
public:
	DECLARE_CLASS (PopupSelector, Object)

	PopupSelector ();
	~PopupSelector ();

	DECLARE_STYLEDEF (popupStyles)
	DECLARE_STRINGID_MEMBER (kPopupClosed)

	PROPERTY_BOOL (menuMode, MenuMode)
	PROPERTY_BOOL (nonModal, NonModal)
	PROPERTY_MUTABLE_CSTRING (decorName, DecorName)
	PROPERTY_SHARED_AUTO (IUnknown, decorController, DecorController)

	void setTheme (Theme* theme);
	void setTheme (const Theme& theme);
	Theme& getTheme () const;
	VisualStyle* getVisualStyle () const;
	void setDecorNameFromStyle (const IVisualStyle& vs);
	IPopupSelectorWindow* getCurrentWindow () const;
	IPopupSelectorClient::Result getPopupResult () const; ///< tells result code after popup
	int32 getBehavior () const;
	void adjustWindowSize (Rect& newSize);
	static bool didMouseHandlerEscape (Window* window, const MouseEvent& event);

	bool popup (Menu* menu, const PopupSizeInfo& sizeInfo, StringID menuType); ///< popup a menu

	IAsyncOperation* CCL_API popupAsync (IMenu* menu, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) override;
	IAsyncOperation* CCL_API popupAsync (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) override;
	IAsyncOperation* CCL_API popupAsync (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) override;

	// IPopupSelector
	void CCL_API setTheme (ITheme* theme) override;
	void CCL_API setVisualStyle (IVisualStyle* visualStyle) override;
	void CCL_API setBehavior (int32 behavior) override;
	void CCL_API setDecor (StringID decorName, IUnknown* decorController) override;
	tbool CCL_API popup (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) override;
	tbool CCL_API popup (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) override;
	tbool CCL_API popup (IMenu* menu, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) override;
	tbool CCL_API popup (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) override;
	tbool CCL_API popupSlider (IParameter* parameter, const PopupSizeInfo& sizeInfo, tbool horizontal) override;
	IAsyncOperation* CCL_API popupAsync (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) override;
	tbool CCL_API isOpen () override;
	void CCL_API close () override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	PROPERTY_FLAG (behavior, kCloseAfterDrag,		closeAfterDrag)
	PROPERTY_FLAG (behavior, kRestoreMousePos,		restoreMousePos)
	PROPERTY_FLAG (behavior, kWantsMouseUpOutside,	wantsMouseUpOutside)
	PROPERTY_FLAG (behavior, kHideHScroll,	hideHScroll)

	CLASS_INTERFACE (IPopupSelector, Object)

private:
	IAsyncOperation* doPopup (IView* popupView, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo);
	void onPopupClosed ();

	class MenuFinalizer;
	class NativeMenuFinalizer;
	class PopupDecorator;

	AutoPtr<IPopupSelectorWindow> currentWindow;
	IPopupSelectorClient::Result popupResult;
	Theme* theme;
	VisualStyle* visualStyle;
	Point oldMousePos;
	int monitor;
	int32 behavior;
};

DECLARE_VISUALSTYLE_CLASS (PopupSelector)

//************************************************************************************************
// PopupSelectorWindow (modal)
//************************************************************************************************

class PopupSelectorWindow: public Dialog,
						   public IPopupSelectorWindow
{
public:
	DECLARE_CLASS (PopupSelectorWindow, Dialog)

	PopupSelectorWindow (Window* parentWindow = nullptr, IPopupSelectorClient* client = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	PROPERTY_POINTER (PopupSelector, owner, Owner)
	PROPERTY_OBJECT (Rect, anchorRect, AnchorRect)

	bool onPopupDeactivated (); ///< true: swallow event
	bool isCloseRequested () const;

	// IPopupSelectorWindow
	IWindow* getParentWindow () const override { return parentWindow; }
	IPopupSelectorClient* getClient () const override { return client; }
	IPopupSelectorClient::Result getPopupResult () const override { return popupResult; }
	void setPopupResult (IPopupSelectorClient::Result _popupResult) override { popupResult = _popupResult; }
	void closePopup () override;

	// Window
	void attached (View* parent) override;
	void onActivate (bool state) override;
	bool onClose () override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	void onGestureProcessed (const GestureEvent& event, View* view) override;
	void CCL_API setSize (RectRef newSize, tbool doInvalidate = true) override;
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

	UNKNOWN_REFCOUNT

private:
	void closeWindow ();
	bool checkClientResult (IPopupSelectorClient::Result result);

	ObservedPtr<IWindow> parentWindow;
	AutoPtr<IPopupSelectorClient> client;
	IPopupSelectorClient::Result popupResult;
	bool parentAutoSeeThru;
	bool isClosing;

	#if CCL_PLATFORM_WINDOWS
	IAsyncOperation* showPlatformDialog (IWindow* parent) override;
	#endif
};

//************************************************************************************************
// NonModalPopupSelectorWindow
//************************************************************************************************

class NonModalPopupSelectorWindow: public PopupWindow,
								   public IPopupSelectorWindow
{
public:
	DECLARE_CLASS (NonModalPopupSelectorWindow, PopupWindow)

	NonModalPopupSelectorWindow (Window* parentWindow = nullptr, IPopupSelectorClient* client = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~NonModalPopupSelectorWindow ();

	PROPERTY_POINTER (PopupSelector, owner, Owner)
	
	AsyncOperation* getAsyncOperation () const;

	PROPERTY_VARIABLE (double, attachedTime, AttachedTime)

	static bool processForeignEvent (const GUIEvent& event, Window* window); ///< returns true if event should be swallowed

	// IPopupSelectorWindow
	IWindow* getParentWindow () const override { return parentWindow; }
	IPopupSelectorClient* getClient () const override { return client; }
	IPopupSelectorClient::Result getPopupResult () const override { return popupResult; }
	void setPopupResult (IPopupSelectorClient::Result _popupResult) override { popupResult = _popupResult; }
	void closePopup () override;

	// PopupWindow
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	void attached (View* parent) override;
	void CCL_API setSize (RectRef newSize, tbool doInvalidate) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	void onGestureProcessed (const GestureEvent& event, View* view) override;

	UNKNOWN_REFCOUNT

private:
	ObservedPtr<IWindow> parentWindow;
	AutoPtr<IPopupSelectorClient> client;
	IPopupSelectorClient::Result popupResult;
	AsyncOperation* asyncOperation;

	bool checkClientResult (IPopupSelectorClient::Result result);
	static bool shouldSwallowClosingEvent ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool PopupSelectorWindow::isCloseRequested () const
{ return isClosing; }

} // namespace CCL

#endif // _ccl_popupselector_h
