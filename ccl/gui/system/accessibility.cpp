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
// Filename    : ccl/gui/system/accessibility.cpp
// Description : Accessibility Classes
//
//************************************************************************************************

#include "ccl/gui/system/accessibility.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/gui/views/view.h"

#include "ccl/public/guiservices.h"
#include "ccl/public/text/translation.h"

#define PLATFORM_ACCESSIBILITYMANAGER_AVAILABLE (CCL_PLATFORM_WINDOWS || CCL_PLATFORM_IOS || CCL_PLATFORM_MAC || CCL_PLATFORM_ANDROID)

namespace CCL {

//************************************************************************************************
// NullAccessibilityManager
//************************************************************************************************	

class NullAccessibilityManager: public AccessibilityManager
{
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Accessibility")
	XSTRING (Pagination, "Page %(1) of %(2)")
	XSTRING (On, "On")
	XSTRING (Off, "Off")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAccessibilityManager& CCL_API System::CCL_ISOLATED (GetAccessibilityManager) ()
{
	return AccessibilityManager::instance ();
}

//************************************************************************************************
// NullAccessibilityManager
//************************************************************************************************	

#if !PLATFORM_ACCESSIBILITYMANAGER_AVAILABLE
DEFINE_EXTERNAL_SINGLETON (AccessibilityManager, NullAccessibilityManager)
#endif

//************************************************************************************************
// PlatformAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlatformAccessibilityProvider, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider::PlatformAccessibilityProvider (AccessibilityProvider& owner)
: owner (owner) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlatformAccessibilityProvider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == AccessibilityProvider::kChildProviderAdded)
	{
		auto* p = unknown_cast<AccessibilityProvider> (msg[0].asUnknown ());
		ASSERT (p)
		onChildProviderAdded (p);
	}
	else if(msg == AccessibilityProvider::kChildProviderRemoved)
	{
		auto* p = unknown_cast<AccessibilityProvider> (msg[0].asUnknown ());
		ASSERT (p)
		onChildProviderRemoved (p);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& PlatformAccessibilityProvider::getEffectiveProvider () const
{
	return AccessibilityManager::instance ().getEffectiveProvider (owner);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& PlatformAccessibilityProvider::getLabelProvider () const
{
	return AccessibilityManager::instance ().getLabelProvider (owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& PlatformAccessibilityProvider::getValueProvider () const
{
	return AccessibilityManager::instance ().getValueProvider (owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformAccessibilityProvider::onChildProviderAdded (AccessibilityProvider* childProvider) 
{
	#if DEBUG_LOG
	String elementName;
	childProvider->getElementName (elementName);
	CCL_PRINTF ("[Accessibility] Child provider added (type = %d name = %s)\n", 
				childProvider->getElementRole (),
				MutableCString (elementName).str ())
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformAccessibilityProvider::onChildProviderRemoved (AccessibilityProvider* childProvider) 
{
	#if DEBUG_LOG
	String elementName;
	childProvider->getElementName (elementName);
	CCL_PRINTF ("[Accessibility] Child provider removed (type = %d name = %s)\n", 
				childProvider->getElementRole (),
				MutableCString (elementName).str ())
	#endif
}

//************************************************************************************************
// AccessibilityProvider
//************************************************************************************************

void AccessibilityProvider::getPaginationText (String& text, int currentPage, int totalPages)
{
	text.empty ();
	text.appendFormat (XSTR (Pagination), currentPage, totalPages);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityProvider::getToggleText (String& text, bool isOn)
{
	text.empty ();
	text.append (isOn ? XSTR (On) : XSTR (Off));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AccessibilityProvider, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider::AccessibilityProvider ()
: parentProvider (nullptr)
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* AccessibilityProvider::getPlatformProvider ()
{
	if(!platformProvider)
		platformProvider = AccessibilityManager::instance ().createPlatformProvider (*this);
	return platformProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityProvider::sendEvent (AccessibilityEvent e)
{
	if(PlatformAccessibilityProvider* p = getPlatformProvider ())
		p->sendPlatformEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityProvider::addChildProvider (AccessibilityProvider* childProvider)
{
	ASSERT (childProvider && !childProvider->getParentProvider ())
	if(!childProvider || childProvider->getParentProvider ())
		return;

	children.add (childProvider);
	childProvider->setParentProvider (this);
	childProvider->retain ();

	signal (Message (kChildProviderAdded, childProvider->asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityProvider::removeChildProvider (AccessibilityProvider* childProvider)
{
	ASSERT (childProvider && childProvider->getParentProvider () == this)
	if(!childProvider || childProvider->getParentProvider () != this)
		return;

	childProvider->disconnect ();
	childProvider->setParentProvider (nullptr);
	children.remove (childProvider);

	signal (Message (kChildProviderRemoved, childProvider->asUnknown ()));
	childProvider->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityProvider::disconnect ()
{
	for(AccessibilityProvider* child : iterate_as<AccessibilityProvider> (children))
	{
		child->disconnect ();
		child->setParentProvider (nullptr);
	}

	if(platformProvider)
	{
		CCL_PRINTLN ("[Accessibility] Disconnect platform provider")
		platformProvider->disconnect ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AccessibilityProvider::signal (MessageRef msg)
{
	SuperClass::signal (msg);
	
	if(PlatformAccessibilityProvider* p = getPlatformProvider ())
		p->notify (this, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API AccessibilityProvider::getIView () const
{
	return getView ();
}

//************************************************************************************************
// AccessibilityManager
//************************************************************************************************

bool AccessibilityManager::isEnabled ()
{
	// work in progress...
	#if (PLATFORM_ACCESSIBILITYMANAGER_AVAILABLE && (DEBUG || CCL_PLATFORM_IOS || CCL_PLATFORM_ANDROID))
	static Configuration::BoolValue enabled ("CCL.Accessibility", "Enabled", false);
	return enabled;
	#else
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AccessibilityManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityManager::shutdown ()
{
	views.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AccessibilityManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		views.removeIf ([&] (const ViewEntry& entry) { return entry.subject == subject; });
		subject->removeObserver (this);
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* AccessibilityManager::createPlatformProvider (AccessibilityProvider& provider)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AccessibilityManager::anyAccessibilityClientsListening () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityManager::registerAccessibleView (View* view, StringID accessibilityId)
{
	ViewEntry* entry = views.findIf ([&] (const ViewEntry& entry) { return entry.view == view; });
	if(entry == nullptr)
	{
		views.add ({view, accessibilityId});
		view->addObserver (this);
	}
	else
		entry->accessibilityId = accessibilityId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AccessibilityManager::setViewRelation (View* view, AccessibilityRelation relation, StringID childId)
{
	ViewEntry* entry = views.findIf ([&] (const ViewEntry& entry) { return entry.view == view; });
	ASSERT (entry != nullptr)
	if(entry)
	{
		RelationEntry* relationEntry = entry->relations.findIf ([&] (const RelationEntry& entry) { return entry.relation == relation; });
		if(relationEntry)
			relationEntry->childId = childId;
		else
			entry->relations.add ({relation, childId});

	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* AccessibilityManager::findRelatedProvider (const IAccessibilityProvider* provider, AccessibilityRelation relation) const
{
	View* view = unknown_cast<View> (provider->getIView ());
	if(view == nullptr)
		return nullptr;

	ViewEntry* entry = views.findIf ([&] (const ViewEntry& entry) { return entry.view == view; });
	if(entry)
	{
		RelationEntry* relationEntry = entry->relations.findIf ([&] (const RelationEntry& entry) { return entry.relation == relation; });
		if(relationEntry && !relationEntry->childId.isEmpty ())
		{
			AutoPtr<IRecognizer> recognizer = Recognizer::create ([&] (IUnknown* obj)
			{
				View* childView = unknown_cast<View> (obj);
				return (childView && getAccessibilityId (childView) == relationEntry->childId);
			});
			View* relatedView = view->findView (*recognizer);
			if(relatedView)
				return relatedView->getAccessibilityProvider ();
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& AccessibilityManager::getEffectiveProvider (AccessibilityProvider& provider) const
{
	if(AccessibilityProvider* proxy = findRelatedProvider (&provider, AccessibilityRelation::kProxy))
		return *proxy;
	return provider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& AccessibilityManager::getValueProvider (AccessibilityProvider& provider) const
{
	if(AccessibilityProvider* valueProvider = findRelatedProvider (&provider, AccessibilityRelation::kValue))
		return *valueProvider;
	return provider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& AccessibilityManager::getLabelProvider (AccessibilityProvider& provider) const
{
	if(AccessibilityProvider* labelProvider = findRelatedProvider (&provider, AccessibilityRelation::kLabel))
		return *labelProvider;
	return provider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString AccessibilityManager::getAccessibilityId (View* view) const
{
	ViewEntry* entry = views.findIf ([&] (const ViewEntry& entry) { return entry.view == view; });
	if(entry && !entry->accessibilityId.isEmpty ())
		return entry->accessibilityId;
	return MutableCString (view->getName ());
}

//************************************************************************************************
// AccessibilityManager::ViewEntry
//************************************************************************************************

AccessibilityManager::ViewEntry::ViewEntry (View* view, StringID accessibilityId)
: view (view),
  subject (UnknownPtr<ISubject> (ccl_as_unknown (view))),
  accessibilityId (accessibilityId)
{}
