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
// Filename    : ccl/extras/packages/unifiedpackageaction.h
// Description : Unified Package Action
//
//************************************************************************************************

#ifndef _ccl_unifiedpackageaction_h
#define _ccl_unifiedpackageaction_h

#include "ccl/extras/packages/unifiedpackage.h"

#include "ccl/base/collections/container.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/system/alerttypes.h"

namespace CCL {
class Component;

namespace Packages {
interface IUnifiedPackageHandler;
interface IUnifiedPackageHandlerObserver;

//************************************************************************************************
// UnifiedPackageAction
/** Represents an action which can be performed on UnifiedPackage instances. @sa IUnifiedPackageHandler */
//************************************************************************************************

class UnifiedPackageAction: public Object,
							Alert::IReporter
{
public:
	DECLARE_CLASS (UnifiedPackageAction, Object)

	UnifiedPackageAction (IUnifiedPackageHandler* handler = nullptr, UnifiedPackage* package = nullptr, CString id = nullptr, int state = kInvalid);

	enum State
	{
		kInvalid = -1,

		kDisabled = 0,
		kEnabled,
		kActive,
		kPaused,

		kNumValidStates
	};

	enum Flags
	{
		kCancelEnabled = 1<<0,		//< this action can be canceled
		kNeedsConfirmation = 1<<1,	//< this action needs to be confirmed by the user
		kHidden = 1<<2,				//< this action should not be displayed directy, it should be used with a macro instead
		kRequired = 1<<3,			//< this action needs to be taken before the user can use the associated package
		kResumable = 1<<4			//< this action can be paused and resumed
	};

	PROPERTY_FLAG (flags, kCancelEnabled, isCancelEnabled)
	PROPERTY_FLAG (flags, kNeedsConfirmation, needsConfirmation)
	PROPERTY_FLAG (flags, kHidden, isHidden)
	PROPERTY_FLAG (flags, kRequired, isRequired)
	PROPERTY_FLAG (flags, kResumable, isResumable)

	PROPERTY_VARIABLE (CString, id, Id)
	PROPERTY_VARIABLE (int, state, State)
	PROPERTY_SHARED_AUTO (UnifiedPackage, package, Package)
	PROPERTY_SHARED_AUTO (IUnifiedPackageHandlerObserver, observer, Observer)

	bool operator== (const UnifiedPackageAction& other) const;

	StringID getGroupId () const;
	StringRef getTitle () const;
	IImage* getIcon () const;
	StringRef getMacroTitle () const;
	StringRef getStateLabel () const;
	StringRef getGroupStateLabel () const;
	void composeTitle (String& title, int itemCount = 1, StringRef details = nullptr) const;

	bool perform ();
	bool cancel ();
	bool pause (bool state);
	void complete (bool success);
	void progress (double progress);
	void packageChanged ();
	void onPause (bool state);
	void requestRestart (StringRef message = nullptr);

	// IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	// Object
	bool equals (const Object& obj) const override
	{
		const UnifiedPackageAction* other = ccl_cast<UnifiedPackageAction> (&obj);
		return other ? *this == *other : SuperClass::equals (obj);
	}

	CLASS_INTERFACE (IReporter, Unknown)

protected:
	IUnifiedPackageHandler* handler;
	int flags;
};

//************************************************************************************************
// IUnifiedPackageHandlerObserver
/** Observes the state of a running UnifiedPackageAction. */
//************************************************************************************************

interface IUnifiedPackageHandlerObserver: Alert::IReporter
{
	/** Progress of the given action has changed */
	virtual void onProgress (const UnifiedPackageAction& action, double progress) = 0;

	/** The given action is completed */
	virtual void onCompletion (const UnifiedPackageAction& action, bool succeeded) = 0;

	/** The given package needs to be refreshed */
	virtual void onPackageChanged (UnifiedPackage* package) = 0;
	
	/** The given action is paused or resumed */
	virtual void onPause (const UnifiedPackageAction& action, bool state) = 0;

	/** Request restart after performing an action */
	virtual void requestRestart (const UnifiedPackageAction& action, StringRef message = nullptr) = 0;

