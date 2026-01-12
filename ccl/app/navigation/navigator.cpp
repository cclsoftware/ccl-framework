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
// Filename    : ccl/app/navigation/navigator.cpp
// Description : Navigator Component
//
//************************************************************************************************

#include "ccl/app/navigation/navigator.h"
#include "ccl/app/navigation/navigationservice.h"
#include "ccl/public/app/inavigationserver.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/system/cclanalytics.h"

#include "ccl/public/gui/appanalytics.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum NavigatorTags
	{
		kLocation = 100,
		kCommandLinks,
		kCommandLinkIndividual
	};
}

//************************************************************************************************
// Navigator::CommandLink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Navigator::CommandLink, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::CommandLink::CommandLink (StringRef name)
: name (name),
  parameter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::CommandLink::CommandLink (StringRef name, StringRef title, UrlRef url)
: name (name), 
  title (title), 
  url (url),
  parameter (nullptr)
{
	if(title.isEmpty ())
		setTitle (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Navigator::CommandLink::queryInterface (UIDRef iid, void** ptr)
{
	if(icon && iid == ccl_iid<IImage> ())
		return icon->queryInterface (iid, ptr);
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::CommandLink::equals (const Object& obj) const
{
	return ((CommandLink*)&obj)->name == name; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::CommandLink::toString (String& string, int flags) const
{
	string = title;
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::CommandLink::isVisible () const
{
	return !visibilityParam || visibilityParam->getValue ().asBool ();
}

//************************************************************************************************
// Navigator
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Navigator, NavigatorBase2)
DEFINE_COMPONENT_SINGLETON (Navigator)

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::Navigator (StringRef name, StringRef title)
: NavigatorBase2 (name.isEmpty () ? CCLSTR ("Navigator") : name, title),
  autoShow (false),
  autoHide (false),
  autoHome (false),
  trackingEnabled (false),
  restoreUrlSuspended (false),
  dispatchCommandsToContentComponent (true),
  defaultContentComponent (nullptr)
{
	commandLinks.objectCleanup ();

	paramList.addString (CSTR ("location"), Tag::kLocation);
	paramList.addList (CSTR ("commandLinks"), Tag::kCommandLinks);

	updateNavigation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::~Navigator ()
{
	cancelSignals ();

	for(auto link : iterate_as<CommandLink> (commandLinks))
		if(IParameter* param = link->getVisibilityParam ())
			ISubject::removeObserver (param, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API Navigator::findChild (StringRef id) const
{
	if(id == CCLSTR ("defaultComponent"))
		return UnknownPtr<IObjectNode> (defaultContentComponent);
	else
		return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Navigator::saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const IViewState*) const
{
	AttributeAccessor a (attributes);
	String urlString;
	currentUrl.getUrl (urlString);
	a.set (CSTR ("url"), urlString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Navigator::loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, IViewState*)
{
	if(restoreUrlSuspended)
		return true;

	String urlString (AttributeReadAccessor (attributes).getString (CSTR ("url")));
	return navigate (Url (urlString)) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::CommandLink* Navigator::addCommandLink (StringRef name, StringRef title, UrlRef url, IImage* icon, int index)
{
	CommandLink* link = NEW CommandLink (name, title, url);
	link->setIcon (icon);
	commandLinks.insertAt (index, link);

	UnknownPtr<IListParameter> linksParam (paramList.byTag (Tag::kCommandLinks));
	linksParam->appendValue (Variant (link->asUnknown ()), index);

	MutableCString paramName (name);
	IParameter* individualParam = paramList.addParam (paramName, Tag::kCommandLinkIndividual);
	link->setParameter (individualParam);

	return link;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Navigator::CommandLink* Navigator::findCommandLink (StringRef name) const
{
	return commandLinks.findIf<CommandLink> ([&] (const CommandLink& link)
	{
		return link.getName () == name;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Navigator::setVisibilityParam (StringRef linkName, IParameter* param)
{
	CommandLink* link = findCommandLink (linkName);

	if(link && link->getVisibilityParam () != param)
	{
		if(link->getVisibilityParam ())
			ISubject::removeObserver (link->getVisibilityParam (), this);

		bool wasVisible = link->isVisible ();
		link->setVisibilityParam (param);

		if(wasVisible != link->isVisible ())
			(NEW Message ("updateCommandLinks"))->post (this, -1);

		if(param)
			ISubject::addObserver (param, this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::canOpenWindow () const
{
	if(isOpen())
		return true;

	if(!isAutoShow ())
		return false;

	return System::GetWindowManager ().canOpenWindow (MutableCString (getName ())) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::openWindow ()
{
	if(!isOpen())
	{
		ASSERT (isAutoShow () == true)
		if(!isAutoShow ())
			return false;
	}

	if(System::GetWindowManager ().findWindowClass (MutableCString (getName ())))
		return System::GetWindowManager ().openWindow (MutableCString (getName ())) != 0; // (activate when already open)

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::closeWindow ()
{
	if(isOpen())
		return System::GetWindowManager ().closeWindow (MutableCString (getName ())) != 0;
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Navigator::navigate (UrlRef url)
{
	if(!isOpen ())
	{
		if(url.isEmpty () == false)
			SuperClass::navigate (url);
		return kResultFalse;
	}

	// check if it's an external link...
	if(!NavigationService::instance ().isValidProtocol (url.getProtocol ()))
		return System::GetSystemShell ().openUrl (url);
		
	tresult result = kResultOk;
	if(currentUrl != url || contentFrame->getChildren ().isEmpty ()) 
	{
		result = kResultFalse;

		// lookup server
		INavigationServer* server = NavigationService::instance ().lookupServer (url);
		if(server)
		{	
			NavigateArgs args (*this, url, *contentFrame, defaultContentComponent);
			result = server->navigateTo (args);

			if(result != kResultOk)
			{
				if(!args.errorDocumentName.isEmpty ())
				{
					contentFrame->getChildren ().removeAll ();
					ViewBox (contentFrame).setTitle (String::kEmpty);
				
					MutableCString formName ("CCL/ErrorDocument:");
					formName.append (args.errorDocumentName);

					ITheme* theme = getTheme ();
					IView* errorView = theme ? theme->createView (formName, this->asUnknown ()) : nullptr;
					ASSERT (errorView != nullptr)
					if(errorView)
					{
						contentFrame->getChildren ().add (errorView);
						ViewBox (contentFrame).setTitle (ViewBox (errorView).getTitle ());
					}

					result = kResultOk;
				}
			}

			if(result == kResultOk)
			{
				SuperClass::navigate (url);
				currentTitle = ViewBox (contentFrame).getTitle (); // <--- set new title

				if(isTrackingEnabled ())
				{
					Attributes analyticsData;
					analyticsData.set (AnalyticsID::kNavigationPath, url.getPath ());
					ccl_analytics_event (AnalyticsID::kNavigation, &analyticsData);
				}
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Navigator::navigateDeferred (UrlRef _url)
{
	AutoPtr<Url> url = NEW Url (_url);
	Message* m = NEW Message ("navigate", url->asUnknown ());
	m->post (this);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Navigator::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "navigate")
	{
		UnknownPtr<IUrl> url = msg.getArg (0).asUnknown ();
		if(url)
			navigate (*url);
	}
	else if(msg == kChanged)
	{
		// visibility param changed from outside
		UnknownPtr<IParameter> param (subject);
		if(param)
			if(auto link = commandLinks.findIf<CommandLink> ([&] (const CommandLink& link) { return link.getVisibilityParam () == param; }))
				updateCommandLinks ();
	}
	else if(msg == "updateCommandLinks")
		updateCommandLinks ();
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Navigator::refresh ()
{
	if(contentFrame.isValid ())
	{
		contentFrame->getChildren ().removeAll ();
		navigate (currentUrl);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Navigator::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kLocation :
		{
			String str;
			param->toString (str);
			Url url (str);
			navigate (url);
		}
		break;

	case Tag::kCommandLinks :
		{
			UnknownPtr<IListParameter> linksParam (param);
			CommandLink* link = (CommandLink*)unknown_cast<Object> (linksParam->getSelectedValue ().asUnknown ());
			if(link)
				navigate (link->getUrl ());
		}
		break;

	case Tag::kCommandLinkIndividual :
		{
			CommandLink* link = (CommandLink*)commandLinks.findEqual (CommandLink (String (param->getName ())));
			if(link && param->getValue ())
				navigate (link->getUrl ());
			else
				goHome ();
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Navigator::onNavigated ()
{
	SuperClass::onNavigated ();

	updateNavigation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Navigator::updateNavigation ()
{
	paramList.checkCommandStates ();

	String location;
	currentUrl.getUrl (location, true);
	paramList.byTag (Tag::kLocation)->setValue (Variant (location));

	CommandLink* linkToSelect = nullptr;

	ForEach (commandLinks, CommandLink, link)
		bool isCurrent = link->getUrl () == currentUrl;
		
		IParameter* individualParam = link->getParameter ();
		if(individualParam)
			individualParam->setValue (isCurrent);
		
		if(isCurrent)
			linkToSelect = link;

	EndFor

	if(linkToSelect)
	{
		if(!linkToSelect->isVisible () && linkToSelect->getVisibilityParam ())
		{
			linkToSelect->getVisibilityParam ()->setValue (true, true);
			updateCommandLinks ();
		}

		UnknownPtr<IListParameter> linksParam (paramList.byTag (Tag::kCommandLinks));
		linksParam->selectValue (Variant (linkToSelect->asUnknown ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Navigator::updateCommandLinks ()
{
	UnknownPtr<IListParameter> linksParam (paramList.byTag (Tag::kCommandLinks));
	auto selectedLink = unknown_cast<CommandLink> (linksParam->getSelectedValue ().asUnknown ());
	bool removedSelected = false;

	linksParam->removeAll ();

	for(auto link : iterate_as<CommandLink> (commandLinks))
	{
		if(link->isVisible ())
			linksParam->appendValue (Variant (link->asUnknown ()));
		else if(link == selectedLink)
			removedSelected = true; // selected was hidden
	}

	if(removedSelected)
		goHome ();
	else if(selectedLink)
		linksParam->selectValue (selectedLink->asUnknown (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Navigator::getContentComponent () const
{
	if(contentFrame.isValid () && !contentFrame->getChildren ().isEmpty ())
	{
		ForEachChildView (contentFrame, childView)
			if(auto* controller = childView.getController ())
				return controller;
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Navigator::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "contentFrame")
	{
		FormBox view (bounds);
		view.setController (this->asUnknown ()); // controller needs to be set
		contentFrame = view;
		
		if(currentUrl.isEmpty ())
		{
			ASSERT (homeUrl.isEmpty () == false)
			navigate (homeUrl);
		}
		else
			navigate (currentUrl);

		return contentFrame;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Navigator::appendContextMenu (IContextMenu& contextMenu)
{
	// try current content controller...
	UnknownPtr<IContextMenuHandler> contentHandler (getContentComponent ());
	if(contentHandler && contentHandler->appendContextMenu (contextMenu) == kResultOk)
		return kResultOk;

	return SuperClass::appendContextMenu (contextMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Navigator::checkCommandCategory (CStringRef category) const
{
	if(SuperClass::checkCommandCategory (category))
		return true;

	if(String (category) == getName ()) // todo: can this be removed?
		return true;

	// ask current content controller
	UnknownPtr<ICommandHandler> contentHandler (getContentComponent ());
	if(contentHandler && contentHandler->checkCommandCategory (category))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Navigator::interpretCommand (const CommandMsg& msg)
{
	if(!isOpen () && msg.checkOnly () && !canOpenWindow ())
	{
		// reset checkmarks of command links
		UnknownPtr<IMenuItem> menuItem (msg.invoker);
		if(menuItem)
		{
			CommandLink* cLink = (CommandLink*)commandLinks.findEqual (CommandLink (String (msg.name)));
			if(cLink)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, false);
		}

		return false;
	}

	bool ownCommand = false; // keep own commands (e.g. nested navigators)
	if(UnknownPtr<IParameter> commandParam = msg.invoker)
		if(isEqualUnknown (commandParam->getController (), asUnknown ()))
			ownCommand = true;
	
	if(dispatchCommandsToContentComponent && !ownCommand)
	{
		// try current content controller first
		UnknownPtr<ICommandHandler> contentHandler (getContentComponent ());
		if(contentHandler && contentHandler->interpretCommand (msg))
			return true;
	}

	if(SuperClass::dispatchCommand (msg))
		return true;

	// try command links...
	if(!commandLinks.isEmpty ())
	{
		CommandLink* cLink = (CommandLink*)commandLinks.findEqual (CommandLink (String (msg.name)));
		if(cLink)
		{
			bool isCurrentlyShown = isOpen () && cLink->getUrl () == currentUrl;

			if(msg.checkOnly ())
			{
				UnknownPtr<IMenuItem> menuItem (msg.invoker);
				if(menuItem)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, isCurrentlyShown);
			}
			else
			{
				ScopedVar<bool> guard (restoreUrlSuspended, true); // don't restore last url while opening window

				// state == true: open; state == false: close
				bool state = !(isAutoHide () && isCurrentlyShown); // autohide: close if same is url already shown
				CommandAutomator::Arguments (msg).getBool ("State", state);
				
				if(!state)
				{
					closeWindow ();
					return true;
				}
				if(isAutoHome () && isCurrentlyShown)
					goHome ();
				else
				{
					// 1.) navigate to new url before opening (don't use last url again)
					navigate (cLink->getUrl ());

					// 2.) ensure window is open
					openWindow ();
				}
			}
			return true;
		}
	}

	return SuperClass::interpretCommand (msg); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::onHomeCmd (CmdArgs args)
{
	bool result = SuperClass::onHomeCmd (args);
	if(!args.checkOnly ())
		openWindow (); // ensure window is open
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Navigator::onRefreshCmd (CmdArgs args)
{
	if(args.checkOnly ())
		return contentFrame.isValid () && !contentFrame->getChildren ().isEmpty ();
	else
		return SuperClass::onRefreshCmd (args);
}
