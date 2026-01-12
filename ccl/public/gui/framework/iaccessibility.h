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
// Filename    : ccl/public/gui/framework/iaccessibility.h
// Description : Accessibility Interfaces
// 
//************************************************************************************************

/**
	\addtogroup gui_accessibility
	
	CCL provides an accessibility abstraction layer and several implementations using platform-specific APIs, 
	allowing accessibility tools like screen readers to consume information about user interface elements in CCL-based applications. 

	Accessibility support is disabled by default. It can be enabled by setting the CCL.Accessibility.Enabled configuration value to "1" in cclgui.config for the application.

	If enabled, the CCL::IAccessibilityProvider interface is used to expose information about user interface elements.
	Additional information and methods for manipulation of elements can be exposed through additional interfaces like CCL::IAccessibilityToggleProvider, CCL::IAccessibilityScrollProvider, etc.

	Default implementations for framework views and controls like buttons, toggles, comboboxes etc. are readily available.

	CCL::IUserControl implementations may implement CCL::IUserControl::getCustomAccessibilityProvider in order to expose information through custom providers using these interfaces.
*/

#ifndef _ccl_iaccessibility_h
#define _ccl_iaccessibility_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IView;
interface IContainer;
interface IUnknownList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityElementRole
/** Accessibility element type.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityElementRole: int32
{
	kGroup,			///< Group of elements
	kRoot,			///< Root group (window or workspace frame)
	kCustom,		///< Custom control
	kList,			///< List of elements
	kTree,			///< Tree of elements
	kDataItem,		///< Data item, e.g. a node in a tree or an item in a list
	kHeader,		///< Header of a control, e.g. section containing column headers of a table or list
	kHeaderItem,	///< Header item, e.g. column header of a table or list
	kTabView,		///< View containing multiple tabs
	kTabItem,		///< Item in a tab view header that is used for selecting the visible tab
	kMenu,			///< Menu
	kMenuItem,		///< Menu item
	kLabel,			///< Static text
	kTextField,		///< Text edit control
	kButton,		///< Button
	kSlider,		///< Slider
	kComboBox		///< ComboBox providing a value from a list
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityElementState
/** Accessibility element states.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AccessibilityElementState
{
	constexpr int kEnabled = 1<<0;		///< element is enabled
	constexpr int kCanFocus = 1<<1;		///< element can have keyboard focus
	constexpr int kHasFocus = 1<<2;		///< element currently has keyboard focus
	constexpr int kIsPassword = 1<<3;	///< element is a password text/edit box
	constexpr int kTopLevel = 1<<4;		///< element is a top-level framework or user control
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityCoordSpace
/** Accessibility element coordinate space.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityCoordSpace: int32
{
	kScreen ///< screen coordinates
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityDirection
/** Accessibility element navigation direction.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityDirection: int32
{
	kParent,
	kNextSibling,
	kPreviousSibling,
	kFirstChild,
	kLastChild
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityScrollDirection
/** Accessibility element scroll direction.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityScrollDirection: int32
{
	kLeft,
	kRight,
	kUp,
	kDown,
	kUndefined
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityScrollAmount
/** Accessibility element scroll amount.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityScrollAmount: int32
{
	kPage,	///< Scroll by page (as if user pressed page up/down button)
	kStep,	///< Scroll by default step size (as if user pressed arrow key)
	kNone	///< Don't scroll
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityRelation
/** Accessibility element relation.
	\ingroup gui_accessibility */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityRelation: int32
{
	kProxy,		//< Denotes a provider that should be used instead of the default provider
	kLabel,		//< Denotes a provider for the label
	kValue,		//< Denotes a provider for the value
	kUndefined
};

//************************************************************************************************
// IAccessibilityProvider
/** Basic accessibility provider interface for elements on screen.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityProvider: IUnknown
{
	/** Get element type, used to determine possible interaction. */
	virtual AccessibilityElementRole CCL_API getElementRole () const = 0;

	/** Get element name, used by screen reader. */
	virtual void CCL_API getElementName (String& name) const = 0;

	/** Get current state of element (@see AccessibilityElementState). */
	virtual int CCL_API getElementState () const = 0;

	/** Set focus to this element. */
	virtual tresult CCL_API setElementFocus () = 0;

	/** Get element bounding rectangle in given coordinate space. */
	virtual tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const = 0;

	/** Get parent provider. */
	virtual IAccessibilityProvider* CCL_API getParentIProvider () const = 0;

	/** Get container with children. */
	virtual const IContainer* CCL_API getChildrenIProvider () const = 0;

	/** Find element by navigating in given direction. */
	virtual IAccessibilityProvider* CCL_API findElementIProvider (AccessibilityDirection direction) const = 0;

	/** Find element at position in given coordinate space. */
	virtual IAccessibilityProvider* CCL_API findElementIProviderAt (PointRef pos, AccessibilityCoordSpace space) const = 0;

	/** Get focus element provider. */
	virtual IAccessibilityProvider* CCL_API geFocusElementIProvider () const = 0;

	/** Get view owning this element. */
	virtual IView* CCL_API getIView () const = 0;

	/** Request to make the element visible (e.g. by scrolling). */
	virtual tresult CCL_API makeVisible (tbool relaxed = false) = 0;

	// Notifications
	DECLARE_STRINGID_MEMBER (kChildProviderAdded)	///< args[0]: IAccessibilityProvider
	DECLARE_STRINGID_MEMBER (kChildProviderRemoved)	///< args[0]: IAccessibilityProvider
	
	DECLARE_IID (IAccessibilityProvider)
};

