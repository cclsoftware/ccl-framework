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
// Filename    : ccl/extras/stores/platform/android/samsung/SamsungStoreContext.java
// Description : Store Context for Samsung In-App Purchase SDK
//
//************************************************************************************************

package dev.ccl.cclextras.stores;

import dev.ccl.cclgui.FrameworkActivity;

import androidx.annotation.Keep;

import com.samsung.android.sdk.iap.lib.constants.*;
import com.samsung.android.sdk.iap.lib.helper.*;
import com.samsung.android.sdk.iap.lib.listener.*;
import com.samsung.android.sdk.iap.lib.vo.*;

import java.util.ArrayList;

//************************************************************************************************
// SamsungStoreContext
//************************************************************************************************

@Keep
public class SamsungStoreContext
{
    private IapHelper iapHelper;

	public SamsungStoreContext (int mode)
	{
        FrameworkActivity activity = FrameworkActivity.getActivity ();

        iapHelper = IapHelper.getInstance (activity);

        for(HelperDefine.OperationMode operationMode : HelperDefine.OperationMode.values ())
            if(operationMode.getValue () == mode)
                iapHelper.setOperationMode (operationMode);
    }

    public void requestProducts (final long nativeOperation, String[] productList)
    {
        String products = String.join (",", productList);

        OnGetProductsDetailsListener responseListener = new OnGetProductsDetailsListener ()
        {
            @Override
            public void	onGetProducts (ErrorVo error, ArrayList<ProductVo> productList)
            {
                onRequestProductsCompletedNative (nativeOperation, error.getErrorCode (), productList != null ? productList.toArray () : null);
            }
        };

        iapHelper.getProductsDetails (products, responseListener);
    }

    public boolean purchaseProduct (final long nativeOperation, String productId)
    {
        OnPaymentListener responseListener = new OnPaymentListener ()
        {
            @Override
            public void	onPayment (ErrorVo error, PurchaseVo purchase)
            {
                onPurchaseCompletedNative (nativeOperation, error.getErrorCode (), purchase);

				if(error.getErrorCode () == IapHelper.IAP_ERROR_NONE)
					acknowledgePurchases (new String[]{ purchase.getPurchaseId () });
            }
        };

        return iapHelper.startPayment (productId, responseListener);
    }

    public boolean queryPurchases (final long nativeOperation)
    {
        OnGetOwnedListListener responseListener = new OnGetOwnedListListener ()
        {
            @Override
            public void	onGetOwnedProducts (ErrorVo error, ArrayList<OwnedProductVo> ownedList)
            {
                onQueryPurchasesCompletedNative (nativeOperation, error.getErrorCode (), ownedList != null ? ownedList.toArray () : null);
				
				if(error.getErrorCode () == IapHelper.IAP_ERROR_NONE)
				{
					ArrayList<String> purchases = new ArrayList<String> ();
					for(OwnedProductVo product : ownedList)
						if(product.getAcknowledgedStatus () == HelperDefine.AcknowledgedStatus.NOT_ACKNOWLEDGED)
							purchases.add (product.getPurchaseId ());
							
					if(!purchases.isEmpty ())
						acknowledgePurchases (purchases.toArray (new String[0]));
				}
            }
        };

        return iapHelper.getOwnedList (HelperDefine.PRODUCT_TYPE_ITEM, responseListener);
    }

    public boolean acknowledgePurchases (String[] purchaseList)
    {
        String purchases = String.join (",", purchaseList);

        OnAcknowledgePurchasesListener responseListener = new OnAcknowledgePurchasesListener ()
        {
            @Override
            public void onAcknowledgePurchases (ErrorVo error, ArrayList<AcknowledgeVo> acknowledgedList)
            {}
        };

        return iapHelper.acknowledgePurchases (purchases, responseListener);
    }
    
    public static native void onRequestProductsCompletedNative (long nativeOperation, int errorCode, Object[] products);
	public static native void onPurchaseCompletedNative (long nativeOperation, int errorCode, Object purchase);
	public static native void onQueryPurchasesCompletedNative (long nativeOperation, int errorCode, Object[] ownedProducts);
}
