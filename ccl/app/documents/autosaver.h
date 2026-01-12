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
// Filename    : ccl/app/documents/autosaver.h
// Description : Autosave Documents
//
//************************************************************************************************

#ifndef _ccl_autosaver_h
#define _ccl_autosaver_h

#include "ccl/app/component.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

class Document;
class DocumentManager;
interface IAsyncOperation;

//************************************************************************************************
// IAutoSaveHook
//************************************************************************************************

interface IAutoSaveHook: IUnknown
{
	/** Tells if autosaving is allowed at this moment. */
	virtual bool canAutoSaveNow (bool urgent) = 0;

	/** Autosave begin/end notification. */
	virtual void onAutoSave (bool begin) = 0;

	DECLARE_IID (IAutoSaveHook)
};

//************************************************************************************************
// AutoSaver
//************************************************************************************************

class AutoSaver: public Component,
				 public ITimerTask,
				 public ComponentSingleton<AutoSaver>
{
public:
	DECLARE_CLASS (AutoSaver, Component)

	AutoSaver ();

	void enable (bool state);
	void resetTimer ();

	IAsyncOperation* tryAutoSavedFile (Document& document, bool isEmergency = false); ///< looks for a newer autosave file, asks user (the date is not checked in case of emergency)
	void removeAutoSaveFile (Document& document);
	bool canSaveNow (bool urgent);
	bool isAutoSaving () const;
	bool doSave (Document& document);

	static const CCL::String kAutosaveExtension;
	static bool isAutoSaveFile (UrlRef path);

	PROPERTY_AUTO_POINTER (IAutoSaveHook, autoSaveHook, AutoSaveHook)

	PROPERTY_VARIABLE (int, saveTimeout, SaveTimeout)			///< ms to wait after the last save
	PROPERTY_VARIABLE (int, softUserTimeout, SoftUserTimeout)	///< ms to wait after a user action (mouse move / commands)
	PROPERTY_VARIABLE (int, hardUserTimeout, HardUserTimeout)	///< ms to wait after a user action after the grace period
	PROPERTY_VARIABLE (int, gracePeriod, GracePeriod)			///< when saveTimeout has passed, we try the softUserTimeout for this period (ms)
	PROPERTY_VARIABLE (int, numFilesToKeep, NumFilesToKeep)		///< number of autosave files to keep in history folder; when that number is exceeded, the oldest one gets deleted
	PROPERTY_BOOL (overwrite, Overwrite)						///< overwrite the opened file instead of creating .autosave file(s)
	PROPERTY_BOOL (suspended, Suspended)

	// ITimerTask
	void CCL_API onTimer (ITimer*) override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (ITimerTask, Component)

	struct Suspender
	{
		Suspender ();
		~Suspender ();

		AutoSaver& autoSaver;
		bool wasSuspended;
	};

private:
	DocumentManager& manager;
	ITimer* timer;
	int64 nextTime;
	bool autoSaving;
	static Configuration::BoolValue enabled;
	static Configuration::IntValue period;

	void makeAutoSavePath (Url& path, const Document& document);
	bool checkDocument (Document& document);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AutoSaver::isAutoSaving () const
{ return autoSaving; }

} // namespace CCL

#endif // _ccl_autosaver_h