DEFINE_IID (IAccessibilityProvider, 0xde78ede1, 0x82af, 0x4902, 0x86, 0xdd, 0xac, 0x48, 0x8f, 0xc0, 0xf2, 0xcf)
DEFINE_STRINGID_MEMBER (IAccessibilityProvider, kChildProviderAdded, "childProviderAdded")
DEFINE_STRINGID_MEMBER (IAccessibilityProvider, kChildProviderRemoved, "childProviderRemoved")

//************************************************************************************************
// AbstractAccessibilityProvider
//************************************************************************************************

class AbstractAccessibilityProvider: public IAccessibilityProvider
{
public:
	// IAccessibilityProvider
	void CCL_API getElementName (String& name) const override 
	{}
	int CCL_API getElementState () const override
	{ return 0; }
	tresult CCL_API setElementFocus () override
	{ return kResultNotImplemented; }
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override
	{ return kResultNotImplemented; }
	IAccessibilityProvider* CCL_API getParentIProvider () const override
	{ return nullptr; };
	const IContainer* CCL_API getChildrenIProvider () const override
	{ return nullptr; }
	IAccessibilityProvider* CCL_API findElementIProvider (AccessibilityDirection direction) const override 
	{ return nullptr; }
	IAccessibilityProvider* CCL_API findElementIProviderAt (PointRef pos, AccessibilityCoordSpace space) const override
	{ return nullptr; }
	IAccessibilityProvider* CCL_API geFocusElementIProvider () const override
	{ return nullptr; }
	IView* CCL_API getIView () const override
	{ return nullptr; }
	tresult CCL_API makeVisible (tbool relaxed = false) override
	{ return kResultNotImplemented; }
};

//************************************************************************************************
// IAccessibilityValueProvider
/** Additional accessibility interface for providers with values.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityValueProvider: IUnknown
{
	virtual tbool CCL_API isReadOnly () const = 0;

	virtual tresult CCL_API getValue (String& value) const = 0;
	
	virtual tresult CCL_API setValue (StringRef value) const = 0;

	virtual tbool CCL_API canIncrement () const = 0;

	virtual tresult CCL_API increment () const = 0;

	virtual tresult CCL_API decrement () const = 0;

	DECLARE_IID (IAccessibilityValueProvider)
};

DEFINE_IID (IAccessibilityValueProvider, 0xa20077bf, 0x22a5, 0x4877, 0xaa, 0x3, 0x5, 0xae, 0x32, 0x51, 0x6e, 0xd6)

//************************************************************************************************
// IAccessibilityActionProvider
/** Additional accessibility interface for providers with single actions (e.g. "push" or "invoke").
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityActionProvider: IUnknown
{
	virtual tresult CCL_API performAction () = 0;

	DECLARE_IID (IAccessibilityActionProvider)
};

DEFINE_IID (IAccessibilityActionProvider, 0x745cd46e, 0x4ac2, 0x4f2e, 0x88, 0x98, 0x3f, 0x4e, 0xe5, 0x51, 0x8f, 0x2b)

//************************************************************************************************
// IAccessibilityExpandCollapseProvider
/** Additional accessibility interface for expand / collapse providers for controls that visually expand to display more content.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityExpandCollapseProvider: IUnknown
{
	virtual tresult CCL_API expand (tbool state = true) = 0;

	virtual tbool CCL_API isExpanded () = 0;

	DECLARE_IID (IAccessibilityExpandCollapseProvider)
};

DEFINE_IID (IAccessibilityExpandCollapseProvider, 0x2046eeb4, 0xd3bd, 0x4429, 0xb0, 0x65, 0x89, 0xc9, 0xb7, 0x73, 0x0e, 0xcf)

//************************************************************************************************
// IAccessibilityToggleProvider
/** Additional accessibility interface for toggle providers.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityToggleProvider: IUnknown
{
	virtual tbool CCL_API isToggleOn () const = 0;
	
	virtual tresult CCL_API toggle () = 0;

	DECLARE_IID (IAccessibilityToggleProvider)
};

DEFINE_IID (IAccessibilityToggleProvider, 0x495e0399, 0x6258, 0x42d0, 0x80, 0x84, 0x11, 0xbf, 0x30, 0xdb, 0x28, 0x35)

//************************************************************************************************
// IAccessibilityTableProvider
/** Additional accessibility interface for tables.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityTableProvider: IUnknown
{
	virtual int CCL_API countColumns () const = 0;
	
	virtual IAccessibilityProvider* CCL_API getColumnHeaderProvider () = 0;
	
	virtual IAccessibilityProvider* CCL_API getColumnHeaderItemProvider (IAccessibilityProvider* dataItem) = 0;

	virtual int CCL_API countRows () const = 0;

	virtual IAccessibilityProvider* CCL_API  getRowHeaderProvider () = 0;

	virtual IAccessibilityProvider* CCL_API getRowHeaderItemProvider (IAccessibilityProvider* dataItem) = 0;
	
	DECLARE_IID (IAccessibilityTableProvider)
};

DEFINE_IID (IAccessibilityTableProvider, 0x326f02f, 0x18ad, 0x4918, 0x9f, 0xf2, 0x82, 0x91, 0xd5, 0x8e, 0x5f, 0xdf)

//************************************************************************************************
// IAccessibilitySelectionProvider
/** Additional accessibility interface for selectable elements.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilitySelectionProvider: IUnknown
{
	enum SelectionFlags
	{
		kExclusive = 0 ///< deselect others
	};

	/** Check if the element is selected. */
	virtual tbool CCL_API isSelected () const = 0;
	
	/** Select the element. */
	virtual tresult CCL_API select (tbool state = true, int flags = 0) = 0;

	/** Get the position of this element and the total number of elements in its container. */
	virtual tresult CCL_API getPosition (int& index, int& total) const = 0;
	
	virtual IAccessibilityProvider* CCL_API getSelectionContainerProvider () const = 0;

	DECLARE_IID (IAccessibilitySelectionProvider)
};

