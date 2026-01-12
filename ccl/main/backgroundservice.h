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
// Filename    : ccl/main/backgroundservice.h
// Description : Background Service
//
//************************************************************************************************

#ifndef _ccl_backgroundservice_h
#define _ccl_backgroundservice_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// BackgroundService
//************************************************************************************************

class BackgroundService: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (BackgroundService, Object)

	BackgroundService (StringRef name, StringRef description, StringRef company, int versionInt = 0);
	~BackgroundService ();

	static const int kIdlePeriod = 20; ///< idle period in milliseconds
	static BackgroundService* getInstance ();
	
	// reduce effective idle period
	PROPERTY_BOOL (highPerformanceMode, HighPerformanceMode)
	
	StringRef getName () const;
	StringRef getDescription () const;
	bool isRunningAsService () const;

	/** Call this from ccl_main() to hand control to underlying service implementation. */
	int run (bool developerMode = false);

	/** Install service - platform-specific. */
	bool install (bool state);

	/** Service is being started. */
	virtual bool startup (ArgsRef args) = 0;

	/** Service is about to shutdown. */
	virtual void shutdown () = 0;

	/** Called periodically from main thread. */
	virtual void onIdle ();

	/** Call during modal loop to remain responsive. This will in turn call onIdle() occasionally. */
	void flushPlatformUpdates ();

	// Device plug&play notification (Windows only)
	void enableDeviceNotifications (const ConstVector<UID>& filter);
	DECLARE_STRINGID_MEMBER (kDeviceNotification) ///< args[0]: device class GUID (string)

protected:
	static BackgroundService* theInstance;

	String name;
	String description;
	bool runningAsService;

	/** Start in development mode, no connection to OS. */
	bool startDevelopmentService ();

	/** Start service and connect with OS - platform-specific. */
	bool startPlatformService ();
};

} // namespace CCL

#endif // _ccl_backgroundservice_h
