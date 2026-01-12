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
// Filename    : ccl/system/persistence/queryresult.h
// Description : Query result iterator
//
//************************************************************************************************

#ifndef _objectquery_h
#define _objectquery_h

#include "ccl/base/collections/objectarray.h"

namespace CCL {
class Iterator;
interface IUnknownIterator;

namespace Database {
interface IConnection;
interface IResultSet; }

namespace Persistence {

class ClassInfo;
interface IPersistentStore;
interface IExpression;

//************************************************************************************************
// QueryResultIterator
//************************************************************************************************

class QueryResultIterator: public Object,
						   public IUnknownIterator
{
public:
	QueryResultIterator (Database::IConnection* connection, ClassInfo* classInfo, IExpression* condition);
	~QueryResultIterator ();

	// IUnknownIterator
	tbool CCL_API done () const override;
	IUnknown* CCL_API nextUnknown () override;

	CLASS_INTERFACE (IUnknownIterator, Object)

protected:
	Database::IConnection* connection;
	Database::IResultSet* resultSet;
	IExpression* condition;

	ObjectArray classes;
	ClassInfo* currentClass;
	int classIndex;

	IUnknown* nextObject; ///< created in advance
	AutoPtr<IUnknown> currentObject; ///< the last one we delivered

	void collectClasses (ClassInfo* classInfo);
	IUnknown* prepareNextClass ();
};

} // namespace Persistence
} // namespace CCL

#endif // _objectquery_h
