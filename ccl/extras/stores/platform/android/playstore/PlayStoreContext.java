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
// Filename    : ccl/extras/stores/platform/android/playstore/PlayStoreContext.java
// Description : Google Play Store Context for Google Play Billing API
//
//************************************************************************************************

package dev.ccl.cclextras.stores;

import dev.ccl.cclgui.FrameworkActivity;

import androidx.annotation.Keep;

import com.android.billingclient.api.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

//************************************************************************************************
// PlayStoreStateListener
//************************************************************************************************

class PlayStoreStateListener implements BillingClientStateListener
{
    private PlayStoreContext context;
    private long nativeOperation;

    public PlayStoreStateListener (PlayStoreContext context, long nativeOperation)
    {
        this.context = context;
        this.nativeOperation = nativeOperation;
    }

    @Override
    public void onBillingSetupFinished(BillingResult billingResult)
    {
        if(nativeOperation != 0)
            PlayStoreContext.onSetupFinishedNative (nativeOperation, billingResult.getResponseCode ());

        nativeOperation = 0;
    }

    @Override
    public void onBillingServiceDisconnected ()
    {
        // try to restart the connection after 10 seconds
        TimerTask task = new TimerTask ()
        {
            @Override
            public void run ()
            {
                context.connect (0);
            }
        };

        new Timer ().schedule (task, 10000);
    }
}

//************************************************************************************************
// PlayStoreContext
//************************************************************************************************

@Keep
public class PlayStoreContext
{
    private BillingClient billingClient;

	public PlayStoreContext ()
	{
		PendingPurchasesParams.Builder paramsBuilder = PendingPurchasesParams.newBuilder ();

		paramsBuilder.enableOneTimeProducts ();
		
        FrameworkActivity activity = FrameworkActivity.getActivity ();
        BillingClient.Builder clientBuilder = BillingClient.newBuilder (activity);

        clientBuilder.setListener (purchasesUpdatedListener);
        clientBuilder.enablePendingPurchases (paramsBuilder.build ());
		clientBuilder.enableAutoServiceReconnection ();

        billingClient = clientBuilder.build ();
	}

    public void connect (final long nativeOperation)
    {
        try
        {
            billingClient.startConnection (new PlayStoreStateListener (this, nativeOperation));
        }
        catch (IllegalStateException exc)
        {}
    }

	public void terminate ()
	{
        try
        {
            billingClient.endConnection ();
        }
        catch (IllegalStateException exc)
        {}
	}

    private List<QueryProductDetailsParams.Product> productListFromProductIds (String[] productIds)
    {
        List<QueryProductDetailsParams.Product> productList = new ArrayList<QueryProductDetailsParams.Product> ();

        for(String productId : productIds)
        {
            QueryProductDetailsParams.Product.Builder productBuilder = QueryProductDetailsParams.Product.newBuilder ();

            productBuilder.setProductType (BillingClient.ProductType.INAPP);
            productBuilder.setProductId (productId);

            productList.add (productBuilder.build ());
        }

        return productList;
    }

    private List<BillingFlowParams.ProductDetailsParams> productDetailsParamsListFromProductDetailsList (List<ProductDetails> productDetailsList)
    {
        List<BillingFlowParams.ProductDetailsParams> productDetailsParamsList = new ArrayList<BillingFlowParams.ProductDetailsParams> ();

        for(ProductDetails productDetails : productDetailsList)
        {
            BillingFlowParams.ProductDetailsParams.Builder paramsBuilder = BillingFlowParams.ProductDetailsParams.newBuilder ();

            paramsBuilder.setProductDetails (productDetails);

            productDetailsParamsList.add (paramsBuilder.build ());
        }

        return productDetailsParamsList;
    }

    public boolean requestProducts (final long nativeOperation, String[] productIds)
    {
        if(billingClient.getConnectionState () != BillingClient.ConnectionState.CONNECTED)
            return false;

        QueryProductDetailsParams.Builder paramsBuilder = QueryProductDetailsParams.newBuilder ();

        paramsBuilder.setProductList (productListFromProductIds (productIds));

        ProductDetailsResponseListener responseListener = new ProductDetailsResponseListener ()
        {
            @Override
            public void	onProductDetailsResponse (BillingResult billingResult, QueryProductDetailsResult productDetailsResult)
            {
				List<ProductDetails> productDetailsList = productDetailsResult.getProductDetailsList ();

                onRequestProductsCompletedNative (nativeOperation, billingResult.getResponseCode (), productDetailsList != null ? productDetailsList.toArray () : null);
            }
        };

        billingClient.queryProductDetailsAsync (paramsBuilder.build (), responseListener);

        return true;
    }

