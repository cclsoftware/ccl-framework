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
// Filename    : ccl/public/gui/framework/iskinmodel.cpp
// Description : Skin Model Interfaces
//
//************************************************************************************************

#include "ccl/public/gui/framework/iskinmodel.h"

#include "ccl/public/base/iobject.h"
#include "ccl/public/collections/iunknownlist.h"

using namespace CCL;

//************************************************************************************************
// SkinModelAccessor
//************************************************************************************************

ISkinElement* SkinModelAccessor::find (IContainer* c, StringID name, StringID typeName)
{
	if(c) ForEachUnknown (*c, unk)
		if(UnknownPtr<ISkinElement> e = unk)
		{
			if(e->getName () == name)
			{
				if(!typeName.isEmpty ())
				{
					if(auto typeInfo = e->getElementClass ())
						if(typeName != typeInfo->getClassName ())
							continue;
				}
				return e;
			}
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModelAccessor::SkinModelAccessor (ISkinModel& model)
: model (model)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinElement* SkinModelAccessor::findForm (StringID formName) const
{
	return find (model.getContainerForType (ISkinModel::kFormsElement), formName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinElement* SkinModelAccessor::findResource (StringID name, StringID typeName) const
{
	return find (model.getContainerForType (ISkinModel::kResourcesElement), name, typeName);
}
