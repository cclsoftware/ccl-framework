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
// Filename    : ccl/extras/stores/platform/cocoa/storekitmanager.cocoa.mm
// Description : Mac/iOS App Store Manager based on StoreKit
//
//************************************************************************************************

#define DEBUG_LOG 0

#import <StoreKit/StoreKit.h>

#include "ccl/extras/stores/platformstoremanager.h"
#include "ccl/extras/stores/platform/cocoa/applestorereceipt.cocoa.h"

#include "ccl/app/component.h"
#include "ccl/base/message.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;

//************************************************************************************************
// StoreObserver
//************************************************************************************************

class TransactionAsyncOperation;

@interface StoreObserver : NSObject<SKPaymentTransactionObserver>
{
	TransactionAsyncOperation* asyncOperation;
}

+ (StoreObserver*)sharedInstance;
- (instancetype)initWithOperation:(TransactionAsyncOperation*)operation;

@end

//************************************************************************************************
// StoreDelegate
//************************************************************************************************

class ProductRequestAsyncOperation;

@interface StoreDelegate : NSObject<SKProductsRequestDelegate>
{
	ProductRequestAsyncOperation* asyncOperation;
}

@property (strong) SKProductsRequest* productRequest;

- (instancetype)initWithOperation:(ProductRequestAsyncOperation*)operation;
- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response;
- (void)fetchProductsMatchingIdentifiers:(NSArray*)identifiers;

@end

//************************************************************************************************
// TransactionAsyncOperation
//************************************************************************************************

class TransactionAsyncOperation: public AsyncOperation
{
public:
	TransactionAsyncOperation ();
	~TransactionAsyncOperation ();

	String getTransactionID () { return transactionID; }

	void triggerPurchaseProduct (SKProduct* product);
	void triggerRestorePurchases ();
	void setState (AsyncOperation::State state) override;

private:
	String transactionID = "";
	StoreObserver* observer = nil;
};

//************************************************************************************************
// ProductRequestAsyncOperation
//************************************************************************************************

class ProductRequestAsyncOperation: public AsyncOperation
{
public:
	ProductRequestAsyncOperation ()
	{
		delegate = [[StoreDelegate alloc] initWithOperation:this];
	}

	~ProductRequestAsyncOperation ()
	{
		if(delegate)
			[delegate release];
	}

	void triggerFetchProductsMatchingIdentifiers (NSArray* identifiers)
	{
		[delegate fetchProductsMatchingIdentifiers:identifiers];
	}

private:
	StoreDelegate* delegate = nil;
};

//************************************************************************************************
// StoreKitManager
//************************************************************************************************

class StoreKitManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (StoreKitManager, PlatformStoreManager)

	StoreKitManager ();
	~StoreKitManager ();

    void resetPendingRestore ();
    
	// PlatformStoreManager
	StringID getID () const override;
	IAsyncOperation* startup () override;
	void shutdown () override;
	void setAvailableProducts (NSArray<SKProduct*>* products);
	ObjectArray* getStoreProducts () { return &storeProducts; }
	IAsyncOperation* requestProducts (const ConstVector<String>& productIds) override;
	IAsyncOperation* purchaseProduct (StringRef productId) override;
	IAsyncOperation* getTransactions () override;
	IAsyncOperation* getLocalLicenses () override;
	IAsyncOperation* restorePurchases () override;

protected:
	bool restoreInProgress = false;
	NSArray<SKProduct*>* availableProducts;
	ObjectArray transactions;
	ObjectArray storeProducts;

	// Indicates whether the user is allowed to make payments.
	static bool canMakePayments ();
	SKProduct* getProduct (NSString* identifier);
};


//************************************************************************************************
// StoreObserver
//************************************************************************************************

@implementation StoreObserver;

