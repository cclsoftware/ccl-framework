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
// Filename    : ccl/public/gui/paramlist.h
// Description : Parameter List
//
//************************************************************************************************

#ifndef _ccl_paramlist_h
#define _ccl_paramlist_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

interface IParameter;
interface IParamObserver;
interface IAliasParameter;
interface IImageProvider;
interface ITextModelProvider;

//************************************************************************************************
// ParamList macros
//************************************************************************************************

/** Delegate parameter lookup to list when implementing controllers. */
#define DECLARE_PARAMETER_LOOKUP(paramList)									\
int CCL_API countParameters () const override								\
{ return paramList.count (); }												\
CCL::IParameter* CCL_API getParameterAt (int index)	const override			\
{ return paramList.at (index); }											\
CCL::IParameter* CCL_API findParameter (CCL::StringID name) const override	\
{ return paramList.lookup (name); }											\
CCL::IParameter* CCL_API getParameterByTag (int tag) const override			\
{ return paramList.byTag (tag); }					

//************************************************************************************************
// ParamList
/** Manages a list of parameters. New parameters are connected to associated controller automatically. 
	\ingroup gui_param */
//************************************************************************************************

class ParamList
{
public:
	ParamList ();
	virtual ~ParamList ();

	typedef VectorIterator<IParameter*> ParamIterator;

	/** Set controller. */
	void setController (IParamObserver* controller);

	/** Add toggle parameter (on/off). */
	IParameter* addParam (StringID name, int tag = 0);

	/** Add numeric parameter (integer). */
	IParameter* addInteger (int min, int max, StringID name, int tag = 0);

	/** Add numeric parameter (floating-point). */
	IParameter* addFloat (double min, double max, StringID name, int tag = 0);

	/** Add string parameter. */
	IParameter* addString (StringID name, int tag = 0);

	/** Add list parameter. */
	IParameter* addList (StringID name, int tag = 0);

	/** Add menu parameter. */
	IParameter* addMenu (StringID name, int tag = 0);

	/** Add palette parameter. */
	IParameter* addPalette (StringID name, int tag = 0);

	/** Add command parameter. */
	IParameter* addCommand (StringID commandCategory, StringID commandName, StringID name, int tag = 0);

	/** Add scroll parameter. */
	IParameter* addScroll (StringID name, int tag = 0);

	/** Add color parameter. */
	IParameter* addColor (StringID name, int tag = 0);

	/** Add image provider. */
	IImageProvider* addImage (StringID name, int tag = 0);

	/** Add text model provider. */
	ITextModelProvider* addTextModel (StringID name, int tag = 0);

	/** Add alias parameter. */
	IAliasParameter* addAlias (StringID name, int tag = 0);

	/** Add existing parameter, list takes ownership. */
	IParameter* add (IParameter* p, int tag = 0);

	/** Share parameter owned by other controller. */
	IParameter* addShared (IParameter* p);

	/** Add indexed parameter to array. */
	IParameter* addIndexedParam (StringID arrayName, IParameter* p, int tag = 0);

	/** Share indexed parameter owned by other controller. */
	IParameter* addIndexedParamShared (StringID arrayName, IParameter* p);

	/** Get indexed parameter from array. */
	IParameter* getIndexedParam (StringID arrayName, int index) const;

	/** Get number of parameters in array. */
	int getParamArrayCount (StringID arrayName) const;

	/** Create iterator for parameter array. */
	ParamIterator* getParamArray (StringID arrayName) const;

	/** Get parameter count. */
	int count () const;

	/** Get parameter by index. */
	IParameter* at (int index) const;
	
	/** Return the number of arrays */
	int arrayCount () const;

	/** Return an iterator for the array with the given index */
	ParamIterator* arrayAt (int index) const;

	/** Get parameter by tag. */
	IParameter* byTag (int tag) const;

	/** Get parameter by command category/name. */
	IParameter* byCommand (StringID commandCategory, StringID commandName) const;

	/** Get parameter by name. */
	IParameter* lookup (StringID name) const;

	/** Remove one parameter. Refcount is decremented according to releaseParam. */
	bool remove (IParameter* p, bool releaseParam = false);

	/** Move parameter to first position in list for faster lookup. */
	bool toHead (IParameter* p);

	/** Check if parameter is in list. */
	bool contains (IParameter* p);

	/** Remove all parameters. */
	void removeAll ();
	
	/** Remove parameter array. */
	void removeArray (StringID arrayName);

	/** Remove indexed parameter from array (releases parameter). */
	void removeIndexedParam (StringID arrayName, int index);

	/** Check states of command parameters. */
	void checkCommandStates ();
	
	/** Enable or disable all command parameters */
	void enableCommands (bool state);

	/** Set feedback flag for all parameters. */
	void setFeedbackNeeded (bool state);

protected:
	class ParamArray: public Vector<IParameter*>
	{
	public:
		ParamArray (StringID name)
		: name (name)
		{}

		MutableCString name;
	};

	Vector<IParameter*> params;
	Vector<ParamArray*> arrays;
	IParamObserver* controller;

	ParamArray* lookupArray (StringID name) const;

	virtual IParameter* newParameter (UIDRef cid) const;
};

} // namespace CCL

#endif // _ccl_paramlist_h
