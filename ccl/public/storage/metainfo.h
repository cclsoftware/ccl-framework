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
// Filename    : ccl/public/storage/metainfo.h
// Description : Meta Info Attributes
//
//************************************************************************************************

#ifndef _ccl_metainfo_h
#define _ccl_metainfo_h

#include "ccl/public/storage/iattributelist.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Meta Info Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare meta info attribute [String]. */
#define METAINFO_ATTRIBUTE_STRING(Name, key) \
void set##Name (CCL::StringRef value) { set (key, value); } \
CCL::String get##Name () const { CCL::String result; CCL::Variant value; if(getList ().getAttribute (value, key)) value.toString (result); return result; }

/** Declare meta info attribute [float]. */
#define METAINFO_ATTRIBUTE_FLOAT(Name, key) \
void set##Name (double value) { set (key, value); } \
double get##Name () const { return getFloat (key); }

/** Declare meta info attribute [int]. */
#define METAINFO_ATTRIBUTE_INT(Name, key) \
void set##Name (int value) { set (key, value); } \
int get##Name () const { return getInt (key); }

/** Declare meta info attribute [int64]. */
#define METAINFO_ATTRIBUTE_INT64(Name, key) \
void set##Name (int64 value) { set (key, value); } \
int64 get##Name () const { return getInt64 (key); }

/** Declare meta info attribute [bool]. */
#define METAINFO_ATTRIBUTE_BOOL METAINFO_ATTRIBUTE_INT

/** Declare meta info attribute [UID] (stored as string!). */
#define METAINFO_ATTRIBUTE_UID(Name, key) \
void set##Name (const CCL::UID& uid) { String str; uid.toString (str); set (key, str); } \
bool get##Name (CCL::UID& uid) const { CCL::String str; return getString (str, key) && uid.fromString (str); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_metainfo_h
