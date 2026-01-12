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
// Filename    : ccl/platform/win/gui/accessibility.win.h
// Description : Windows Accessibility (UI Automation)
//
//************************************************************************************************

#ifndef _ccl_accessibility_win_h
#define _ccl_accessibility_win_h

#include "ccl/gui/system/accessibility.h"

#include "ccl/platform/win/system/cclcom.h"

#include <UIAutomation.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32::UIAutomationElementProvider
//************************************************************************************************

class UIAutomationElementProvider: public PlatformAccessibilityProvider,
								   public IRawElementProviderSimple,
								   public IRawElementProviderFragment, 
								   public IRawElementProviderFragmentRoot,
								   public IRawElementProviderAdviseEvents,
								   public IValueProvider,
								   public IRangeValueProvider, 
								   public IInvokeProvider,
								   public IToggleProvider,
								   //public ITextProvider,
								   public ITableProvider,
								   public ITableItemProvider,
								   public ISelectionProvider,
								   public ISelectionItemProvider,
								   public IScrollProvider,
								   public IScrollItemProvider,
								   public IExpandCollapseProvider
{
public:
	DECLARE_CLASS_ABSTRACT (UIAutomationElementProvider, PlatformAccessibilityProvider)

	UIAutomationElementProvider (AccessibilityProvider& owner);
	~UIAutomationElementProvider ();

	static UIAutomationElementProvider* toPlatformProvider (AccessibilityProvider* provider);

	// IRawElementProviderSimple
	HRESULT STDMETHODCALLTYPE get_ProviderOptions (ProviderOptions* pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetPatternProvider (PATTERNID patternId, ::IUnknown** pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetPropertyValue (PROPERTYID propertyId, VARIANT* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_HostRawElementProvider (IRawElementProviderSimple** pRetVal) override;

	// IRawElementProviderFragment
	HRESULT STDMETHODCALLTYPE Navigate (NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetRuntimeId (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_BoundingRectangle (UiaRect* pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE SetFocus () override;
	HRESULT STDMETHODCALLTYPE get_FragmentRoot (IRawElementProviderFragmentRoot** pRetVal) override;
	
	// IRawElementProviderFragmentRoot
	HRESULT STDMETHODCALLTYPE ElementProviderFromPoint (double x, double y, IRawElementProviderFragment** pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetFocus (IRawElementProviderFragment** pRetVal) override;
	
	// IRawElementProviderAdviseEvents
	HRESULT STDMETHODCALLTYPE AdviseEventAdded (EVENTID eventId, SAFEARRAY* propertyIDs) override;
	HRESULT STDMETHODCALLTYPE AdviseEventRemoved (EVENTID eventId, SAFEARRAY* propertyIDs) override;

	// IValueProvider
	HRESULT STDMETHODCALLTYPE SetValue (LPCWSTR val) override;
	HRESULT STDMETHODCALLTYPE get_Value (BSTR* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_IsReadOnly (BOOL* pRetVal) override;
	
	// IRangeValueProvider
	HRESULT STDMETHODCALLTYPE SetValue (double val) override;
	HRESULT STDMETHODCALLTYPE get_Value (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_Maximum (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_Minimum (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_LargeChange (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_SmallChange (double* pRetVal) override;

	// IInvokeProvider
	HRESULT STDMETHODCALLTYPE Invoke () override;

	// IToggleProvider
	HRESULT STDMETHODCALLTYPE Toggle () override;
	HRESULT STDMETHODCALLTYPE get_ToggleState (ToggleState* pRetVal) override;

	// ITextProvider
	//HRESULT STDMETHODCALLTYPE GetSelection (SAFEARRAY** pRetVal) override;
	//HRESULT STDMETHODCALLTYPE GetVisibleRanges (SAFEARRAY** pRetVal) override;
	//HRESULT STDMETHODCALLTYPE RangeFromChild (IRawElementProviderSimple* childElement, ITextRangeProvider** pRetVal) override;
	//HRESULT STDMETHODCALLTYPE RangeFromPoint (UiaPoint point, ITextRangeProvider** pRetVal) override;
	//HRESULT STDMETHODCALLTYPE get_DocumentRange (ITextRangeProvider** pRetVal) override;
	//HRESULT STDMETHODCALLTYPE get_SupportedTextSelection (SupportedTextSelection* pRetVal) override;

	// ITableProvider
	HRESULT STDMETHODCALLTYPE GetRowHeaders (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetColumnHeaders (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor (enum RowOrColumnMajor* pRetVal) override;
	
	// ITableItemProvider
	HRESULT STDMETHODCALLTYPE GetRowHeaderItems (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE GetColumnHeaderItems (SAFEARRAY** pRetVal) override;

	// ISelectionProvider 
	HRESULT STDMETHODCALLTYPE GetSelection (SAFEARRAY** pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_CanSelectMultiple (BOOL* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_IsSelectionRequired (BOOL* pRetVal) override;

	// ISelectionItemProvider
	HRESULT STDMETHODCALLTYPE Select () override;
    HRESULT STDMETHODCALLTYPE AddToSelection () override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection () override;
    HRESULT STDMETHODCALLTYPE get_IsSelected (BOOL* pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SelectionContainer (IRawElementProviderSimple** pRetVal) override;

	// IScrollProvider
	HRESULT STDMETHODCALLTYPE Scroll (ScrollAmount horizontalAmount, ScrollAmount verticalAmount) override;
	HRESULT STDMETHODCALLTYPE SetScrollPercent (double horizontalPercent, double verticalPercent) override;
	HRESULT STDMETHODCALLTYPE get_HorizontalScrollPercent (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_VerticalScrollPercent (double *pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_HorizontalViewSize (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_VerticalViewSize (double* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_HorizontallyScrollable (BOOL* pRetVal) override;
	HRESULT STDMETHODCALLTYPE get_VerticallyScrollable (BOOL* pRetVal) override;

	// IScrollItemProvider
	HRESULT STDMETHODCALLTYPE ScrollIntoView () override;

	// IExpandCollapseProvider
	HRESULT STDMETHODCALLTYPE Expand () override;
	HRESULT STDMETHODCALLTYPE Collapse () override;
	HRESULT STDMETHODCALLTYPE get_ExpandCollapseState (ExpandCollapseState* pRetVal) override;

	DELEGATE_COM_IUNKNOWN
		
	// PlatformAccessibilityProvider
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	void disconnect () override;
	void sendPlatformEvent (AccessibilityEvent e) override;

protected:
	static int toPlatformControlType (AccessibilityElementRole role);
	static int nextRuntimeId;
	
	int runtimeId;
	bool disconnected;
	
	template<typename T>
	HRESULT sharePlatformProvider (T** pRetVal, AccessibilityProvider* provider);

	bool isContentElement (const AccessibilityProvider& owner) const;

	// PlatformAccessibilityProvider
	void onChildProviderAdded (AccessibilityProvider* childProvider) override;
	void onChildProviderRemoved (AccessibilityProvider* childProvider) override;
};

//************************************************************************************************
// Win32::UIAutomationManager
//************************************************************************************************

class UIAutomationManager: public AccessibilityManager
{
public:
	// AccessibilityManager
	PlatformAccessibilityProvider* createPlatformProvider (AccessibilityProvider& provider) override;
	void shutdown () override;
	tbool CCL_API anyAccessibilityClientsListening () const override;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_accessibility_win_h