+ (StoreObserver*)sharedInstance
{
	static dispatch_once_t onceToken;
	static StoreObserver* storeObserverSharedInstance;

	dispatch_once (&onceToken, ^{
		storeObserverSharedInstance = [[StoreObserver alloc] initWithOperation:nil];
	});

	return storeObserverSharedInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (instancetype)initWithOperation:(TransactionAsyncOperation*)operation
{
	self = [super init];

	if(self != nil)
		asyncOperation = operation;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions
{
	if(asyncOperation == nullptr)
		return;

	StoreKitManager::instance ().deferSignal (NEW Message (PlatformStoreManager::kTransactionsChanged));

	void(^work) (void) = ^
	{
		for(SKPaymentTransaction* transaction in transactions)
		{
			switch(transaction.transactionState)
			{
			case SKPaymentTransactionStatePurchasing:
				break;
			case SKPaymentTransactionStateDeferred: CCL_PRINT ("Deferred");
				break;
			case SKPaymentTransactionStatePurchased: [self handlePurchasedTransaction:transaction];
				break;
			case SKPaymentTransactionStateFailed: [self handleFailedTransaction:transaction];
				break;
			case SKPaymentTransactionStateRestored: [self handleRestoredTransaction:transaction];
				break;
			default:
				break;
			}
		}
	};
	
	// call on main thread for signal handler
	if([NSThread isMainThread] == YES)
		work ();
	else
		dispatch_sync (dispatch_get_main_queue (), work);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)paymentQueue:(SKPaymentQueue*)queue removedTransactions:(NSArray*)transactions
{
	for(SKPaymentTransaction* transaction in transactions)
		CCL_PRINTF ("%s Removed", transaction.payment.productIdentifier);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error
{
	if(asyncOperation && asyncOperation->getTransactionID () == "")
	{
		void(^work) (void) = ^
		{
			// keep alive during completion handling
			SharedPtr<AsyncOperation> saver (asyncOperation);
			if(error.code != SKErrorPaymentCancelled)
				asyncOperation->setState (AsyncOperation::kFailed);
			else
				asyncOperation->setState (AsyncOperation::kCanceled);
		};

		// call on main thread for signal handler
		if([NSThread isMainThread] == YES)
			work ();
		else
			dispatch_sync (dispatch_get_main_queue (), work);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue
{
	if(asyncOperation && asyncOperation->getTransactionID () == "")
	{
		void(^work) (void) = ^
		{
			// keep alive during completion handling
			SharedPtr<AsyncOperation> saver (asyncOperation);
			asyncOperation->setResult (true);
			StoreKitManager::instance ().deferSignal (NEW Message (PlatformStoreManager::kLocalLicensesChanged));
			asyncOperation->setState (AsyncOperation::kCompleted);
		};

		// call on main thread for signal handler
		if([NSThread isMainThread] == YES)
			work ();
		else
			dispatch_sync (dispatch_get_main_queue (), work);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

-(void)handlePurchasedTransaction:(SKPaymentTransaction*)transaction
{
	String productID;
	productID.appendNativeString ([[transaction payment] productIdentifier]);
	if(asyncOperation && asyncOperation->getTransactionID () == productID)
	{
		@try
		{
			[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
		}
		@catch(NSException* exception)
		{
			ASSERT (false)
		}

		// keep alive during completion handling
		SharedPtr<AsyncOperation> saver (asyncOperation);
		asyncOperation->setResult (true);
		StoreKitManager::instance ().deferSignal (NEW Message (PlatformStoreManager::kLocalLicensesChanged));
		asyncOperation->setState (AsyncOperation::kCompleted);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

-(void)handleFailedTransaction:(SKPaymentTransaction*)transaction
{
	if(transaction.error)
	{
		CCL_PRINTF ("Error: %s", transaction.error.localizedDescription);
	}

	String productID;
	productID.appendNativeString ([[transaction payment] productIdentifier]);
	if(asyncOperation && asyncOperation->getTransactionID () == productID)
	{
		@try
		{
			[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
		}
		@catch(NSException* exception)
		{
			ASSERT (false)
		}

		// keep alive during completion handling
		SharedPtr<AsyncOperation> saver (asyncOperation);
		asyncOperation->setState (AsyncOperation::kFailed);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

-(void)handleRestoredTransaction:(SKPaymentTransaction*)transaction
{
	CCL_PRINTF ("Restore content for %s.", transaction.payment.productIdentifier);
	@try
	{
		[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
		StoreKitManager::instance ().deferSignal (NEW Message (PlatformStoreManager::kLocalLicensesChanged));
	}
	@catch(NSException* exception)
	{
		ASSERT (false)
	}
}

@end

//************************************************************************************************
// StoreDelegate
//************************************************************************************************

@implementation StoreDelegate;

- (instancetype)initWithOperation:(ProductRequestAsyncOperation*)operation
{
	self = [super init];

	if(self != nil)
		asyncOperation = operation;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)fetchProductsMatchingIdentifiers:(NSArray*)identifiers
{
	NSSet* productIdentifiers = [NSSet setWithArray:identifiers];

	self.productRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
	self.productRequest.delegate = self;

	[self.productRequest start];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response
{
	NSArray<SKProduct*>* responseProducts = [NSArray arrayWithArray:response.products];
	StoreKitManager* manager = ccl_cast<StoreKitManager> (&StoreKitManager::instance ());
	if(manager)
		manager->setAvailableProducts (responseProducts);

	if(asyncOperation)
	{
		if(manager)
		{
			Variant result (manager->getStoreProducts ()->asUnknown ());
			void(^work) (void) = ^
			{
				// keep alive during completion handling
				SharedPtr<AsyncOperation> saver (asyncOperation);
				asyncOperation->setResult (result);
				asyncOperation->setState (AsyncOperation::kCompleted);
			};
			// call on main thread for signal handler
			if([NSThread isMainThread] == YES)
				work ();
			else
				dispatch_sync (dispatch_get_main_queue (), work);
		}
		else
			asyncOperation->setState (AsyncOperation::kFailed);
	}

	self.productRequest = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)request:(SKRequest*)request didFailWithError:(NSError*)error
{
	if(asyncOperation)
	{
		void(^work) (void) = ^
		{
			SharedPtr<AsyncOperation> saver (asyncOperation);
			asyncOperation->setState (AsyncOperation::kFailed);
		};

		// call on main thread for signal handler
		if([NSThread isMainThread] == YES)
			work ();
		else
			dispatch_sync (dispatch_get_main_queue (), work);
	}
}

@end

//************************************************************************************************
// TransactionAsyncOperation
//************************************************************************************************

TransactionAsyncOperation::TransactionAsyncOperation ()
{
	observer = [[StoreObserver alloc] initWithOperation:this];
	if(observer)
		[[SKPaymentQueue defaultQueue] addTransactionObserver:observer];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransactionAsyncOperation::~TransactionAsyncOperation ()
{
	if(observer)
	{
		[[SKPaymentQueue defaultQueue] removeTransactionObserver:observer];
		[observer release];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransactionAsyncOperation::triggerPurchaseProduct (SKProduct* product)
{
	if(observer == nil)
		setState (kFailed);

	String productID;
	productID.appendNativeString ([product productIdentifier]);
	transactionID = productID;

	@try
	{
		SKPayment* payment = [SKPayment paymentWithProduct:product];
		[[SKPaymentQueue defaultQueue] addPayment:payment];
	}
	@catch(NSException* exception)
	{
		setState (kFailed);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransactionAsyncOperation::triggerRestorePurchases ()
{
	if(observer == nil)
		setState (kFailed);

	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransactionAsyncOperation::setState (AsyncOperation::State state)
{
	AsyncOperation::setState (state);
	if(transactionID == "" && (state == AsyncOperation::kFailed || state == AsyncOperation::kCanceled || state == AsyncOperation::kCompleted))
	{
		if(StoreKitManager* manager = ccl_cast<StoreKitManager> (&StoreKitManager::instance ()))
			manager->resetPendingRestore ();
	}
}

//************************************************************************************************
// StoreKitManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreKitManager, PlatformStoreManager)
#if !CCL_DEMO_STORE_MANAGER_ENABLED
DEFINE_EXTERNAL_SINGLETON (PlatformStoreManager, StoreKitManager)
#endif

bool StoreKitManager::canMakePayments ()
{
	return [SKPaymentQueue canMakePayments];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreKitManager::StoreKitManager ()
: availableProducts (nil)
{
	transactions.objectCleanup ();
	storeProducts.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreKitManager::~StoreKitManager ()
{
	if(availableProducts)
		[availableProducts release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID StoreKitManager::getID () const
{
	#if CCL_PLATFORM_MAC
		return PlatformStoreID::kAppleAppStoreMacOS;
	#else
		return PlatformStoreID::kAppleAppStoreIOS;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreKitManager::resetPendingRestore ()
{
	restoreInProgress = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::startup ()
{
	[[SKPaymentQueue defaultQueue] addTransactionObserver:[StoreObserver sharedInstance]];
	
	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreKitManager::shutdown ()
{
	[[SKPaymentQueue defaultQueue] removeTransactionObserver:[StoreObserver sharedInstance]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreKitManager::setAvailableProducts (NSArray<SKProduct*>* products)
{
	if(availableProducts)
		[availableProducts release];

	availableProducts = [products retain];

	storeProducts.removeAll ();
	for(SKProduct* product in availableProducts)
	{
		String productID;
		productID.appendNativeString ([product productIdentifier]);

		AutoPtr<StoreProduct> p = NEW StoreProduct ();
		p->setID (productID);

		String productName;
		productName.appendNativeString ([product localizedTitle]);
		p->setName (productName);

		String productPrice;
		NSNumberFormatter* formatter = [[[NSNumberFormatter alloc] init] autorelease];
		[formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
		[formatter setLocale: [product priceLocale]];
		productPrice.appendNativeString ([formatter stringFromNumber:[product price]]);
		p->setPrice (productPrice);

		storeProducts.add (p.detach ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::requestProducts (const ConstVector<String>& productIds)
{
	if(!canMakePayments ())
		return AsyncOperation::createFailed ();
	
	NSMutableArray* identifiers = [[NSMutableArray alloc] initWithCapacity:0];
	for(StringRef s : productIds)
		[identifiers addObject:[s.createNativeString<NSString*> () autorelease]];
	
	ProductRequestAsyncOperation* result = NEW ProductRequestAsyncOperation ();
	result->triggerFetchProductsMatchingIdentifiers (identifiers);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::purchaseProduct (StringRef productId)
{
	if(!canMakePayments ())
		return AsyncOperation::createFailed ();
	
	NSString* productIdString = [productId.createNativeString<NSString*> () autorelease];
	SKProduct* product = getProduct (productIdString);
	if(product == nil)
		return AsyncOperation::createFailed ();
	
	TransactionAsyncOperation* result = NEW TransactionAsyncOperation ();
	result->triggerPurchaseProduct (product);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::getTransactions ()
{
	NSArray<SKPaymentTransaction*>* skTransactions = [[SKPaymentQueue defaultQueue] transactions];
	transactions.removeAll ();
	for(SKPaymentTransaction* transaction in skTransactions)
	{
		AutoPtr<StoreTransaction> t = NEW StoreTransaction ();
		SKPaymentTransactionState state = [transaction transactionState];
		if(state == SKPaymentTransactionStatePurchasing)
			t->setState (PurchaseState::kInProgress);
		else if(state == SKPaymentTransactionStateDeferred)
			t->setState (PurchaseState::kDeferred);
		else if(state == SKPaymentTransactionStatePurchased || state == SKPaymentTransactionStateRestored)
		{
			t->setState (PurchaseState::kCompleted);
			@try
			{
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
			}
			@catch(NSException* exception)
			{
				ASSERT (false)
			}
		}
		else if(state == SKPaymentTransactionStateFailed)
		{
			t->setState (PurchaseState::kFailed);
			@try
			{
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
			}
			@catch(NSException* exception)
			{
				ASSERT (false)
			}
		}
		else
			continue;
		
		String productID;
		productID.appendNativeString ([[transaction payment] productIdentifier]);
		t->setProductID (productID);
		
		String transactionID;
		transactionID.appendNativeString ([transaction transactionIdentifier]);
		t->setTransactionID (transactionID);

		transactions.add (t.detach ());
	}
	
	return AsyncOperation::createCompleted (Variant (transactions));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::getLocalLicenses ()
{
	AutoPtr<ObjectArray> licenses = NEW ObjectArray;
	licenses->objectCleanup (true);
	#if DEBUG && 0
	AppleStoreReceipt receipt ([[NSBundle mainBundle] URLForResource:@"receipt" withExtension:@""]);
	#else
	AppleStoreReceipt receipt;
	#endif
	receipt.getPurchases (*licenses);
	
	return AsyncOperation::createCompleted (Variant (licenses->asUnknown (), true));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* StoreKitManager::restorePurchases ()
{
	if(restoreInProgress)
		return nullptr;
	
	restoreInProgress = true;
	TransactionAsyncOperation* result = NEW TransactionAsyncOperation ();
	result->triggerRestorePurchases ();
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SKProduct* StoreKitManager::getProduct (NSString* identifier)
{
	if(availableProducts == nil)
		return nil;
	
	for(SKProduct* product : availableProducts)
	{
		if([product.productIdentifier isEqualToString:identifier])
			return product;
	}
	
	return nil;
}
