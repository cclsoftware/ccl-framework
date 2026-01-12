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
// Filename    : sqlitetest.cpp
// Description : SQLite database tests
//
//************************************************************************************************

#include "sqlitetest.h"
#include "plugversion.h"

#include "ccl/base/unittest.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/plugins/idatabase.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Database;

//************************************************************************************************
// SQLiteTest
//************************************************************************************************

class SQLiteTest: public Test
{
public:
	SQLiteTest ()
	: databaseEngine (nullptr)
	{}
	
	~SQLiteTest ()
	{
		if(databaseUrl)
			System::GetFileSystem ().removeFile (*databaseUrl);

		if(databaseEngine)
			ccl_release (databaseEngine);
	}
	
	void setUp () override
	{
		if(!databaseEngine)
		{
			databaseUrl = NEW Url (String::kEmpty);
			System::GetSystem ().getLocation (*databaseUrl, System::kTempFolder);
			databaseUrl->descend ("sqlitetest.db");
			System::GetFileUtilities ().makeUniqueFileName (System::GetFileSystem (), *databaseUrl, false);

			// create database connection
			databaseEngine = ccl_new<IDatabaseEngine> (CCL::ClassID::SQLite);
			if(databaseEngine)
				connection = databaseEngine->createConnection (*databaseUrl);
		}
	}
	
protected:
	AutoPtr<IUrl> databaseUrl;
	AutoPtr<IConnection> connection;
	IDatabaseEngine* databaseEngine;
	
	void select (bool ordered)
	{
		if(connection)
		{
			MutableCString sql ("select * from files");
			if(ordered)
				sql.append (" order by path");
			AutoPtr<IStatement> statement = connection->createStatement (sql);
			if(statement)
			{
				AutoPtr<IResultSet> result;
				if(statement->execute (result))
				{
					int rows = 0;
					while(result->nextRow ())
					{
						String str;
						result->getStringValue (result->getColumnIndex ("id"), str);

						Variant value;
						result->getValue (1, value);
						str.append (": ");
						str.append (value.asString ());

						if(rows++ < 10)
							Logging::debug (str);
					}
					Logging::debug ("...");
				}
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static const char* folderNames[]=
	{"build", "ccl", "trunk", "devices", "engine", "lib", "media", "services", "simrec", "studioapp", "testapp", "testarea", "tools", nullptr};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (SQLiteTest, TestSelect)
{
	select (false);
}

CCL_TEST_F (SQLiteTest, TestSelectOrdered)
{
	select (true);
}

CCL_TEST_F (SQLiteTest, TestInsert)
{
	if(connection)
	{
		connection->beginTransaction ();
		connection->execute ("drop table if exists files");
		connection->execute ("create table files (id INTEGER PRIMARY KEY, path TEXT)");
		connection->execute ("create index table_path on files (path)");

		AutoPtr<IStatement> statement = connection->createStatement ("insert into files (path) values (?)");
		if(statement)
		{
			for(int index = 0; index < 1000; index++)
			{
				int i = 0;
				while(const char* v = folderNames[i++])
				{
					String str (v);
					str.appendIntValue (index);
					statement->bindVariable (0, str);
					statement->execute ();
				}
			}
		}
		connection->commitTransaction ();
	}
}

CCL_TEST_F (SQLiteTest, TestUpdate)
{
	if(connection)
		connection->execute ("update files set path= path || ' (Updated)' where path like 'lib%'");
}
