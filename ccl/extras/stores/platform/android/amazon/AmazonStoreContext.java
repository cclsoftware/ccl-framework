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
// Filename    : ccl/extras/stores/platform/android/amazon/AmazonStoreContext.java
// Description : Store Context for Amazon AppStore SDK
//
//************************************************************************************************

package dev.ccl.cclextras.stores;

import dev.ccl.cclgui.FrameworkActivity;

import androidx.annotation.Keep;

import com.amazon.device.iap.*;
import com.amazon.device.iap.model.*;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

//************************************************************************************************
// AmazonStoreContext
//************************************************************************************************

@Keep
public class AmazonStoreContext implements PurchasingListener
{
    private Map<RequestId, Long> pendingRequests = new HashMap<RequestId, Long> ();

	public AmazonStoreContext ()
	{
        FrameworkActivity activity = FrameworkActivity.getActivity ();

        PurchasingService.registerListener (activity, this);
        PurchasingService.enablePendingPurchases ();
    }

    public String requestProducts (final long nativeOperation, String[] productList)
    {
        RequestId request = PurchasingService.getProductData (new HashSet<String> (Arrays.asList (productList)));

        pendingRequests.put (request, nativeOperation);

        return request.toString ();
    }

    public String purchaseProduct (final long nativeOperation, String productId)
    {
        RequestId request = PurchasingService.purchase (productId);

        pendingRequests.put (request, nativeOperation);

        return request.toString ();
    }

    public String queryPurchases (final long nativeOperation)
    {
        RequestId request = PurchasingService.getPurchaseUpdates (true);

        pendingRequests.put (request, nativeOperation);

        return request.toString ();
    }
    
    @Override
    public void onProductDataResponse (ProductDataResponse productDataResponse)
    {
        RequestId request = productDataResponse.getRequestId ();
        ProductDataResponse.RequestStatus requestStatus = productDataResponse.getRequestStatus ();

        long nativeOperation = pendingRequests.get (request);

        Map<String, Product> productData = productDataResponse.getProductData ();

        onRequestProductsCompletedNative (nativeOperation, requestStatus.ordinal (), productData.values ().toArray ());
    }

    @Override
    public void onPurchaseResponse (PurchaseResponse purchaseResponse)
    {
        RequestId request = purchaseResponse.getRequestId ();
        PurchaseResponse.RequestStatus requestStatus = purchaseResponse.getRequestStatus ();

        long nativeOperation = pendingRequests.get (request);

        Receipt receipt = purchaseResponse.getReceipt ();

        onPurchaseCompletedNative (nativeOperation, requestStatus.ordinal (), receipt);

        if(requestStatus == PurchaseResponse.RequestStatus.SUCCESSFUL && !receipt.isCanceled ())
            PurchasingService.notifyFulfillment (receipt.getReceiptId (), FulfillmentResult.FULFILLED);
    }

    @Override
    public void onPurchaseUpdatesResponse (PurchaseUpdatesResponse purchaseUpdatesResponse)
    {
        RequestId request = purchaseUpdatesResponse.getRequestId ();
        PurchaseUpdatesResponse.RequestStatus requestStatus = purchaseUpdatesResponse.getRequestStatus ();

        long nativeOperation = pendingRequests.get (request);

        List<Receipt> receipts = purchaseUpdatesResponse.getReceipts ();

        onQueryPurchasesCompletedNative (nativeOperation, requestStatus.ordinal (), receipts.toArray ());

        if(requestStatus == PurchaseUpdatesResponse.RequestStatus.SUCCESSFUL)
            for(Receipt receipt : receipts)
                if(!receipt.isCanceled ())
                    PurchasingService.notifyFulfillment (receipt.getReceiptId (), FulfillmentResult.FULFILLED);
    }

    @Override
    public void onUserDataResponse (UserDataResponse userDataResponse) {}

    public static native void onRequestProductsCompletedNative (long nativeOperation, int requestStatus, Object[] products);
	public static native void onPurchaseCompletedNative (long nativeOperation, int requestStatus, Object receipt);
	public static native void onQueryPurchasesCompletedNative (long nativeOperation, int requestStatus, Object[] receipts);
}
