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
// Filename    : ccl/extras/packages/unifiedpackageaction.cpp
// Description : Unified Package Action
//
//************************************************************************************************

#include "ccl/extras/packages/unifiedpackageaction.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageActions")
	// action titles
	XSTRING (Enable, "Enable")
	XSTRING (Disable, "Disable")
	XSTRING (Install, "Install")
	XSTRING (Uninstall, "Uninstall")
	XSTRING (Update, "Update")
	XSTRING (Restart, "Restart")

	// macro titles
	XSTRING (InstallAll, "Install All")
	XSTRING (UninstallAll, "Uninstall All")
	XSTRING (UpdateAll, "Update All")
	XSTRING (EnableAll, "Enable All")
	XSTRING (DisableAll, "Disable All")

	// state labels
	XSTRING (Installed, "Installed")
	XSTRING (NotInstalled, "Not Installed")
	XSTRING (Enabled, "Enabled")
	XSTRING (Disabled, "Disabled")
	XSTRING (RestartRequired, "Restart Required")

	// composed titles
	XSTRING (EnableItems, "Enable %(1) items")
	XSTRING (DisableItems, "Disable %(1) items")
	XSTRING (InstallItems, "Install %(1) items")
	XSTRING (InstallItemsWithSize, "Install %(1) items (%(2))")
	XSTRING (UninstallItems, "Uninstall %(1) items")
	XSTRING (UpdateItems, "Update %(1) items")
END_XSTRINGS

//************************************************************************************************
// UnifiedPackageAction
//************************************************************************************************

DEFINE_CLASS (UnifiedPackageAction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageAction::UnifiedPackageAction (IUnifiedPackageHandler* handler, UnifiedPackage* package, CString id, int state)
: handler (handler),
  package (nullptr),
  id (id),
  state (state),
  flags (0)
{
	setPackage (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID UnifiedPackageAction::getGroupId () const
{
	return handler->getActionGroupId (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageAction::getTitle () const
{
	return handler->getActionTitle (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* UnifiedPackageAction::getIcon () const
{
	return handler->getActionIcon (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageAction::getMacroTitle () const
{
	return handler->getMacroTitle (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageAction::getStateLabel () const
{
	return handler->getStateLabel (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageAction::getGroupStateLabel () const
{
	return handler->getGroupStateLabel (getGroupId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::composeTitle (String& title, int itemCount, StringRef details) const
{
	return handler->composeTitle (title, id, itemCount, details);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageAction::perform ()
{
	ASSERT (state == kEnabled)
	retain (); // keep alive until complete is called
	setState (kActive);
	if(handler->performAction (*this) == false)
	{
		release ();
		return false;
	}
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageAction::cancel ()
{
	return handler->cancelAction (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageAction::pause (bool state)
{
	return handler->pauseAction (*this, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::complete (bool success)
{
	ASSERT (state == kActive || state == kPaused)
	handler->updateAction (*this);
	if(observer)
	{
		observer->onCompletion (*this, success);
		setObserver (nullptr);
	}
	release (); // match retain in perform
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::progress (double progress)
{
	if(observer)
		observer->onProgress (*this, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::packageChanged ()
{
	if(observer)
		observer->onPackageChanged (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::onPause (bool state)
{
	setState (state ? kPaused : kActive);
	if(observer)
		observer->onPause (*this, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnifiedPackageAction::reportEvent (const Alert::Event& e)
{
	if(observer)
		observer->reportEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageAction::requestRestart (StringRef message)
{
	if(observer)
		observer->requestRestart (*this, message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnifiedPackageAction::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageAction::operator== (const UnifiedPackageAction& other) const
{
	return other.handler == handler && other.id == id;
}

//************************************************************************************************
// IUnifiedPackageHandlerObserver
//************************************************************************************************

DEFINE_IID_ (IUnifiedPackageHandlerObserver, 0x535841ae, 0x30d, 0x4583, 0x98, 0x3, 0x1f, 0x45, 0xcd, 0x1f, 0xe8, 0x79)

//************************************************************************************************
// IUnifiedPackageHandler
//************************************************************************************************

DEFINE_IID_ (IUnifiedPackageHandler, 0x8e08d2d6, 0xe5a5, 0x44b6, 0xbe, 0x77, 0x7d, 0xfa, 0x5d, 0xc2, 0x5, 0x80)

//************************************************************************************************
// UnifiedPackageHandler
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kEnable, "enable")
DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kDisable, "disable")
DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kInstall, "install")
DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kUninstall, "uninstall")
DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kUpdate, "update")
DEFINE_STRINGID_MEMBER_ (UnifiedPackageHandler, kRestart, "restart")

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageHandler::getActionTitle (StringID id) const
{
	if(id == kInstall) return XSTR (Install);
	if(id == kUninstall) return XSTR (Uninstall);
	if(id == kUpdate) return XSTR (Update);
	if(id == kEnable) return XSTR (Enable);
	if(id == kDisable) return XSTR (Disable);
	if(id == kRestart) return XSTR (Restart);
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageHandler::getMacroTitle (StringID id) const
{
	if(id == kInstall) return XSTR (InstallAll);
	if(id == kUninstall) return XSTR (UninstallAll);
	if(id == kUpdate) return XSTR (UpdateAll);
	if(id == kEnable) return XSTR (EnableAll);
	if(id == kDisable) return XSTR (DisableAll);
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageHandler::getStateLabel (StringID id) const
{
	if(id == kUninstall) return XSTR (Installed);
	if(id == kInstall) return XSTR (NotInstalled);
	if(id == kDisable) return XSTR (Enabled);
	if(id == kEnable) return XSTR (Disabled);
	if(id == kRestart) return XSTR (RestartRequired);
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UnifiedPackageHandler::getGroupStateLabel (StringID id) const
{
	if(id == kInstall) return XSTR (Installed);
	if(id == kEnable) return XSTR (Enabled);
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID UnifiedPackageHandler::getActionGroupId (StringID id) const
{
	if(id == kInstall) return kInstall;
	if(id == kUninstall) return kInstall;
	if(id == kUpdate) return kUpdate;
	if(id == kEnable) return kEnable;
	if(id == kDisable) return kEnable;
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageHandler::composeTitle (String& title, StringID id, int itemCount, StringRef details) const
{
	if(details.isEmpty () == false && id == kInstall)
		title = String ().appendFormat (XSTR (InstallItemsWithSize), itemCount, details);
	else if(id == kInstall)
		title = String ().appendFormat (XSTR (InstallItems), itemCount);
	else if(id == kUninstall)
		title = String ().appendFormat (XSTR (UninstallItems), itemCount);
	else if(id == kUpdate)
		title = String ().appendFormat (XSTR (UpdateItems), itemCount);
	else if(id == kEnable)
		title = String ().appendFormat (XSTR (EnableItems), itemCount);
	else if(id == kDisable)
		title = String ().appendFormat (XSTR (DisableItems), itemCount);
	else
		title = getActionTitle (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageAction* UnifiedPackageHandler::createAction (UnifiedPackage* package, StringID actionId)
{
	UnifiedPackageAction* action = NEW UnifiedPackageAction (this, package, actionId, UnifiedPackageAction::kInvalid);
	updateAction (*action);
	return action;
}
