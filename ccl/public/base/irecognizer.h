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
// Filename    : ccl/public/base/irecognizer.h
// Description : Recognizer Interface
//
//************************************************************************************************

#ifndef _ccl_irecognizer_h
#define _ccl_irecognizer_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// IRecognizer
/** Interface for recognizing objects.
	\ingroup ccl_base */
//************************************************************************************************

interface IRecognizer: IUnknown
{
	/** Recognize object. */
	virtual tbool CCL_API recognize (IUnknown* object) const = 0;

	DECLARE_IID (IRecognizer)
};

DEFINE_IID (IRecognizer, 0xD26BB017, 0xE844, 0x41B7, 0x9E, 0x12, 0x72, 0x30, 0x63, 0x66, 0x69, 0x17)

//************************************************************************************************
// IResolver
/** Interface for resolving objects.
	\ingroup ccl_base */
//************************************************************************************************

interface IResolver: IUnknown
{
	/** Resolve object. */
	virtual IUnknown* CCL_API resolve (IUnknown* object) const = 0;

	DECLARE_IID (IResolver)
	DECLARE_STRINGID_MEMBER (kExtensionID);
};

DEFINE_IID (IResolver, 0x91b1c554, 0x2717, 0x447f, 0x86, 0x6a, 0x37, 0x66, 0x7, 0xf8, 0x81, 0x6)
DEFINE_STRINGID_MEMBER (IResolver, kExtensionID, "Resolver")

//************************************************************************************************
// IObjectFilter
/** Object filter interface.
	\ingroup ccl_base */
//************************************************************************************************

interface IObjectFilter: IUnknown
{
	/** Tell if the object matches a filter condition. */
	virtual tbool CCL_API matches (IUnknown* object) const = 0;

	DECLARE_IID (IObjectFilter)
};

DEFINE_IID (IObjectFilter, 0xAEE50837, 0x403D, 0x44BF, 0xA5, 0xE7, 0x46, 0x72, 0x43, 0x2C, 0x5B, 0x08)

//************************************************************************************************
// Recognizer
/** Helper class for implementing IRecognizer.
	\ingroup ccl_base */
//************************************************************************************************

class Recognizer: public Unknown, 
				  public IRecognizer
{
public:
	CLASS_INTERFACE (IRecognizer, Unknown)

	// create with lambda
	template <typename T> static IRecognizer* create (const T& lambda);
};

//************************************************************************************************
// Resolver
/** Helper class for implementing IResolver. 
	\ingroup ccl_base */
//************************************************************************************************

class Resolver: public Unknown, 
				public IResolver
{
public:
	CLASS_INTERFACE (IResolver, Unknown)

	// create with lambda
	template <typename T> static IResolver* create (const T& lambda);
};

//************************************************************************************************
// ObjectFilter
/** Helper class for implementing IObjectFilter.
	\ingroup ccl_base */
//************************************************************************************************

class ObjectFilter: public Unknown, 
					public IObjectFilter
{
public:
	CLASS_INTERFACE (IObjectFilter, Unknown)

	// create with lambda
	template <typename T> static ObjectFilter* create (const T& lambda);
};

//************************************************************************************************
// AlwaysTrueFilter
/** Matches any object. 
	\ingroup ccl_base */
//************************************************************************************************

class AlwaysTrueFilter: public ObjectFilter
{
public:
	tbool CCL_API matches (IUnknown* object) const override { return true; }
};

//************************************************************************************************
// LambdaFilter
/** Wraps the IObjectFilter and IRecognizer interface around a lambda function.
	\ingroup ccl_base */
//************************************************************************************************

template <typename T>
class LambdaFilter: public ObjectFilter,
					public IRecognizer
{
public:
	LambdaFilter (const T& lambda)
	: lambda (lambda)
	{}

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override
	{
		return lambda (object);
	}

	// IRecognizer
	tbool CCL_API recognize (IUnknown* object) const override
	{
		return lambda (object);
	}

	CLASS_INTERFACE (IRecognizer, ObjectFilter)

protected:
	T lambda;
};

//************************************************************************************************
// LambdaResolver
/** Wraps the IResolver interface around a lambda function.
	\ingroup ccl_base */
//************************************************************************************************

template <typename T>
class LambdaResolver: public Resolver
{
public:
	LambdaResolver (const T& lambda)
	: lambda (lambda)
	{}

	// IResolver
	IUnknown* CCL_API resolve (IUnknown* object) const override
	{
		return lambda (object);
	}

	CLASS_INTERFACE (IResolver, Resolver)

protected:
	T lambda;
};

//************************************************************************************************
// ObjectFilterChain
/** Matches when all sub-filters match. 
    \ingroup ccl_base */
//************************************************************************************************

class ObjectFilterChain: public ObjectFilter
{
public:
	ObjectFilterChain& addFilter (CCL::IObjectFilter* filter) // filter treated shared
	{ 
		filters.add (filter); 
		return *this;
	}	
	
	void removeAll ()
	{
		filters.removeAll ();
	}

	tbool CCL_API matches (IUnknown* object) const override
	{
		for(auto& filter : filters)
		{
			if(filter->matches (object) == false)
				return false;
		}
		return true;
	}
protected:
	Vector <SharedPtr <IObjectFilter> > filters; 
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
ObjectFilter* ObjectFilter::create (const T& lambda)
{ return NEW LambdaFilter<T> (lambda); }

template <typename T> IRecognizer* Recognizer::create (const T& lambda)
{ return NEW LambdaFilter<T> (lambda); }

template <typename T> IResolver* Resolver::create (const T& lambda)
{ return NEW LambdaResolver<T> (lambda); }

} // namespace CCL

#endif // _ccl_irecognizer_h
