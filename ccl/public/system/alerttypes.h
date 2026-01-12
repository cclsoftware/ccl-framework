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
// Filename    : ccl/public/system/alerttypes.h
// Description : Alert types
//
//************************************************************************************************

#ifndef _ccl_alerttypes_h
#define _ccl_alerttypes_h

#include "ccl/public/base/datetime.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

/** \ingroup ccl_system */
namespace Alert { 

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alert Type Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Alert type. */
DEFINE_ENUM (AlertType)
{
	kInformation,		///< Information
	kWarning,			///< Warning
	kError,				///< Error
	kNumAlertTypes
};

/** Alert type to severity conversion. */
inline Severity toSeverity (AlertType type)
{
	switch(type)
	{
	case kInformation : return kSeverityInfo;
	case kWarning : return kSeverityWarning;
	default : ASSERT (type == kError) return kSeverityError;
	}
}

/** Severity to alert type conversion. */
inline AlertType toAlertType (Severity severity)
{
	switch(severity)
	{
	case kSeverityFatal :
	case kSeverityError : 
		return kError;
	case kSeverityWarning : 
		return kWarning;
	case kSeverityInfo :
	case kSeverityDebug :
	case kSeverityTrace :
	default :
		return kInformation;
	}
}

//************************************************************************************************
// Alert::Event
/** \ingroup ccl_system */
//************************************************************************************************

struct Event
{
	AlertType type;			///< type of alert
	Severity severity;		///< severity (finer level of detail than type)
	String message;			///< friendly message
	tresult resultCode;     ///< result code for programmatic error handling	
	DateTime time;			///< timestamp (local time, optional)
	String moduleName;		///< module name (optional)
	String fileName;		///< file name (optional)
	int lineNumber;			///< line number (optional, starts at 1, not zero)
	
	Event (String message = nullptr, AlertType type = kInformation)
	: message (message),
	  type (type),
	  severity (toSeverity (type)),
	  resultCode (kResultOk),
	  lineNumber (0)
	{}
	
	Event (String message, tresult resultCode, AlertType type)
	: message (message),
	  type (type),
	  severity (toSeverity (type)),
	  resultCode (resultCode),
	  lineNumber (0)
	{}
	
	Event (Severity severity, String message)
	: message (message),
	  severity (severity),
	  type (toAlertType (severity)),
	  resultCode (kResultOk),
	  lineNumber (0)
	{}

	enum FormatFlags
	{
		kWithTime	= 1<<0,		///< with time
		kWithAlertType = 1<<1,	///< with alert type prefix
		kWithSeverity = 1<<2,	///< with severity prefix (ignored for kWithAlertType)
		kWithModule = 1<<3		///< with module prefix
	};

	bool isLowLevel () const;				///< low-level event (caused by CCL_WARN)
	String format (int flags = 0) const;	///< format as string
};

//************************************************************************************************
// Alert::IReporter
/** \ingroup ccl_system */
//************************************************************************************************

interface IReporter: IUnknown
{
	/** Report alert event. */
	virtual void CCL_API reportEvent (const Event& e) = 0;

	/** Set minimum logging level and report message format. */
	virtual void CCL_API setReportOptions (Severity minSeverity, int eventFormat) = 0;

	DECLARE_IID (IReporter)
};

} // namespace Alert

/** Alert event reference type. 
	\ingroup ccl_system */
typedef const Alert::Event& AlertEventRef;

} // namespace CCL

#endif // _ccl_alerttypes_h
