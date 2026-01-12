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
// Filename    : ccl/system/persistence/sqlwriter.h
// Description : SQL Writer
//
//************************************************************************************************

#ifndef _ccl_sqlwriter_h
#define _ccl_sqlwriter_h

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Persistence {

interface IExpression;
class ClassInfo;

//************************************************************************************************
// SqlWriter
//************************************************************************************************

class SqlWriter
{
public:
	SqlWriter ();

	// write
	SqlWriter& operator<< (const char* utf8);
	SqlWriter& write (const char* utf8);
	SqlWriter& writeLiteral (VariantRef value);
	SqlWriter& writeStringLiteral (StringRef string);
	SqlWriter& writeExpression (IExpression* expression, ClassInfo& classInfo);

	// get result
	CStringRef getSQL () const    { return sql; }
	operator const char* () const { return sql; }

	SqlWriter& clear ();

private:
	MutableCString sql;
	int outerPriority;

	template<int expressionType> void writeCompoundExpression (IExpression* expression, ClassInfo& classInfo);
	template<int expressionType> void writeCompareExpression (IExpression* expression, ClassInfo& classInfo);
	void writeNotExpression (IExpression* expression, ClassInfo& classInfo);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline SqlWriter& SqlWriter::operator<< (const char* utf8)	{ sql.append (utf8); return *this; }
inline SqlWriter& SqlWriter::write (const char* utf8)		{ sql.append (utf8); return *this; }
inline SqlWriter& SqlWriter::clear ()						{ sql.empty (); return* this; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Persistence
} // namespace CCL

#endif // _ccl_sqlwriter_h