DEFINE_IID (IAccessibilitySelectionProvider, 0xe8ba21ec, 0xa2bf, 0x48f0, 0xa0, 0xdc, 0xf1, 0x4e, 0x46, 0xbf, 0x50, 0xa5)

//************************************************************************************************
// IAccessibilitySelectionContainerProvider
/** Additional accessibility interface for selection containers.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilitySelectionContainerProvider: IUnknown
{
	/** Get the list of selection providers in this container. */
	virtual tresult CCL_API getSelectionProviders (IUnknownList& selection) const = 0;
	
	/** Check if this container requires a selection. If true, at least one element is selected at any time. */
	virtual tbool CCL_API isSelectionRequired () const = 0;
	
	/** Check if this container allows selecting multiple elements. */
	virtual tbool CCL_API canSelectMultiple () const = 0;

	DECLARE_IID (IAccessibilitySelectionContainerProvider)
};

DEFINE_IID (IAccessibilitySelectionContainerProvider, 0x9af96b48, 0x2a73, 0x4a08, 0xaf, 0x34, 0xfb, 0xe3, 0xf2, 0x27, 0xde, 0x47)

//************************************************************************************************
// IAccessibilityScrollProvider
/** Additional accessibility interface for scrollable elements.
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityScrollProvider: IUnknown
{
	/** Check if this provider can scroll in the given direction. */
	virtual tbool CCL_API canScroll (AccessibilityScrollDirection direction) const = 0;
	
	/** Request to scroll content in the given direction by the given amount. */
	virtual tresult CCL_API scroll (AccessibilityScrollDirection direction, AccessibilityScrollAmount amount) = 0;
	
	/** Request to scroll content to the given normalized position. */
	virtual tresult CCL_API scrollTo (double normalizedX, double normalizedY) = 0;
	
	/** Get the current normalized horizontal position. */
	virtual double CCL_API getNormalizedScrollPositionX () const = 0;
	
	/** Get the current normalized vertical position. */
	virtual double CCL_API getNormalizedScrollPositionY () const = 0;
	
	/** Get the current horizontal page position. */
	virtual int CCL_API getPagePositionX () const = 0;
	
	/** Get the total number of horizontal pages. */
	virtual int CCL_API countPagesX () const = 0;
	
	/** Get the current vertical page position. */
	virtual int CCL_API getPagePositionY () const = 0;
	
	/** Get the total number of vertical pages. */
	virtual int CCL_API countPagesY () const = 0;

	DECLARE_IID (IAccessibilityScrollProvider)
};

DEFINE_IID (IAccessibilityScrollProvider, 0x72731edd, 0xa941, 0x48dd, 0xa8, 0x88, 0x3, 0xa8, 0xcc, 0x3a, 0x10, 0xc4)

//************************************************************************************************
// IAccessibilityManager
/** Accessibility manager singleton, access via System::GetAccessibilityManager().
	\ingroup gui_accessibility */
//************************************************************************************************

interface IAccessibilityManager: IUnknown
{
	/** Check if any accessibility client applications are currently listening. */
	virtual tbool CCL_API anyAccessibilityClientsListening () const = 0;

	DECLARE_IID (IAccessibilityManager)
};

DEFINE_IID (IAccessibilityManager, 0x361002aa, 0x1ada, 0x40b9, 0x91, 0x45, 0x64, 0x21, 0xce, 0xc3, 0xa, 0xa1)

} // namespace CCL

#endif // _ccl_iaccessibility_h
