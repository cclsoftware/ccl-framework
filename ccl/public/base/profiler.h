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
// Filename    : ccl/public/base/profiler.h
// Description : Profiler
//
//************************************************************************************************

#ifndef _ccl_profiler_h
#define _ccl_profiler_h

#include "ccl/public/base/debug.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// Profiler
/** \ingroup ccl_base */
//************************************************************************************************

class Profiler
{
public:
	//********************************************************************************************
	// Registrar
	//********************************************************************************************

	struct Primitive;
	typedef LinkedList<Primitive*> Registrar;

	static Registrar& getRegistrar ()
	{
		static Deleter<Registrar> theRegistrar (nullptr);
		if(theRegistrar._ptr == nullptr)
			theRegistrar._ptr = NEW Registrar;
		return *theRegistrar._ptr;
	}

	//********************************************************************************************
	// Scope
	//********************************************************************************************

	template <class T> 
	struct Scope
	{
		T& p;
		
		Scope (T& p)
		: p (p)
		{ p.begin (); }

		~Scope ()
		{ p.end (); }
	};

	//********************************************************************************************
	// Primitive
	//********************************************************************************************

	struct Primitive
	{
		const char* name;

		Primitive (const char* name = nullptr)
		: name (name)
		{}

		~Primitive ()
		{}

		const char* getName () const { return name ? name : ""; }
	};

	//********************************************************************************************
	// TimeAccumulator
	//********************************************************************************************

	struct TimeAccumulator: Primitive
	{
		double elapsed;
		double variance;
		double start;
		int iterations;
		bool verbose;

		TimeAccumulator (const char* name = nullptr, bool verbose = true)
		: Primitive (name),
		  elapsed (0),
		  start (0),
		  iterations (0),
		  variance (0),
		  verbose (verbose)
		{}

		~TimeAccumulator ()
		{
			if(verbose)
				print ();
		}

		void begin ()
		{
			start = System::GetProfileTime ();
		}

		void end ()
		{
			double end = System::GetProfileTime ();
			double duration = end - start;
			double oldAverage = elapsed / iterations;
			iterations++;
			elapsed += duration;

			if(iterations == 1)
			{
				variance = 0;
				return;
			}

			double average = elapsed / iterations;
			variance += oldAverage * oldAverage - average * average + (duration * duration - variance - oldAverage * oldAverage) / iterations;
		}

		double getStdDev () const
		{
			return sqrt (variance);
		}

		double getAverage () const
		{
			return elapsed / iterations;
		}

		void reset ()
		{
			variance = 0;
			iterations = 0;
			elapsed = 0;
		}

		void print ()
		{
			Debugger::printf ("%s elapsed: %d iterations in %lf seconds\n", getName (), iterations, elapsed);
		}
	};
};

} // namespace CCL

#endif // _ccl_profiler_h
