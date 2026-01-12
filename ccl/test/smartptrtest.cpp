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
// Filename    : smartptrtest.cpp
// Description : Smart pointers unit test
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/message.h"

#include "ccl/public/collections/iunknownlist.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/network/isocket.h"

using namespace CCL;

//************************************************************************************************
// SmartPointerTest
// This is basically a check if everything compiles...
//************************************************************************************************

CCL_TEST (SmartPointerTest, TestAssignSmartPtr)
{
	IStream* plainStream = nullptr;

	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// assign between different smart pointers of same underlying type
	autoStream = sharedStream;
	autoStream = unknownStream;
	sharedStream = autoStream;
	sharedStream = unknownStream;
	unknownStream = autoStream;
	unknownStream = sharedStream;

	// assign plain pointer
	autoStream = plainStream;
	sharedStream = plainStream;
	unknownStream = plainStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestConstructFromSmartPtr)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// construct from other smart pointer
	AutoPtr<IStream> auto0 (autoStream);
	AutoPtr<IStream> auto1 (sharedStream);
	AutoPtr<IStream> auto2 (unknownStream);

	SharedPtr<IStream> shared0 (autoStream);
	SharedPtr<IStream> shared1 (sharedStream);
	SharedPtr<IStream> shared2 (unknownStream);

	UnknownPtr<IStream> unknown0 (autoStream);
	UnknownPtr<IStream> unknown1 (sharedStream);
	UnknownPtr<IStream> unknown2 (unknownStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestConstructAssignSmartPtr)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// construct from other smart pointer (as assignment)
	AutoPtr<IStream> auto0 = autoStream;
	AutoPtr<IStream> auto1 = sharedStream;
	AutoPtr<IStream> auto2 = unknownStream;

	SharedPtr<IStream> shared0 = autoStream;
	SharedPtr<IStream> shared1 = sharedStream;
	SharedPtr<IStream> shared2 = unknownStream;

	UnknownPtr<IStream> unknown0 = autoStream;
	UnknownPtr<IStream> unknown1 = sharedStream;
	UnknownPtr<IStream> unknown2 = unknownStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestConstructWithPlain)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	IStream* plainStream = nullptr;

	// construct with plain pointer
	AutoPtr<IStream> auto1 (plainStream);
	SharedPtr<IStream> shared (plainStream);
	UnknownPtr<IStream> unknown (plainStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestAssignSuperClass)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// assign plain or smart pointer to smart pointer of SuperClass (IMemoryStream is derived from IStream)
	AutoPtr<IMemoryStream> autoMemStream;
	SharedPtr<IMemoryStream> sharedMemStream;
	UnknownPtr<IMemoryStream> unknownMemStream;
	IMemoryStream* plainMemStream = nullptr;

	autoStream = autoMemStream;
	sharedStream = autoMemStream;
	unknownStream = autoMemStream;

	autoStream = sharedMemStream;
	sharedStream = sharedMemStream;
	unknownStream = sharedMemStream;

	autoStream = unknownMemStream;
	sharedStream = unknownMemStream;
	unknownStream = unknownMemStream;

	autoStream = plainMemStream;
	sharedStream = plainMemStream;
	unknownStream = plainMemStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestUnknownPtr)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// assign unrelated interface to UnknownPtr (-> queryInterface)
	UnknownPtr<IMemoryStream> memStream1 (autoStream);
	UnknownPtr<IContainer> memStream2 (autoStream);

	#if 0 // downcast, would not compile (as expected)
	AutoPtr<IMemoryStream> memStreamX1 (autoStream);
	SharedPtr<IMemoryStream> memStreamX2 (autoStream);
	#endif

	// assign smart pointer to UnknownPtr (any interface, result can be nullptr)
	UnknownPtr<IContainer> container (autoStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestAssignFromVariant)
{
	// assign Variant to UnknownPtr
	Variant variant;
	UnknownPtr<IContainer> container0 (variant);
#if !__clang__
	UnknownPtr<IContainer> container1 = variant; // does not compile in clang!
#endif
	container0 = variant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SmartPointerTest, TestAssignToVariant)
{
	AutoPtr<IStream> autoStream;
	SharedPtr<IStream> sharedStream;
	UnknownPtr<IStream> unknownStream;

	// assign smart pointer to Variant
	Variant v0 (autoStream);
	Variant v1 (sharedStream);
	Variant v2 (unknownStream);

	Variant variant;
	variant = unknownStream;
	variant = autoStream;
	variant = sharedStream;

	// example use case: pass smart pointer as message argument
	Message message (kChanged, unknownStream, autoStream, sharedStream);
}
