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
// Filename    : ccl/system/persistence/queryresult.cpp
// Description : Query result iterator
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/persistence/queryresult.h"
#include "ccl/system/persistence/classinfo.h"

#include "ccl/public/system/ipersistentstore.h"
#include "ccl/public/plugins/idatabase.h"

using namespace CCL;
using namespace Persistence;
using namespace Database;

//************************************************************************************************
// QueryResultIterator
//************************************************************************************************

QueryResultIterator::QueryResultIterator (IConnection* connection, ClassInfo* classInfo, IExpression* condition)
: connection (connection),
  condition (condition),
  resultSet (nullptr),
  classIndex (-1),
  currentClass (nullptr),
  nextObject (nullptr)
{
	collectClasses (classInfo);

	nextObject = prepareNextClass ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QueryResultIterator::~QueryResultIterator ()
{
	if(resultSet)
		resultSet->release ();
	if(nextObject)
		nextObject->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QueryResultIterator::collectClasses (ClassInfo* classInfo)
{
//	if(!classInfo->isAbstract ())
		classes.add (classInfo);

	ForEach (classInfo->getSubClasses (), ClassInfo, subClass)
		collectClasses (subClass);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* QueryResultIterator::prepareNextClass ()
{
	ASSERT (resultSet == nullptr)
	while(currentClass = (ClassInfo*)classes.at (++classIndex))
	{
		AutoPtr<IStatement> statement (currentClass->createQueryStatement (connection, condition));
		if(statement)
		{
			statement->execute (resultSet);
			if(resultSet && resultSet->nextRow ())
				return currentClass->createObject (*resultSet);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API QueryResultIterator::done () const
{
	return nextObject == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API QueryResultIterator::nextUnknown ()
{
	currentObject = nextObject;
	nextObject = nullptr;

	if(resultSet)
	{
		if(resultSet->nextRow ())
		{
			// todo: if table can contain multiple classes, determine the concrete class of object
			nextObject = currentClass->createObject (*resultSet);
			ASSERT (nextObject)
		}
		else
		{
			resultSet->release ();
			resultSet = nullptr;
			nextObject = prepareNextClass ();
		}
	}

	return currentObject;
}
