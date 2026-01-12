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
// Filename    : ccl/public/gui/paramlist.cpp
// Description : IParameter List
//
//************************************************************************************************

#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itextmodel.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ParamList
//************************************************************************************************

ParamList::ParamList ()
: controller (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList::~ParamList ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::newParameter (UIDRef cid) const
{
	return ccl_new<IParameter> (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::setController (IParamObserver* c)
{
	controller = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::add (IParameter* p, int tag)
{
	ASSERT (p != nullptr)
	if(p)
	{
		p->connect (controller, tag);
		params.add (p);
	}
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addShared (IParameter* p)
{
	ASSERT (p != nullptr)
	if(p)
	{
		params.add (p);
		p->retain ();
	}
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addParam (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::Parameter);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addInteger (int min, int max, StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::IntParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setMin (min);
	p->setMax (max);
	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addFloat (double min, double max, StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::FloatParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setMin (min);
	p->setMax (max);
	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addString (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::StringParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addList (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::ListParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addMenu (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::MenuParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addPalette (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::PaletteParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addCommand (StringID commandCategory, StringID commandName, StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::CommandParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	UnknownPtr<ICommandParameter> cmdParam (p);
	ASSERT (cmdParam != nullptr)
	cmdParam->setCommand (commandCategory, commandName);
	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addScroll (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::ScrollParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addColor (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::ColorParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return add (p, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImageProvider* ParamList::addImage (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::ImageProvider);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return UnknownPtr<IImageProvider> (add (p, tag));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextModelProvider* ParamList::addTextModel (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::TextModelProvider);
	ASSERT (p)
	if(!p)
		return nullptr;

	p->setName (name);
	return UnknownPtr<ITextModelProvider> (add (p, tag));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAliasParameter* ParamList::addAlias (StringID name, int tag)
{
	IParameter* p = newParameter (ClassID::AliasParam);
	ASSERT (p != nullptr)
	if(!p)
		return nullptr;

	p->setName (name);
	return UnknownPtr<IAliasParameter> (add (p, tag));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList::ParamArray* ParamList::lookupArray (StringID name) const
{
	VectorForEach (arrays, ParamArray*, a)
		if(a->name == name)
			return a;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addIndexedParam (StringID arrayName, IParameter* p, int tag)
{
	ParamArray* a = lookupArray (arrayName);
	if(!a)
		arrays.add (a = NEW ParamArray (arrayName));

	p->connect (controller, tag);
	a->add (p);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::addIndexedParamShared (StringID arrayName, IParameter* p)
{
	ParamArray* a = lookupArray (arrayName);
	if(!a)
		arrays.add (a = NEW ParamArray (arrayName));
	a->add (p);
	p->retain ();
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::getIndexedParam (StringID arrayName, int index) const
{
	ParamArray* a = lookupArray (arrayName);
	if(a == nullptr || index >= a->count ())
		return nullptr;

	return a->at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ParamList::getParamArrayCount (StringID arrayName) const
{
	ParamArray* a = lookupArray (arrayName);
	return a ? a->count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList::ParamIterator* ParamList::getParamArray (StringID arrayName) const
{
	ParamArray* a = lookupArray (arrayName);
	return a ? NEW ParamIterator (*a) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ParamList::count () const
{
	return params.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::at (int index) const
{
	return params.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ParamList::arrayCount () const
{
	return arrays.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList::ParamIterator* ParamList::arrayAt (int index) const
{
	ParamArray* a = arrays.at (index);
	return a ? NEW ParamIterator (*a) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::byTag (int tag) const
{
	VectorForEach (params, IParameter*, p)
		if(p->getTag () == tag)
			return p;
	EndFor

	if(!arrays.isEmpty ())
	{
		VectorForEach (arrays, ParamArray*, a)
			VectorForEach (*a, IParameter*, p)
				if(p->getTag () == tag)
					return p;
			EndFor
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::byCommand (StringID commandCategory, StringID commandName) const
{
	VectorForEach (params, IParameter*, p)
		UnknownPtr<ICommandParameter> cmdParam (p);
		if(cmdParam)
		{
			if(cmdParam->getCommandCategory () == commandCategory && cmdParam->getCommandName () == commandName)
				return p;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamList::lookup (StringID name) const
{
	if(name[0] == '@')
	{
		int idx1 = name.index ("[");
		int idx2 = name.index ("]");
		if(idx1 > -1 && idx2 > idx1)
		{
			int64 arrayIndex = -1;
			MutableCString temp = name.subString (idx1 + 1, idx2 - idx1 - 1);
			temp.getIntValue (arrayIndex);

			temp = name.subString (1, idx1 - 1);
			ParamArray* a = lookupArray (temp);
			IParameter* p = a ? a->at ((int)arrayIndex) : 0;
			return p;
		}
	}

	VectorForEach (params, IParameter*, p)
		if(p->getName () == name)
			return p;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::removeAll ()
{
	VectorForEach (params, IParameter*, p)
 		p->release ();
	EndFor

	VectorForEach (arrays, ParamArray*, a)
		VectorForEach (*a, IParameter*, p)
			p->release ();
		EndFor
		delete a;
	EndFor

	params.removeAll ();
	arrays.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::removeArray (StringID arrayName)
{
	if(ParamArray* a = lookupArray (arrayName))
	{
		VectorForEach (*a, IParameter*, p)
			p->release ();
		EndFor

		arrays.remove (a);
		delete a;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::removeIndexedParam (StringID arrayName, int index)
{
	if(ParamArray* a = lookupArray (arrayName))
	{
		IParameter* p = a->at (index);
		if(a->remove (p))
			p->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamList::remove (IParameter* p, bool releaseParam)
{
	auto success = [&] ()
	{
		if(p && releaseParam)
			p->release ();
		return true;
	};
	
	if(params.remove (p))
		return success ();
	VectorForEach (arrays, ParamArray*, a)
		if(a->remove (p))
			return success ();
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamList::contains (IParameter* p)
{
	return params.contains (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamList::toHead (IParameter* p)
{
	int index = params.index (p);
	if(index > 0)
	{
		params.removeAt (index);
		params.insertAt (0, p);
	}
	return index != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::checkCommandStates ()
{
	VectorForEach (params, IParameter*, p)
		UnknownPtr<ICommandParameter> cmdParam (p);
		if(cmdParam)
			cmdParam->checkEnabled ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::enableCommands (bool state)
{
	VectorForEach (params, IParameter*, p)
		UnknownPtr<ICommandParameter> cmdParam (p);
		if(cmdParam)
			p->enable (state);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::setFeedbackNeeded (bool state)
{
	VectorForEach (params, IParameter*, p)
		p->setFeedbackNeeded (state);
	EndFor
}
