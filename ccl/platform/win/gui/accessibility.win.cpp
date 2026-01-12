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
// Filename    : ccl/platform/win/gui/accessibility.win.cpp
// Description : Windows Accessibility (UI Automation)
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/accessibility.win.h"
#include "ccl/platform/win/gui/screenscaling.h"

#include "ccl/gui/windows/desktop.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#pragma comment(lib, "Uiautomationcore.lib")

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// Win32::UIAutomationElementProvider
//************************************************************************************************

UIAutomationElementProvider* UIAutomationElementProvider::toPlatformProvider (AccessibilityProvider* provider)
{
	UIAutomationElementProvider* elementProvider = provider ? ccl_cast<UIAutomationElementProvider> (provider->getPlatformProvider ()) : nullptr;
	ASSERT (elementProvider != nullptr || provider == nullptr)
	if(elementProvider && elementProvider->disconnected)
		return nullptr;
	return elementProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
HRESULT UIAutomationElementProvider::sharePlatformProvider (T** pRetVal, AccessibilityProvider* provider)
{
	*pRetVal = toPlatformProvider (provider);
	if(*pRetVal != nullptr)
	{
		(*pRetVal)->AddRef ();
		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UIAutomationElementProvider::toPlatformControlType (AccessibilityElementRole role)
{
	switch(role)
	{
	case AccessibilityElementRole::kGroup : return UIA_GroupControlTypeId;
	case AccessibilityElementRole::kRoot : return UIA_WindowControlTypeId;
	case AccessibilityElementRole::kList : return UIA_ListControlTypeId;
	case AccessibilityElementRole::kTree : return UIA_TreeControlTypeId;
	case AccessibilityElementRole::kDataItem : return UIA_TextControlTypeId; // UIA_DataItemControlTypeId;
	case AccessibilityElementRole::kHeader : return UIA_HeaderControlTypeId;
	case AccessibilityElementRole::kHeaderItem : return UIA_HeaderItemControlTypeId;
	case AccessibilityElementRole::kTabView : return UIA_TabControlTypeId;
	case AccessibilityElementRole::kTabItem : return UIA_TabItemControlTypeId;
	case AccessibilityElementRole::kMenu : return UIA_MenuControlTypeId;
	case AccessibilityElementRole::kMenuItem : return UIA_MenuItemControlTypeId;
	case AccessibilityElementRole::kLabel : return UIA_TextControlTypeId;
	case AccessibilityElementRole::kTextField : return UIA_EditControlTypeId;
	case AccessibilityElementRole::kButton : return UIA_ButtonControlTypeId;
	case AccessibilityElementRole::kSlider : return UIA_SliderControlTypeId;
	case AccessibilityElementRole::kComboBox : return UIA_ComboBoxControlTypeId;
	default :
		ASSERT (role == AccessibilityElementRole::kCustom)
		return UIA_CustomControlTypeId;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UIAutomationElementProvider::isContentElement (const AccessibilityProvider& owner) const
{
	if(owner.getElementRole () == AccessibilityElementRole::kRoot 
			|| owner.getElementRole () == AccessibilityElementRole::kGroup
			|| owner.getElementRole () == AccessibilityElementRole::kLabel)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (UIAutomationElementProvider, PlatformAccessibilityProvider)
int UIAutomationElementProvider::nextRuntimeId = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAutomationElementProvider::UIAutomationElementProvider (AccessibilityProvider& owner)
: PlatformAccessibilityProvider (owner),
  runtimeId (nextRuntimeId++),
  disconnected (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAutomationElementProvider::~UIAutomationElementProvider ()
{
	ASSERT (disconnected)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UIAutomationElementProvider::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IRawElementProviderSimple)
	QUERY_COM_INTERFACE (IRawElementProviderFragment)
	QUERY_COM_INTERFACE (IRawElementProviderFragmentRoot)
	QUERY_COM_INTERFACE (IRawElementProviderAdviseEvents)
	QUERY_COM_INTERFACE (IValueProvider)
	QUERY_COM_INTERFACE (IRangeValueProvider)
	QUERY_COM_INTERFACE (IInvokeProvider)
	QUERY_COM_INTERFACE (IToggleProvider)
	QUERY_COM_INTERFACE (ITableProvider)
	QUERY_COM_INTERFACE (ITableItemProvider)
	//QUERY_COM_INTERFACE (ITextProvider)
	QUERY_COM_INTERFACE (ISelectionProvider)
	QUERY_COM_INTERFACE (ISelectionItemProvider)
	QUERY_COM_INTERFACE (IScrollProvider)
	QUERY_COM_INTERFACE (IScrollItemProvider)
	QUERY_COM_INTERFACE (IExpandCollapseProvider)

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutomationElementProvider::disconnect ()
{
	if(disconnected)
		return;

	ASSERT ((InSendMessageEx (nullptr) & (ISMEX_REPLIED|ISMEX_SEND)) != ISMEX_SEND)
	HRESULT hr = ::UiaDisconnectProvider (this);
	ASSERT (hr != RPC_E_CANTCALLOUT_ININPUTSYNCCALL)

	disconnected = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutomationElementProvider::sendPlatformEvent (AccessibilityEvent e)
{
	if(disconnected)
		return;
	
	switch(e)
	{
	case AccessibilityEvent::kValueChanged :
		{
			ComVariant value;
			 GetPropertyValue (UIA_ValueValuePropertyId, &value);
			::UiaRaiseAutomationPropertyChangedEvent (this, UIA_ValueValuePropertyId, ComVariant (), value);
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutomationElementProvider::onChildProviderAdded (AccessibilityProvider* childProvider)
{
	SuperClass::onChildProviderAdded (childProvider);
	if(!disconnected)
		::UiaRaiseAutomationEvent (this, UIA_StructureChangedEventId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutomationElementProvider::onChildProviderRemoved (AccessibilityProvider* childProvider)
{
	if(!disconnected)
		::UiaRaiseAutomationEvent (this, UIA_StructureChangedEventId);
	SuperClass::onChildProviderRemoved (childProvider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IRawElementProviderSimple
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_ProviderOptions (ProviderOptions* pRetVal)
{
	if(!pRetVal)
		return E_INVALIDARG;
	
	*pRetVal = ProviderOptions_ServerSideProvider|ProviderOptions_UseComThreading;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetPatternProvider (PATTERNID patternId, ::IUnknown** pRetVal)
{
	ASSERT (System::IsInMainThread ())	
	if(!pRetVal)
		return E_INVALIDARG;

	*pRetVal = nullptr;
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	switch(patternId)
	{
	case UIA_ValuePatternId :
		if(getValueProvider ().hasInterface<CCL::IAccessibilityValueProvider> ())
		{
			*pRetVal = static_cast<IValueProvider*> (this);
			retain ();
		}
		break;
	
	case UIA_InvokePatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilityActionProvider> () || getEffectiveProvider ().getElementRole () == AccessibilityElementRole::kButton)
		{
			*pRetVal = static_cast<IInvokeProvider*> (this);
			retain ();
		}
		break;

	case UIA_TogglePatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilityToggleProvider> ())
		{
			*pRetVal = static_cast<IToggleProvider*> (this);
			retain ();
		}
		break;

	case UIA_TablePatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilityTableProvider> ())
		{
			*pRetVal = static_cast<ITableProvider*> (this);
			retain ();
		}
		break;

	case UIA_TableItemPatternId :
		if(getEffectiveProvider ().getElementRole () == AccessibilityElementRole::kDataItem)
		{
			*pRetVal = static_cast<ITableItemProvider*> (this);
			retain ();
		}
		break;
		
	case UIA_SelectionPatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilitySelectionContainerProvider> ())
		{
			*pRetVal = static_cast<ISelectionProvider*> (this);
			retain ();
		}
		break;

	case UIA_SelectionItemPatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilitySelectionProvider> ())
		{
			*pRetVal = static_cast<ISelectionItemProvider*> (this);
			retain ();
		}
		break;

	case UIA_ScrollPatternId :
		if(getEffectiveProvider ().hasInterface<CCL::IAccessibilityScrollProvider> ())
		{
			*pRetVal = static_cast<IScrollProvider*> (this);
			retain ();
		}
		break;
		
	case UIA_ScrollItemPatternId :
		*pRetVal = static_cast<IScrollItemProvider*> (this);
		retain ();
		break;

	case UIA_ExpandCollapsePatternId :
		*pRetVal = static_cast<IExpandCollapseProvider*> (this);
		retain ();
		break;

	//case UIA_TextPatternId :
	//	if(owner.getElementRole () == AccessibilityElementRole::kLabel)
	//	{
	//		*pRetVal = static_cast<ITextProvider*> (this);
	//		retain ();
	//	}
	//	break;
	}
	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetPropertyValue (PROPERTYID propertyId, VARIANT* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	::VariantInit (pRetVal); // this parameter is passed uninitialized

	switch(propertyId)
	{
	case UIA_AutomationIdPropertyId :
		{
			String id;
			id.appendIntValue (runtimeId);
			ComVariant::convert (*pRetVal, id);
		}
		break;

	case UIA_RuntimeIdPropertyId  :
		ComVariant::fromInt32Vector (*pRetVal, {UiaAppendRuntimeId, runtimeId});
		break;

	case UIA_IsControlElementPropertyId :
		ComVariant::convert (*pRetVal, !disconnected);
		break;
	}

	// properties below need access to the owner, so return if we already disconnected
	if(disconnected)
		return S_OK;

	switch(propertyId)
	{
	case UIA_IsContentElementPropertyId :
		ComVariant::convert (*pRetVal, isContentElement (getEffectiveProvider ()));
		break;

	case UIA_ControlTypePropertyId :
		{
			AccessibilityElementRole role = getEffectiveProvider ().getElementRole ();
			if(role == AccessibilityElementRole::kRoot && !ccl_cast<Window> (getEffectiveProvider ().getView ()))
				role = AccessibilityElementRole::kGroup; // don't report workspace frames as windows

			int platformType = toPlatformControlType (role);

			if(platformType == UIA_ListControlTypeId && getEffectiveProvider ().hasInterface<CCL::IAccessibilityTableProvider> ())
				platformType = UIA_TableControlTypeId;
			
			ComVariant::convert (*pRetVal, platformType);
			pRetVal->vt = VT_I4;
		}
		break;

	case UIA_DescribedByPropertyId :
		{
			AccessibilityManager& manager = AccessibilityManager::instance ();
			if(IRawElementProviderSimple* provider = toPlatformProvider (&getLabelProvider ()))
			{
				::IUnknown* unknown = provider;
				SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (&unknown, 1);
				ComVariant::takeSafeArray (*pRetVal, safeArray.detach (), VT_UNKNOWN);
			}
		}
		break;

	case UIA_IsValuePatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getValueProvider ().hasInterface<CCL::IAccessibilityValueProvider> ());
		break;
		
	case UIA_IsInvokePatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilityActionProvider> ());
		break;

	case UIA_IsTogglePatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilityToggleProvider> ());
		break;

	case UIA_IsTablePatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilityTableProvider> ());
		break;
		
	case UIA_IsTableItemPatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().getElementRole () == AccessibilityElementRole::kDataItem);
		break;
		
	case UIA_IsSelectionPatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilitySelectionContainerProvider> ());
		break;
		
	case UIA_IsSelectionItemPatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilitySelectionProvider> ());
		break;
		
	case UIA_IsScrollPatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilityScrollProvider> ());
		break;
		
	case UIA_IsScrollItemPatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, true);
		break;
		
	case UIA_IsExpandCollapsePatternAvailablePropertyId :
		ComVariant::convert (*pRetVal, getEffectiveProvider ().hasInterface<CCL::IAccessibilityExpandCollapseProvider> ());
		break;
		
	case UIA_BoundingRectanglePropertyId :
		{
			Rect screenRect;
			owner.getElementBounds (screenRect, AccessibilityCoordSpace::kScreen);
			gScreens->toPixelRect (screenRect);
			ComVariant::fromDoubleVector (*pRetVal, {double(screenRect.left), double(screenRect.top), double(screenRect.getWidth ()), double(screenRect.getHeight ())});
		}
		break;
	
	case UIA_CenterPointPropertyId :
		{
			Rect screenRect;
			owner.getElementBounds (screenRect, AccessibilityCoordSpace::kScreen);
			gScreens->toPixelRect (screenRect);
			Point center = screenRect.getCenter ();
			ComVariant::fromDoubleVector (*pRetVal, {double(center.x), double(center.y)});
		}
		break;

	case UIA_ClickablePointPropertyId :
		if(isContentElement (getEffectiveProvider ()))
			return GetPropertyValue (UIA_CenterPointPropertyId, pRetVal);
		return UIA_E_NOCLICKABLEPOINT;

	case UIA_NamePropertyId :
		{
			String name;
			getEffectiveProvider ().getElementName (name);
			ComVariant::convert (*pRetVal, name);
		}
		break;
	
	case UIA_FullDescriptionPropertyId :
		// TODO: add interface method?
		{
			//String description;
			//owner.getElementName (description);
			//if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = owner.asUnknown ())
			//{
			//	String value;
			//	if(valueProvider->getValue (value) == kResultOk)
			//		description.append (" ").append (value);
			//}
			//ComVariant::convert (*pRetVal, description);
		}
		break;

	case UIA_HasKeyboardFocusPropertyId :
		ComVariant::convert (*pRetVal, get_flag (getEffectiveProvider ().getElementState (), AccessibilityElementState::kHasFocus));
		break;

	case UIA_IsKeyboardFocusablePropertyId :
		ComVariant::convert (*pRetVal, get_flag (getEffectiveProvider ().getElementState (), AccessibilityElementState::kCanFocus));
		break;

	case UIA_IsEnabledPropertyId :
		ComVariant::convert (*pRetVal, get_flag (getEffectiveProvider ().getElementState (), AccessibilityElementState::kEnabled));
		break;

	case UIA_IsPasswordPropertyId :
		ComVariant::convert (*pRetVal, get_flag (getEffectiveProvider ().getElementState (), AccessibilityElementState::kIsPassword));
		break;

	case UIA_FrameworkIdPropertyId :
		ComVariant::convert (*pRetVal, String (CCL_SHORT_NAME));
		break;

	case UIA_ValueValuePropertyId :
		if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
		{
			String value;
			if(valueProvider->getValue (value) == kResultOk)
				ComVariant::convert (*pRetVal, value);
		}
		break;

	case UIA_ValueIsReadOnlyPropertyId :
		if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
			ComVariant::convert (*pRetVal, valueProvider->isReadOnly ());
		break;

	case UIA_PositionInSetPropertyId : CCL_FALLTHROUGH
	case UIA_SizeOfSetPropertyId :
		if(UnknownPtr<CCL::IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
		{
			int index = 0;
			int total = 0;
			if(selectionProvider->getPosition (index, total) == kResultOk)
			{
				if(propertyId == UIA_PositionInSetPropertyId)
					ComVariant::convert (*pRetVal, index + 1);
				else
					ComVariant::convert (*pRetVal, total);
				pRetVal->vt = VT_I4;
			}
		}
		break;
	}	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_HostRawElementProvider (IRawElementProviderSimple** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	*pRetVal = nullptr;
	if(disconnected)
		return S_OK;

	// for non-toplevel elements, return nullptr
	if(owner.getParentProvider () && owner.getElementRole () != AccessibilityElementRole::kRoot)
		return S_OK;

	// for toplevel elements, return the default provider of the window
	HWND hwnd = NULL;
	if(auto* view = owner.getView ())
		if(auto* window = view->getWindow ())
			hwnd = HWND(window->getSystemWindow ());

	if(hwnd)
		return ::UiaHostProviderFromHwnd (hwnd, pRetVal);
	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IRawElementProviderFragment
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Navigate (NavigateDirection direction, IRawElementProviderFragment** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	*pRetVal = nullptr;
	if(disconnected)
		return S_OK;

	AccessibilityProvider* result = nullptr;
	switch(direction)
	{
	case NavigateDirection_Parent :
		result = owner.findElementProvider (AccessibilityDirection::kParent);
		break;

	case NavigateDirection_NextSibling :
		result = owner.findElementProvider (AccessibilityDirection::kNextSibling);
		break;

	case NavigateDirection_PreviousSibling :
		result = owner.findElementProvider (AccessibilityDirection::kPreviousSibling);
		break;

	case NavigateDirection_FirstChild :
		result = getEffectiveProvider ().findElementProvider (AccessibilityDirection::kFirstChild);
		break;

	case NavigateDirection_LastChild :
		result = getEffectiveProvider ().findElementProvider (AccessibilityDirection::kLastChild);
		break;
	}

	sharePlatformProvider (pRetVal, result);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetRuntimeId (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	// Implementations should return NULL for a top-level element that is hosted in a window.
	if(!disconnected && owner.getParentProvider () == nullptr)
	{
		*pRetVal = nullptr;
		return S_OK;
	}

	int values[] = {UiaAppendRuntimeId, runtimeId};
	SafeArray<VT_I4, int> safeArray (values, ARRAY_COUNT (values));
	*pRetVal = safeArray.detach ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_BoundingRectangle (UiaRect* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	Rect screenRect;
	owner.getElementBounds (screenRect, AccessibilityCoordSpace::kScreen);
	gScreens->toPixelRect (screenRect);

	pRetVal->left = double(screenRect.left);
	pRetVal->top = double(screenRect.top);
	pRetVal->width = double(screenRect.getWidth ());
	pRetVal->height = double(screenRect.getHeight ());
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetEmbeddedFragmentRoots (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;

	*pRetVal = nullptr;	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::SetFocus ()
{
	ASSERT (System::IsInMainThread ())
		
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	return HRESULT(owner.setElementFocus ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_FragmentRoot (IRawElementProviderFragmentRoot** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;

	AccessibilityProvider* result = nullptr;
	if(disconnected == false)
		for(AccessibilityProvider* current = &owner; current != nullptr; current = current->getParentProvider ())
		{
			if(get_flag (current->getElementState (), AccessibilityElementState::kTopLevel))
				result = current;
		}

	sharePlatformProvider (pRetVal, result);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IRawElementProviderFragmentRoot
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::ElementProviderFromPoint (double x, double y, IRawElementProviderFragment** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	*pRetVal = nullptr;

	Point screenPos;
	screenPos.x = Coord(x);
	screenPos.y = Coord(y);
	gScreens->toCoordPoint (screenPos);

	AccessibilityProvider* deepestProvider = nullptr;
	AccessibilityProvider* current = &owner;
	
	if(AccessibilityManager::instance ().findRelatedProvider (current, AccessibilityRelation::kProxy) == nullptr)
	{
		while(AccessibilityProvider* provider = current ? current->findElementProviderAt (screenPos, AccessibilityCoordSpace::kScreen) : nullptr)
		{
			deepestProvider = provider;
			current = provider;
		
			if(AccessibilityManager::instance ().findRelatedProvider (current, AccessibilityRelation::kProxy) != nullptr)
				break;
		}
	}
	else
		deepestProvider = current;
	sharePlatformProvider (pRetVal, deepestProvider);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetFocus (IRawElementProviderFragment** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	AccessibilityProvider* focusElement = nullptr;
	AccessibilityProvider* current = &owner;
	
	while(current)
	{
		if(auto* element = current->getFocusElementProvider ())
			focusElement = element;
		current = current->findElementProvider (AccessibilityDirection::kFirstChild);
	}

	sharePlatformProvider (pRetVal, focusElement);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::AdviseEventAdded (EVENTID eventId, SAFEARRAY* propertyIDs)
{
	// Method exists simply to tell UIA that we want to receive event registrations
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::AdviseEventRemoved (EVENTID eventId, SAFEARRAY* propertyIDs)
{
	// Method exists simply to tell UIA that we want to receive event registrations
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IValueProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::SetValue (LPCWSTR val)
{
	ASSERT (System::IsInMainThread ())
		
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
	{
		String value (val);
		return static_cast<HRESULT> (valueProvider->setValue (value));
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_Value (BSTR* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	*pRetVal = nullptr;
	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
	{
		String value;
		tresult result = valueProvider->getValue (value);
		if(result == kResultOk)
		{
			*pRetVal = value.createNativeString<BSTR> ();
			return S_OK;
		}
		return static_cast<HRESULT> (result);
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_IsReadOnly (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ();
	*pRetVal = valueProvider ? valueProvider->isReadOnly () : true;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::SetValue (double val)
{
	ASSERT (System::IsInMainThread ())
		
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
	{
		String value (val);
		return static_cast<HRESULT> (valueProvider->setValue (value));
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_Value (double* pRetVal)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_Maximum (double* pRetVal)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_Minimum (double* pRetVal)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_LargeChange (double* pRetVal)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_SmallChange (double* pRetVal)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IInvokeProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Invoke ()
{
	ASSERT (System::IsInMainThread ())
		
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityActionProvider> actionProvider = getEffectiveProvider ().asUnknown ())
		return static_cast<HRESULT> (actionProvider->performAction ());
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IToggleProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Toggle ()
{
	ASSERT (System::IsInMainThread ())
		
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = getEffectiveProvider ().asUnknown ())
		return static_cast<HRESULT> (toggleProvider->toggle ());
	return E_FAIL;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_ToggleState (ToggleState* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = getEffectiveProvider ().asUnknown ())
	{
		*pRetVal = toggleProvider->isToggleOn () ? ToggleState_On : ToggleState_Off;
		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ITextProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetSelection (SAFEARRAY** pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetVisibleRanges (SAFEARRAY** pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::RangeFromChild (IRawElementProviderSimple* childElement, ITextRangeProvider** pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::RangeFromPoint (UiaPoint point, ITextRangeProvider** pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_DocumentRange (ITextRangeProvider** pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_SupportedTextSelection (SupportedTextSelection* pRetVal)
//{
//	ASSERT (System::IsInMainThread ())
//	if(!pRetVal)
//		return E_INVALIDARG;
//	
//	if(disconnected)
//		return UIA_E_ELEMENTNOTAVAILABLE;
//
//	return E_NOTIMPL;
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ITableProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetRowHeaders (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityTableProvider> tableProvider = getEffectiveProvider ().asUnknown ())
	{
		AccessibilityProvider* provider = unknown_cast<AccessibilityProvider> (tableProvider->getRowHeaderProvider ());
		if(provider)
		{
			::UIAutomationElementProvider* rawElementProvider = nullptr;
			if(sharePlatformProvider (&rawElementProvider, provider) == S_OK)
			{
				::IUnknown* result = static_cast<::IRawElementProviderSimple*> (rawElementProvider);
				SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (&result, 1);
				*pRetVal = safeArray.detach ();
			}
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetColumnHeaders (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityTableProvider> tableProvider = getEffectiveProvider ().asUnknown ())
	{
		AccessibilityProvider* provider = unknown_cast<AccessibilityProvider> (tableProvider->getColumnHeaderProvider ());
		if(provider)
		{
			::UIAutomationElementProvider* rawElementProvider = nullptr;
			if(sharePlatformProvider (&rawElementProvider, provider) == S_OK)
			{
				::IUnknown* result = static_cast<::IRawElementProviderSimple*> (rawElementProvider);
				SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (&result, 1);
				*pRetVal = safeArray.detach ();
			}
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_RowOrColumnMajor (enum RowOrColumnMajor* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = RowOrColumnMajor_Indeterminate;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	if(UnknownPtr<CCL::IAccessibilityTableProvider> tableProvider = getEffectiveProvider ().asUnknown ())
	{
		if(tableProvider->countRows () > tableProvider->countColumns ())
			*pRetVal = RowOrColumnMajor_RowMajor;
		else if(tableProvider->countColumns () > tableProvider->countRows ())
			*pRetVal = RowOrColumnMajor_ColumnMajor;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ITableItemProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetRowHeaderItems (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;

	UnknownPtr<CCL::IAccessibilityTableProvider> tableProvider;	
	for(AccessibilityProvider* current = &getEffectiveProvider (); current != nullptr && !tableProvider.isValid (); current = current->getParentProvider ())
		tableProvider = current->asUnknown ();

	if(tableProvider.isValid ())
	{	
		AccessibilityProvider* provider = unknown_cast<AccessibilityProvider> (tableProvider->getRowHeaderItemProvider (&owner));
		if(provider)
		{
			::UIAutomationElementProvider* rawElementProvider = nullptr;
			if(sharePlatformProvider (&rawElementProvider, provider) == S_OK)
			{
				::IUnknown* result = static_cast<::IRawElementProviderSimple*> (rawElementProvider);
				SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (&result, 1);
				*pRetVal = safeArray.detach ();
				return S_OK;
			}
		}
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetColumnHeaderItems (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	UnknownPtr<CCL::IAccessibilityTableProvider> tableProvider;	
	for(AccessibilityProvider* current = &getEffectiveProvider (); current != nullptr && !tableProvider.isValid (); current = current->getParentProvider ())
		tableProvider = current->asUnknown ();

	if(tableProvider.isValid ())
	{	
		AccessibilityProvider* provider = unknown_cast<AccessibilityProvider> (tableProvider->getColumnHeaderItemProvider (&owner));
		if(provider)
		{
			::UIAutomationElementProvider* rawElementProvider = nullptr;
			if(sharePlatformProvider (&rawElementProvider, provider) == S_OK)
			{
				::IUnknown* result = static_cast<::IRawElementProviderSimple*> (rawElementProvider);
				SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (&result, 1);
				*pRetVal = safeArray.detach ();
				return S_OK;
			}
		}
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ISelectionProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::GetSelection (SAFEARRAY** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> containerProvider = getEffectiveProvider ().asUnknown ())
	{
		UnknownList selection;
		if(containerProvider->getSelectionProviders (selection) != kResultOk)
			return E_FAIL;

		Vector<::IUnknown*> result;
		ForEachUnknown (selection, unk)
			AccessibilityProvider* provider = unknown_cast<AccessibilityProvider> (unk);
			::IRawElementProviderSimple* rawElementProvider = nullptr;
			if(sharePlatformProvider (&rawElementProvider, provider) == S_OK)
				result.add (rawElementProvider);
		EndFor
			
		SafeArray<VT_UNKNOWN, ::IUnknown*> safeArray (result, result.count ());
		*pRetVal = safeArray.detach ();
		
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_CanSelectMultiple (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = FALSE;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> containerProvider = getEffectiveProvider ().asUnknown ())
	{
		if(containerProvider->canSelectMultiple ())
			*pRetVal = TRUE;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_IsSelectionRequired (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = FALSE;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> containerProvider = getEffectiveProvider ().asUnknown ())
	{
		if(containerProvider->isSelectionRequired ())
			*pRetVal = TRUE;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ISelectionItemProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Select ()
{
	ASSERT (System::IsInMainThread ())

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
	{
		if(!selectionProvider->isSelected ())
		{
			if(selectionProvider->select (true, IAccessibilitySelectionProvider::kExclusive) != kResultOk)
				return E_FAIL;
		}
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::AddToSelection ()
{
	ASSERT (System::IsInMainThread ())

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
	{
		if(selectionProvider->select (true) != kResultOk)
			return E_FAIL;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::RemoveFromSelection ()
{
	ASSERT (System::IsInMainThread ())

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
	{
		if(selectionProvider->select (false) != kResultOk)
			return E_FAIL;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_IsSelected (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = FALSE;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
	{
		if(selectionProvider->isSelected ())
			*pRetVal = TRUE;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_SelectionContainer (IRawElementProviderSimple** pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = nullptr;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilitySelectionProvider> selectionProvider = getEffectiveProvider ().asUnknown ())
	{
		if(AccessibilityProvider* containerProvider = unknown_cast<AccessibilityProvider> (selectionProvider->getSelectionContainerProvider ()))
			return sharePlatformProvider (pRetVal, containerProvider);
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IScrollProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Scroll (ScrollAmount horizontalAmount, ScrollAmount verticalAmount)
{
	ASSERT (System::IsInMainThread ())

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	auto toScrollDirection = [] (ScrollAmount amount, bool horizontal)
	{
		switch(amount)
		{
		case ScrollAmount_SmallIncrement : CCL_FALLTHROUGH
		case ScrollAmount_LargeIncrement :
			return horizontal ? AccessibilityScrollDirection::kRight : AccessibilityScrollDirection::kDown;
		case ScrollAmount_SmallDecrement : CCL_FALLTHROUGH
		case ScrollAmount_LargeDecrement :
			return horizontal ? AccessibilityScrollDirection::kLeft : AccessibilityScrollDirection::kUp;
		default : 
			return AccessibilityScrollDirection::kUndefined;
		}
	};
	
	auto toScrollAmount = [] (ScrollAmount amount)
	{
		switch(amount)
		{
		case ScrollAmount_SmallIncrement : CCL_FALLTHROUGH
		case ScrollAmount_SmallDecrement :
			return AccessibilityScrollAmount::kStep;
		case ScrollAmount_LargeIncrement : CCL_FALLTHROUGH
		case ScrollAmount_LargeDecrement :
			return AccessibilityScrollAmount::kPage;
		default : 
			return AccessibilityScrollAmount::kNone;
		}
	};
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		if(scrollProvider->scroll (toScrollDirection (horizontalAmount, true), toScrollAmount (horizontalAmount)) == kResultFailed)
			return E_FAIL;
	
		if(scrollProvider->scroll (toScrollDirection (verticalAmount, false), toScrollAmount (verticalAmount)) == kResultFailed)
			return E_FAIL;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::SetScrollPercent (double horizontalPercent, double verticalPercent)
{
	ASSERT (System::IsInMainThread ())
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		if(scrollProvider->scrollTo (horizontalPercent, verticalPercent) != kResultOk)
			return E_FAIL;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_HorizontalScrollPercent (double* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = 0.;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		*pRetVal = scrollProvider->getNormalizedScrollPositionX ();
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_VerticalScrollPercent (double *pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = 0.;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		*pRetVal = scrollProvider->getNormalizedScrollPositionY ();
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_HorizontalViewSize (double* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = 0.;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	Rect screenRect;
	owner.getElementBounds (screenRect, AccessibilityCoordSpace::kScreen);
	*pRetVal = screenRect.getWidth ();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_VerticalViewSize (double* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = 0.;

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	Rect screenRect;
	owner.getElementBounds (screenRect, AccessibilityCoordSpace::kScreen);
	*pRetVal = screenRect.getHeight ();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_HorizontallyScrollable (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = FALSE;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		if(scrollProvider->canScroll (AccessibilityScrollDirection::kRight))
			*pRetVal = TRUE;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_VerticallyScrollable (BOOL* pRetVal)
{
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = FALSE;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = getEffectiveProvider ().asUnknown ())
	{
		if(scrollProvider->canScroll (AccessibilityScrollDirection::kDown))
			*pRetVal = TRUE;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IScrollItemProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::ScrollIntoView ()
{	
	ASSERT (System::IsInMainThread ())

	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(getEffectiveProvider ().makeVisible () != kResultOk)
		return E_FAIL;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IExpandCollapseProvider
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Expand ()
{
	ASSERT (System::IsInMainThread ())
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityExpandCollapseProvider> expandCollapseProvider = getEffectiveProvider ().asUnknown ())
	{
		if(expandCollapseProvider->expand (true) != kResultOk)
			return E_FAIL;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::Collapse ()
{
	ASSERT (System::IsInMainThread ())
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityExpandCollapseProvider> expandCollapseProvider = getEffectiveProvider ().asUnknown ())
	{
		if(expandCollapseProvider->expand (false) != kResultOk)
			return E_FAIL;
		return S_OK;
	}

	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE UIAutomationElementProvider::get_ExpandCollapseState (ExpandCollapseState* pRetVal)
{	
	ASSERT (System::IsInMainThread ())
	if(!pRetVal)
		return E_INVALIDARG;
	*pRetVal = ExpandCollapseState_Collapsed;
	
	if(disconnected)
		return UIA_E_ELEMENTNOTAVAILABLE;
	
	if(UnknownPtr<IAccessibilityExpandCollapseProvider> expandCollapseProvider = getEffectiveProvider ().asUnknown ())
	{
		if(expandCollapseProvider->isExpanded ())
			*pRetVal = ExpandCollapseState_Expanded;
	}

	return S_OK;
}

//************************************************************************************************
// Win32::UIAutomationManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (AccessibilityManager, UIAutomationManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* UIAutomationManager::createPlatformProvider (AccessibilityProvider& provider)
{
	return NEW UIAutomationElementProvider (provider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutomationManager::shutdown ()
{
	HRESULT hr = ::UiaDisconnectAllProviders ();
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIAutomationManager::anyAccessibilityClientsListening () const
{
	if(::UiaClientsAreListening ())
		return true;

	// TODO: 
	//::SystemParametersInfo (SPI_GETSCREENREADER,...)???
	return false;
}