    public boolean purchaseProduct (String productId)
    {
        if(billingClient.getConnectionState () != BillingClient.ConnectionState.CONNECTED)
            return false;

        QueryProductDetailsParams.Builder paramsBuilder = QueryProductDetailsParams.newBuilder ();

        paramsBuilder.setProductList (productListFromProductIds (new String[] { productId }));

        ProductDetailsResponseListener responseListener = new ProductDetailsResponseListener ()
        {
            @Override
            public void	onProductDetailsResponse (BillingResult billingResult, QueryProductDetailsResult productDetailsResult)
            {
                if(billingResult.getResponseCode () != BillingClient.BillingResponseCode.OK)
                    return;

                BillingFlowParams.Builder paramsBuilder = BillingFlowParams.newBuilder ();
        
                paramsBuilder.setProductDetailsParamsList (productDetailsParamsListFromProductDetailsList (productDetailsResult.getProductDetailsList ()));

                billingClient.launchBillingFlow (FrameworkActivity.getActivity (), paramsBuilder.build ());
            }
        };

        billingClient.queryProductDetailsAsync (paramsBuilder.build (), responseListener);

        return true;
    }

    public boolean queryPurchases (final long nativeOperation)
    {
        if(billingClient.getConnectionState () != BillingClient.ConnectionState.CONNECTED)
            return false;

        PurchasesResponseListener responseListener = new PurchasesResponseListener ()
        {
            @Override
            public void	onQueryPurchasesResponse (BillingResult billingResult, List<Purchase> purchases)
            {
                if(billingResult.getResponseCode () == BillingClient.BillingResponseCode.OK)
                    acknowledgePurchases (purchases);

                onQueryPurchasesCompletedNative (nativeOperation, billingResult.getResponseCode (), purchases != null ? purchases.toArray () : null);
            }
        };

        QueryPurchasesParams.Builder paramsBuilder = QueryPurchasesParams.newBuilder ();

        paramsBuilder.setProductType (BillingClient.ProductType.INAPP);

        billingClient.queryPurchasesAsync (paramsBuilder.build (), responseListener);

        return true;
    }

    private PurchasesUpdatedListener purchasesUpdatedListener = new PurchasesUpdatedListener ()
    {
        @Override
        public void onPurchasesUpdated (BillingResult billingResult, List<Purchase> purchases)
        {
            if(billingResult.getResponseCode () == BillingClient.BillingResponseCode.OK)
                acknowledgePurchases (purchases);

            onPurchasesUpdatedNative (billingResult.getResponseCode (), purchases != null ? purchases.toArray () : null);
        }
    };

    private void acknowledgePurchases (List<Purchase> purchases)
    {
        // acknowledge purchases to finalize them (non-acknowledged purchases will be refunded after 3 days)
        for(Purchase purchase : purchases)  
        {
            if(purchase.getPurchaseState () != Purchase.PurchaseState.PURCHASED || purchase.isAcknowledged ())
                continue;

            AcknowledgePurchaseParams.Builder paramsBuilder = AcknowledgePurchaseParams.newBuilder ();

            paramsBuilder.setPurchaseToken (purchase.getPurchaseToken ());
                    
            AcknowledgePurchaseResponseListener responseListener = new AcknowledgePurchaseResponseListener ()
            {
                @Override
                public void	onAcknowledgePurchaseResponse (BillingResult billingResult) { }
            };

            billingClient.acknowledgePurchase (paramsBuilder.build (), responseListener);
        }
    }
    
	public static native void onSetupFinishedNative (long nativeOperation, int billingResponseCode);
	public static native void onRequestProductsCompletedNative (long nativeOperation, int billingResponseCode, Object[] productDetailsList);
	public static native void onQueryPurchasesCompletedNative (long nativeOperation, int billingResponseCode, Object[] purchases);
	public static native void onPurchasesUpdatedNative (int billingResponseCode, Object[] purchases);
}
