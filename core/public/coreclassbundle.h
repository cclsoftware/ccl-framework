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
// Filename    : core/public/coreclassbundle.h
// Description : Core Plug-in Class Bundle
//
//************************************************************************************************

#ifndef _coreclassbundle_h
#define _coreclassbundle_h

#include "core/public/coreplugin.h"
#include "core/public/corevector.h"
#include "core/public/corestringbuffer.h"

namespace Core {
namespace Plugins {

//************************************************************************************************
// ClassAttributeWriter
/** Helper to write class attributes to string. */
//************************************************************************************************

template <class StringType>
class ClassAttributeWriter
{
public:
	ClassAttributeWriter (StringType& string)
	: string (string)
	{}

	/** Add class attribute (key/value). */
	void addValue (CStringPtr key, CStringPtr value)
	{
		if(!string.isEmpty ())
			string += "\n";
		string += key;
		string += "=";
		string += value;
	}

protected:
	StringType& string;
};

//************************************************************************************************
// MutableClassBundle
/** Mutable core plug-in class bundle. */
//************************************************************************************************

class MutableClassBundle
{
public:
	MutableClassBundle ()
	{
		rawInfo.versionInfo = versionInfoBuffer.str ();
	}

	/** Get resulting class information bundle. */
	const ClassInfoBundle& getInfo () const
	{
		return rawInfo; 
	}

	/** Add class information (copied). */
	void addClass (const ClassInfo& classInfo)
	{
		ClassInfoBuffer infoBuffer;
		infoBuffer.assign (classInfo);
		classes.add (infoBuffer);

		classInfoPointers.removeAll ();
		for(int i = 0; i < classes.count (); i++)
			classInfoPointers.add (&classes[i].rawInfo);
		rawInfo.numClasses = classInfoPointers.count ();
		rawInfo.classInfos = classInfoPointers;
	}
	
	/** Set version information (copied). */
	void setVersionInfo (CStringPtr versionInfo)
	{
		versionInfoBuffer = versionInfo;
	}
	
protected:
	struct ClassInfoBuffer
	{
		ClassInfo rawInfo;
		CString64 classTypeBuffer;
		CString128 displayNameBuffer;
		CString64 classIDBuffer;
		CString256 classAttributesBuffer;

		ClassInfoBuffer ()
		{
			clear ();
		}

		ClassInfoBuffer (const ClassInfoBuffer& other)
		{
			clear ();
			assign (other.rawInfo);
		}

		ClassInfoBuffer& operator = (const ClassInfoBuffer& other)
		{
			clear ();
			assign (other.rawInfo);
			return *this;
		}

		void clear ()
		{
			::memset (&rawInfo, 0, sizeof(ClassInfo)); 
			rawInfo.classType = classTypeBuffer.str ();
			rawInfo.displayName = displayNameBuffer.str ();
			rawInfo.classID = classIDBuffer.str ();
			rawInfo.classAttributes = classAttributesBuffer.str ();
		}

		void assign (const ClassInfo& classInfo)
		{
			rawInfo.flags = classInfo.flags;
			classTypeBuffer = classInfo.classType;
			displayNameBuffer = classInfo.displayName;
			classIDBuffer = classInfo.classID;
			classAttributesBuffer = classInfo.classAttributes;
			rawInfo.createFunction = classInfo.createFunction;
		}
	};

	ClassInfoBundle rawInfo;
	Vector<ClassInfoBuffer> classes;
	Vector<const ClassInfo*> classInfoPointers;
	CString256 versionInfoBuffer;
};

} // namespace Plugins
} // namespace Core

#endif // _coreclassbundle_h