	DECLARE_IID (IUnifiedPackageHandlerObserver)
};

//************************************************************************************************
// UnifiedPackageInstallLocation
/** Information about package install locations. */
//************************************************************************************************

struct UnifiedPackageInstallLocation
{
	CString id;
	String description;
	Url path;
};

//************************************************************************************************
// IUnifiedPackageHandler
/** Provides actions and additional information for UnifiedPackage instances. */
//************************************************************************************************

interface IUnifiedPackageHandler: IUnknown
{
	/** Check if this handler can handle the given package */
	virtual bool canHandle (UnifiedPackage* package) const = 0;

	/** Get all available actions for this package */
	virtual void getActions (Container& actions, UnifiedPackage* package = nullptr) = 0;

	/** Update an action */
	virtual void updateAction (UnifiedPackageAction& action) = 0;

	/** Perform an action */
	virtual bool performAction (UnifiedPackageAction& action) = 0;
	
	/** Pause or resume an action
	    @param[in] state pause the action when set to true and resume otherwise */
	virtual bool pauseAction (UnifiedPackageAction& action, bool state = true) = 0;

	/** Cancel an action */
	virtual bool cancelAction (UnifiedPackageAction& action) = 0;

	/** Create a display component for the given package */
	virtual Component* createComponent (UnifiedPackage* package) = 0;

	/** Get the localized title of the given action */
	virtual StringRef getActionTitle (StringID actionId) const = 0;

	/** Get the icon of the given action */
	virtual IImage* getActionIcon (StringID actionId) const = 0;

	/** Get the localized macro title of the given action */
	virtual StringRef getMacroTitle (StringID actionId) const = 0;

	/** Get the action group of the given action */
	virtual StringID getActionGroupId (StringID actionId) const = 0;

	/** Get the localized state label of the given action */
	virtual StringRef getStateLabel (StringID actionId) const = 0;

	/** Get the localized state label of the given action group */
	virtual StringRef getGroupStateLabel (StringID groupId) const = 0;

	/** Get the localized title of the given action, adding the number of processed items and additional details */
	virtual void composeTitle (String& title, StringID groupId, int itemCount, StringRef details) const = 0;

	/** Get information about install locations */
	virtual bool getInstallLocations (Vector<UnifiedPackageInstallLocation>& locations) const = 0;

	/** Set an install location */
	virtual bool setInstallLocation (StringID locationId, UrlRef path) = 0;
	
	DECLARE_IID (IUnifiedPackageHandler)
};

//************************************************************************************************
// UnifiedPackageHandler
/** Base class for UnifiedPackageHandler implementations. */
//************************************************************************************************

class UnifiedPackageHandler: public Object,
							 public IUnifiedPackageHandler
{
public:
	// basic action ids
	DECLARE_STRINGID_MEMBER (kEnable)
	DECLARE_STRINGID_MEMBER (kDisable)
	DECLARE_STRINGID_MEMBER (kInstall)
	DECLARE_STRINGID_MEMBER (kUninstall)
	DECLARE_STRINGID_MEMBER (kUpdate)
	DECLARE_STRINGID_MEMBER (kRestart)

	// IUnifiedPackageHandler
	StringRef getActionTitle (StringID actionId) const override;
	StringRef getMacroTitle (StringID actionId) const override;
	StringID getActionGroupId (StringID actionId) const override;
	StringRef getStateLabel (StringID actionId) const override;
	StringRef getGroupStateLabel (StringID groupId) const override;
	void composeTitle (String& title, StringID groupId, int itemCount, StringRef details) const override;
	IImage* getActionIcon (StringID actionId) const override { return nullptr; }
	bool pauseAction (UnifiedPackageAction& action, bool state) override { return false; };
	bool getInstallLocations (Vector<UnifiedPackageInstallLocation>& locations) const override { return false; }
	bool setInstallLocation (StringID locationId, UrlRef path) override { return false; }
	
	CLASS_INTERFACE (IUnifiedPackageHandler, Object)

protected:
	virtual UnifiedPackageAction* createAction (UnifiedPackage* package, StringID actionId);
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_unifiedpackageaction_h
