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
// Filename    : ccl/public/system/diagnosticprofiler.h
// Description : Diagnostic profiler
//
//************************************************************************************************

#ifndef _ccl_diagnosticprofiler_h
#define _ccl_diagnosticprofiler_h

#include "ccl/public/base/profiler.h"
#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/base/istream.h"

namespace CCL {

//************************************************************************************************
// DiagnosticTimeAccumulator
//************************************************************************************************

struct DiagnosticTimeAccumulator: Profiler::TimeAccumulator
{
	DiagnosticTimeAccumulator (StringID context, StringID key, StringRef label = nullptr)
	: TimeAccumulator (context, false),
	  context (context),
	  key (key),
	  label (label),
	  enabled (true)
	{}

	~DiagnosticTimeAccumulator ()
	{
		if(!isEnabled ())
			return;
		for(int i = 0; i < iterations; i++)
			System::GetDiagnosticStore ().submitValue (context, key, elapsed / iterations, label);
	}

	PROPERTY_BOOL (enabled, Enabled)

private:
	MutableCString context;
	CString key;
	String label;
};

//************************************************************************************************
// DiagnosticProfilingScope
//************************************************************************************************

struct DiagnosticProfilingScope
{
	DiagnosticProfilingScope (StringID context, StringID key, StringRef label = nullptr)
	: accumulator (context, key, label),
	  scope (accumulator)
	{}

	void setEnabled (bool state)
	{
		accumulator.setEnabled (state);
	}

private:
	DiagnosticTimeAccumulator accumulator;
	Profiler::Scope<DiagnosticTimeAccumulator> scope;
};

//************************************************************************************************
// DiagnosticSizeProfilingScope
//************************************************************************************************

struct DiagnosticSizeProfilingScope
{
	DiagnosticSizeProfilingScope (StringID context, StringID key, IStream& stream, StringRef label = nullptr)
	: context (context),
	  key (key),
	  stream (stream),
	  label (label),
	  streamPosition (stream.tell ()),
	  enabled (true)
	{}

	~DiagnosticSizeProfilingScope ()
	{
		if(!isEnabled ())
			return;
		System::GetDiagnosticStore ().submitValue (context, key, stream.tell () - streamPosition, label);
	}

	PROPERTY_BOOL (enabled, Enabled)

private:
	MutableCString context;
	CString key;
	IStream& stream;
	String label;
	int64 streamPosition;
};

//************************************************************************************************
// DiagnosticScope
//************************************************************************************************

struct DiagnosticScope
{
	DiagnosticScope (IDiagnosticStore::DiagnosticMode mode = IDiagnosticStore::kShortTerm)
	{
		oldMode = System::GetDiagnosticStore ().setMode (mode);
	}

	~DiagnosticScope ()
	{
		System::GetDiagnosticStore ().setMode (oldMode);
	}

private:
	IDiagnosticStore::DiagnosticMode oldMode;
};

} // namespace CCL

#endif // _ccl_diagnosticprofiler_h
