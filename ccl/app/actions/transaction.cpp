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
// Filename    : ccl/app/actions/transaction.cpp
// Description : Transaction
//
//************************************************************************************************

#include "ccl/app/actions/transaction.h"

using namespace CCL;

//************************************************************************************************
// Transaction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Transaction, Object)

//************************************************************************************************
// UndoableTransaction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UndoableTransaction, Transaction)

//************************************************************************************************
// SimpleTransaction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SimpleTransaction, Transaction)

//************************************************************************************************
// TransactionExecuter
//************************************************************************************************

TransactionExecuter::TransactionExecuter (Transaction* t)
: t (t), count (0)
{
	t->begin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransactionExecuter::TransactionExecuter (UndoableTransaction* t, IActionContext* c)
: t (t), count (0)
{
	t->init (c);
	t->begin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransactionExecuter::~TransactionExecuter ()
{
	t->describe (count > 1);
	t->end ();
	t->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransactionExecuter::apply (Object* object)
{
	t->apply (object);
	count++;
}
