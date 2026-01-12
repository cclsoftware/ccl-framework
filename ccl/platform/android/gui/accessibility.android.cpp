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
// Filename    : ccl/platform/android/gui/accessibility.android.cpp
// Description : Android Accessibility
//
//************************************************************************************************

#include "ccl/platform/android/gui/accessibility.android.h"
#include "ccl/platform/android/gui/window.android.h"
#include "ccl/platform/android/gui/frameworkview.h"

#include "ccl/platform/android/graphics/androidgraphics.h"

#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/gui/controls/editbox.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// JniAccessibilityEvent
//************************************************************************************************

DECLARE_JNI_CLASS (JniAccessibilityEvent, "android/view/accessibility/AccessibilityEvent")
	DECLARE_JNI_METHOD (int, getEventType)
END_DECLARE_JNI_CLASS (JniAccessibilityEvent)

DEFINE_JNI_CLASS (JniAccessibilityEvent)
	DEFINE_JNI_METHOD (getEventType, "()I")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// JniAccessibilityNodeInfo
//************************************************************************************************

DECLARE_JNI_CLASS (JniAccessibilityNodeInfo, "android/view/accessibility/AccessibilityNodeInfo")
	DECLARE_JNI_METHOD (void, addChild, jobject, int)
	DECLARE_JNI_METHOD (bool, removeChild, jobject, int)
	DECLARE_JNI_METHOD (void, setBoundsInScreen, jobject)
	DECLARE_JNI_METHOD (void, setCheckable, bool)
	DECLARE_JNI_METHOD (void, setChecked, bool)
	DECLARE_JNI_METHOD (void, setClassName, jstring)
	DECLARE_JNI_METHOD (void, setClickable, bool)
	DECLARE_JNI_METHOD (void, setCollectionInfo, jobject)
	DECLARE_JNI_METHOD (void, setCollectionItemInfo, jobject)
	DECLARE_JNI_METHOD (void, setContentDescription, jstring)
	DECLARE_JNI_METHOD (void, setEditable, bool)
	DECLARE_JNI_METHOD (void, setEnabled, bool)
	DECLARE_JNI_METHOD (void, setFocusable, bool)
	DECLARE_JNI_METHOD (void, setFocused, bool)
	DECLARE_JNI_METHOD (void, setHintText, jstring)
	DECLARE_JNI_METHOD (void, setImportantForAccessibility, bool)
	DECLARE_JNI_METHOD (void, setInputType, int)
	DECLARE_JNI_METHOD (void, setMaxTextLength, int)
	DECLARE_JNI_METHOD (void, setMultiLine, bool)
	DECLARE_JNI_METHOD (void, setParent, jobject, int)
	DECLARE_JNI_METHOD (void, setPassword, bool)
	DECLARE_JNI_METHOD (void, setRangeInfo, jobject)
	DECLARE_JNI_METHOD (void, setScrollable, bool)
	DECLARE_JNI_METHOD (void, setSelected, bool)
	DECLARE_JNI_METHOD (void, setText, jstring)
	DECLARE_JNI_METHOD (void, setVisibleToUser, bool)
END_DECLARE_JNI_CLASS (JniAccessibilityNodeInfo)

