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
// Filename    : ccl/app/actions/transaction.h
// Description : Transaction
//
//************************************************************************************************

#ifndef _ccl_transaction_h
#define _ccl_transaction_h

#include "ccl/base/object.h"

namespace CCL {

interface IActionContext;

//************************************************************************************************
// Transaction
/** Transaction to be applied to multiple objects. */
//************************************************************************************************

class Transaction: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Transaction, Object)

	virtual void begin () = 0;

	virtual void apply (Object* object) = 0;

	virtual void describe (bool multiple) = 0;

	virtual void end () = 0;
};

//************************************************************************************************
// UndoableTransaction
/** Transaction with support for undo. */
//************************************************************************************************

class UndoableTransaction: public Transaction
{
public:
	DECLARE_CLASS_ABSTRACT (UndoableTransaction, Transaction)

	virtual void init (IActionContext* c) = 0;
};

//************************************************************************************************
// SimpleTransaction
/** Simple transaction base class. */
//************************************************************************************************

class SimpleTransaction: public Transaction
{
public:
	DECLARE_CLASS_ABSTRACT (SimpleTransaction, Transaction)

	// Transaction
	void begin () override {}
	void describe (bool multiple) override {}
	void end () override {}
};

//************************************************************************************************
// TransactionExecuter
//************************************************************************************************

class TransactionExecuter
{
public:
	TransactionExecuter (Transaction* t);								///< takes ownership of transaction
	TransactionExecuter (UndoableTransaction* t, IActionContext* c);	///< takes ownership of transaction
	~TransactionExecuter ();

	void apply (Object* object);

protected:
	Transaction* t;
	int count;
};

} // namespace CCL

#endif // _ccl_transaction_h
