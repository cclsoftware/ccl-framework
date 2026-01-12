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
// Filename    : ccl/system/safetymanager.h
// Description : Safety Manager
//
//************************************************************************************************

#ifndef _ccl_safetymanager_h
#define _ccl_safetymanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/system/idiagnosticdataprovider.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/collections/unknownlist.h"

namespace CCL {

//************************************************************************************************
// SafetyManager
//************************************************************************************************

class SafetyManager: public Object,
				     public ISafetyManager,
					 public IDiagnosticDataProvider,
					 public ExternalSingleton<SafetyManager>
{
public:
	SafetyManager ();
	~SafetyManager ();
	
	// ISafetyManager
	void CCL_API setSafetyOptions (int features) override;
	void CCL_API setValue (CStringRef safetyOptionId, tbool state) override;
	tbool CCL_API getValue (CStringRef safetyOptionId) const override;
	tresult CCL_API addFilter (IObjectFilter* filter) override;
	int CCL_API countFilters () const override;
	const IObjectFilter* CCL_API getFilter (int index) const override;
	const IObjectFilter& CCL_API getCombinedFilter () const override;
	void CCL_API registerAction (CStringRef actionId, StringRef title) override;
	void CCL_API beginAction (CStringRef actionId, const String arguments[], int argumentCount) override;
	void CCL_API endAction () override;
	ICrashReport* CCL_API detectCrash () override;
	tresult CCL_API checkStability () override;
	void CCL_API reportException (void* exceptionInformation, const uchar* systemDumpFile) override;
	tbool CCL_API handleException () override;

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;
	
	CLASS_INTERFACE2 (ISafetyManager, IDiagnosticDataProvider, Object)

protected:
	class CrashReport;

	struct TitleMapping
	{
		CString id;
		String title;
	};
	typedef Vector<TitleMapping> TitleMap;

	int features;

	// safety options
	SignalSource signalSource;
	Vector<MutableCString> activeOptions;
	UnknownList filters;
	AutoPtr<IObjectFilter> combinedFilter;
	mutable Threading::CriticalSection optionLock;

	// crash detection
	AutoPtr<CrashReport> crashReport;
	TitleMap actionTitles;
	
	IObjectFilter* createCombinedFilter ();
	virtual void enableCrashRecovery (bool state);
	void reportCrash (const uchar* crashingModulePath, const uchar* systemDumpFile);
	void reportCallingModule (const uchar* callingModulePath);
	void reportUnexpectedBehavior (const uchar* modulePath);
};

} // namespace CCL

#endif // _ccl_safetymanager_h