DEFINE_JNI_CLASS (JniAccessibilityNodeInfo)
	DEFINE_JNI_METHOD (addChild, "(Landroid/view/View;I)V")
	DEFINE_JNI_METHOD (removeChild, "(Landroid/view/View;I)Z")
	DEFINE_JNI_METHOD (setBoundsInScreen, "(Landroid/graphics/Rect;)V")
	DEFINE_JNI_METHOD (setCheckable, "(Z)V")
	DEFINE_JNI_METHOD (setChecked, "(Z)V")
	DEFINE_JNI_METHOD (setClassName, "(Ljava/lang/CharSequence;)V")
	DEFINE_JNI_METHOD (setClickable, "(Z)V")
	DEFINE_JNI_METHOD (setCollectionInfo, "(Landroid/view/accessibility/AccessibilityNodeInfo$CollectionInfo;)V")
	DEFINE_JNI_METHOD (setCollectionItemInfo, "(Landroid/view/accessibility/AccessibilityNodeInfo$CollectionItemInfo;)V")
	DEFINE_JNI_METHOD (setContentDescription, "(Ljava/lang/CharSequence;)V")
	DEFINE_JNI_METHOD (setEditable, "(Z)V")
	DEFINE_JNI_METHOD (setEnabled, "(Z)V")
	DEFINE_JNI_METHOD (setFocusable, "(Z)V")
	DEFINE_JNI_METHOD (setFocused, "(Z)V")
	DEFINE_JNI_METHOD (setHintText, "(Ljava/lang/CharSequence;)V")
	DEFINE_JNI_METHOD (setImportantForAccessibility, "(Z)V")
	DEFINE_JNI_METHOD (setInputType, "(I)V")
	DEFINE_JNI_METHOD (setMaxTextLength, "(I)V")
	DEFINE_JNI_METHOD (setMultiLine, "(Z)V")
	DEFINE_JNI_METHOD (setParent, "(Landroid/view/View;I)V")
	DEFINE_JNI_METHOD (setPassword, "(Z)V")
	DEFINE_JNI_METHOD (setRangeInfo, "(Landroid/view/accessibility/AccessibilityNodeInfo$RangeInfo;)V")
	DEFINE_JNI_METHOD (setScrollable, "(Z)V")
	DEFINE_JNI_METHOD (setSelected, "(Z)V")
	DEFINE_JNI_METHOD (setText, "(Ljava/lang/CharSequence;)V")
	DEFINE_JNI_METHOD (setVisibleToUser, "(Z)V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// JniAccessibilityCollectionInfo
//************************************************************************************************

DECLARE_JNI_CLASS (JniAccessibilityCollectionInfo, "android/view/accessibility/AccessibilityNodeInfo$CollectionInfo")
	enum SelectionMode
	{
		SELECTION_MODE_NONE = 0,
		SELECTION_MODE_SINGLE = 1,
		SELECTION_MODE_MULTIPLE = 2
	};

	DECLARE_JNI_STATIC_METHOD (jobject, obtain, int, int, bool, int)
END_DECLARE_JNI_CLASS (JniAccessibilityCollectionInfo)

DEFINE_JNI_CLASS (JniAccessibilityCollectionInfo)
	DEFINE_JNI_STATIC_METHOD (obtain, "(IIZI)Landroid/view/accessibility/AccessibilityNodeInfo$CollectionInfo;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// JniAccessibilityCollectionItemInfo
//************************************************************************************************

DECLARE_JNI_CLASS (JniAccessibilityCollectionItemInfo, "android/view/accessibility/AccessibilityNodeInfo$CollectionItemInfo")
	DECLARE_JNI_STATIC_METHOD (jobject, obtain, int, int, int, int, bool, bool)
END_DECLARE_JNI_CLASS (JniAccessibilityCollectionItemInfo)

DEFINE_JNI_CLASS (JniAccessibilityCollectionItemInfo)
	DEFINE_JNI_STATIC_METHOD (obtain, "(IIIIZZ)Landroid/view/accessibility/AccessibilityNodeInfo$CollectionItemInfo;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// JniAccessibilityRangeInfo
//************************************************************************************************

DECLARE_JNI_CLASS (JniAccessibilityRangeInfo, "android/view/accessibility/AccessibilityNodeInfo$RangeInfo")
	DECLARE_JNI_STATIC_METHOD (jobject, obtain, int, float, float, float)
END_DECLARE_JNI_CLASS (JniAccessibilityRangeInfo)

DEFINE_JNI_CLASS (JniAccessibilityRangeInfo)
	DEFINE_JNI_STATIC_METHOD (obtain, "(IFFF)Landroid/view/accessibility/AccessibilityNodeInfo$RangeInfo;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AccessibilityElementProvider
//************************************************************************************************

Vector<AccessibilityElementProvider::AccessibilityElementProviderPair> AccessibilityElementProvider::providers;
int AccessibilityElementProvider::nextProviderId = kProviderFirst;

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementProvider* AccessibilityElementProvider::toPlatformProvider (AccessibilityProvider* provider)
{
	return provider ? ccl_cast<AccessibilityElementProvider> (provider->getPlatformProvider ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AccessibilityElementProvider, PlatformAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementProvider::AccessibilityElementProvider (AccessibilityProvider& owner)
: PlatformAccessibilityProvider (owner),
  providerId (nextProviderId++),
  parent (nullptr)
{
	providers.add (AccessibilityElementProviderPair (providerId, this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementProvider::~AccessibilityElementProvider ()
{
	providers.removeIf ([this] (const AccessibilityElementProviderPair& provider) { return provider.key == providerId; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityElementProvider::disconnect ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityElementProvider::sendPlatformEvent (AccessibilityEvent e)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityElementProvider::onChildProviderAdded (AccessibilityProvider* childProvider)
{
	children.add (childProvider);

	AccessibilityElementProvider* childElementProvider = toPlatformProvider (childProvider);
	childElementProvider->parent = this;

	if(FrameworkView* view = getFrameworkView ())
		view->accessibilityContentChanged (providerId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityElementProvider::onChildProviderRemoved (AccessibilityProvider* childProvider)
{
	children.removeIf ([childProvider] (const SharedPtr<AccessibilityProvider>& child) { return child == childProvider; });

	if(FrameworkView* view = getFrameworkView ())
		view->accessibilityContentChanged (providerId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityElementProvider::fillAccessibilityNodeInfo (jobject view, int virtualViewId, jobject info) const
{
	ASSERT (virtualViewId == providerId)

	// add children
	JniAccessor jni;
	for(const SharedPtr<AccessibilityProvider>& provider : children)
	{
		AccessibilityElementProvider* platformProvider = toPlatformProvider (provider);

		JniAccessibilityNodeInfo.addChild (info, view, platformProvider->getProviderId ());
	}

	// set parent
	if(parent)
		JniAccessibilityNodeInfo.setParent (info, view, parent->getProviderId ());

	// collection info
	AccessibilityElementRole role = owner.getElementRole ();
	if(role == AccessibilityElementRole::kList)
	{
		JniAccessibilityNodeInfo.setClassName (info, JniString (jni, "android.widget.ListView"));

		LocalRef collectionInfo (jni, JniAccessibilityCollectionInfo.obtain (children.count (), 1, false, JniAccessibilityCollectionInfoClass::SELECTION_MODE_SINGLE));
		JniAccessibilityNodeInfo.setCollectionInfo (info, collectionInfo);
	}

	if(parent && parent->owner.getElementRole () == AccessibilityElementRole::kList)
	{
		LocalRef itemInfo (jni, JniAccessibilityCollectionItemInfo.obtain (parent->children.index (SharedPtr (const_cast<AccessibilityProvider*> (&owner))), 1, 0, 1, false, false));
		JniAccessibilityNodeInfo.setCollectionItemInfo (info, itemInfo);
	}

	// visibility/importance
	if(role == AccessibilityElementRole::kRoot || role == AccessibilityElementRole::kGroup || role == AccessibilityElementRole::kList)
	{
		JniAccessibilityNodeInfo.setVisibleToUser (info, false);
		JniAccessibilityNodeInfo.setImportantForAccessibility (info, false);
	}

	// element bounds
	Rect rect;
	owner.getElementBounds (rect, AccessibilityCoordSpace::kScreen);
	DpiScale::toPixelRect (rect, getContentScaleFactor ());

	LocalRef jRect (jni, jni.newObject (AndroidRect, AndroidRect.construct, rect.left, rect.top, rect.right, rect.bottom));
	JniAccessibilityNodeInfo.setBoundsInScreen (info, jRect);

	// label/description
	String label;
	getLabelProvider ().getElementName (label);
	if(!label.isEmpty ())
	{
		JniCCLString jLabel (label);
		JniAccessibilityNodeInfo.setText (info, jLabel);
		JniAccessibilityNodeInfo.setContentDescription (info, jLabel);
	}
	else
		JniAccessibilityNodeInfo.setContentDescription (info, nullptr);


	// labels
	if(role == AccessibilityElementRole::kLabel)
	{
		JniAccessibilityNodeInfo.setClassName (info, JniString (jni, "android.widget.TextView"));
		JniAccessibilityNodeInfo.setClickable (info, false);
	}

	// buttons
	if(role == AccessibilityElementRole::kButton)
	{
		JniAccessibilityNodeInfo.setClassName (info, JniString (jni, "android.widget.Button"));
		JniAccessibilityNodeInfo.setClickable (info, true);
	}

	// toggles
	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = getEffectiveProvider ().asUnknown ())
	{
		JniAccessibilityNodeInfo.setClassName (info, JniString (jni, "android.widget.ToggleButton"));
		JniAccessibilityNodeInfo.setCheckable (info, true);
		JniAccessibilityNodeInfo.setChecked (info, toggleProvider->isToggleOn ());
	}

	// edit boxes
	if(EditBox* editBox = ccl_cast<EditBox> (getEffectiveProvider ().getView ()))
	{
		JniAccessibilityNodeInfo.setClassName (info, JniString (jni, "android.widget.EditText"));
		JniAccessibilityNodeInfo.setMultiLine (info, false);
	}

	// values
	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = getValueProvider ().asUnknown ())
	{
		String value;
		valueProvider->getValue (value);

		String description = value;
		if(!label.isEmpty ())
			description = String (label).append (": ").append (value);

		if(!description.isEmpty ())
			JniAccessibilityNodeInfo.setContentDescription (info, JniCCLString (description));

		JniAccessibilityNodeInfo.setEditable (info, !valueProvider->isReadOnly ());
	}

	// element state
	int state = getEffectiveProvider ().getElementState ();
	JniAccessibilityNodeInfo.setEnabled (info, get_flag (state, AccessibilityElementState::kEnabled));
	JniAccessibilityNodeInfo.setFocusable (info, get_flag (state, AccessibilityElementState::kCanFocus));
	JniAccessibilityNodeInfo.setFocused (info, get_flag (state, AccessibilityElementState::kHasFocus));
	JniAccessibilityNodeInfo.setPassword (info, get_flag (state, AccessibilityElementState::kIsPassword));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AccessibilityElementProvider::getContentScaleFactor () const
{
	if(Window* window = getEffectiveProvider ().getView ()->getWindow ())
		return window->getContentScaleFactor ();

	return 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkView* AccessibilityElementProvider::getFrameworkView () const
{
	if(AndroidWindow* window = AndroidWindow::cast (getEffectiveProvider ().getView ()->getWindow ()))
		return window->getFrameworkView ();

	return nullptr;
}

//************************************************************************************************
// AccessibilityRootElementProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AccessibilityRootElementProvider, AccessibilityElementProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityRootElementProvider::AccessibilityRootElementProvider (AccessibilityProvider& owner)
: AccessibilityElementProvider (owner)
{
	providerId = kProviderRoot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityRootElementProvider::fillAccessibilityNodeInfo (jobject view, int virtualViewId, jobject info) const
{
	// fill root node information
	if(virtualViewId == kProviderRoot)
	{
		SuperClass::fillAccessibilityNodeInfo (view, virtualViewId, info);
		return;
	}

	// relay to virtual view provider
	for(AccessibilityElementProviderPair& entry : providers)
	{
		if(entry.key != virtualViewId)
			continue;

		entry.value->fillAccessibilityNodeInfo (view, virtualViewId, info);
		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AccessibilityRootElementProvider::getVirtualViewAt (PointRef pos) const
{
	Point coordPos = pos;
	DpiScale::toCoordPoint (coordPos, getContentScaleFactor ());

	AccessibilityProvider* provider = owner.findElementProviderAt (coordPos, AccessibilityCoordSpace::kScreen);
	if(!provider)
		return kProviderRoot;

	AccessibilityElementProvider* elementProvider = toPlatformProvider (provider);
	return elementProvider->getProviderId ();
}

//************************************************************************************************
// AndroidAccessibilityManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (AccessibilityManager, AndroidAccessibilityManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* AndroidAccessibilityManager::createPlatformProvider (AccessibilityProvider& provider)
{
	if(provider.getElementRole () == AccessibilityElementRole::kRoot)
		return NEW AccessibilityRootElementProvider (provider);

	return NEW AccessibilityElementProvider (provider);
}
