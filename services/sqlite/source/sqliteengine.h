//************************************************************************************************
//
// SQLite Database Engine
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
// Filename    : sqliteengine.h
// Description : SQLite database engine
//
//************************************************************************************************

#ifndef _sqliteengine_h
#define _sqliteengine_h

#include "ccl/base/object.h"

#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/plugins/idatabase.h"

struct sqlite3;

namespace CCL {
namespace Database {

//************************************************************************************************
// SQLiteEngine
//************************************************************************************************

class SQLiteEngine: public Object,
					public PluginInstance,
					public IDatabaseEngine
{
public:
	SQLiteEngine ();
	~SQLiteEngine ();

	static IUnknown* createInstance (UIDRef, void*);

	// IDatabaseEngine
	IConnection* CCL_API createConnection (UrlRef url) override;
	
	CLASS_INTERFACE2 (IDatabaseEngine, IPluginInstance, Object)
};

} // namespace Database
} // namespace CCL

#endif // _sqliteengine_h
