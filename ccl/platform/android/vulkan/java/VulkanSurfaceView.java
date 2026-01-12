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
// Filename    : ccl/platform/android/vulkan/java/VulkanSurfaceView.java
// Description : Vulkan view implemented as a SurfaceView
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.content.Context;
import android.graphics.PixelFormat;
import android.view.*;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

//************************************************************************************************
// VulkanRenderThread
//************************************************************************************************

class VulkanRenderThread extends Thread
{
    protected VulkanSurfaceView surfaceView;

    private boolean isDone = false;
    private boolean hasSurface = false;
    private boolean waitingForSurface = true;
	private boolean paused = true;

    private int width = 0;
    private int height = 0;

    public VulkanRenderThread (VulkanSurfaceView surfaceView)
	{
        super ();

        this.surfaceView = surfaceView;
    }

    @Override
    public void run ()
	{
		setName ("VulkanRenderThread " + getId ());

		// loop until asked to quit
		try
		{
			while(!isDone)
			{
				// update asynchronous state (window size)
				int w, h;
				synchronized(this)
				{
					while(needToWait ())
					{
						if(!hasSurface)
						{
							if(!waitingForSurface)
							{
								waitingForSurface = true;
								notify ();
							}
						}
						wait ();
					}

					if(isDone)
						break;

					w = width;
					h = height;

					if(hasSurface && waitingForSurface)
						waitingForSurface = false;
				}
				
				// draw a frame
				if(w > 0 && h > 0 && !paused)
					surfaceView.onRender ();
			}
		}
		catch(InterruptedException e)
		{}
    }

    private boolean needToWait ()
	{
        if(isDone)
            return false;

        if(!hasSurface || paused)
            return true;

        if(width > 0 && height > 0)
            return false;

        return true;
    }

    public void surfaceCreated ()
	{
        synchronized(this)
		{
            hasSurface = true;
            notify ();
        }
    }

    public void surfaceDestroyed ()
	{
        synchronized(this)
		{
            hasSurface = false;
            notify ();

            while(!waitingForSurface && isAlive ())
			{
                try
				{
                    wait ();
                }
				catch(InterruptedException e)
				{
                    Thread.currentThread ().interrupt ();
                }
            }
        }
    }

    public void surfaceResized (int w, int h)
	{
        synchronized(this)
		{
            width = w;
            height = h;
            notify ();
        }
    }

	public void pauseRendering ()
	{
	    synchronized(this)
		{
            paused = true;
        }
	}
	
	public void resumeRendering ()
	{
	    synchronized(this)
		{
            paused = false;
            notify ();
        }
	}

    public void requestExitAndWait ()
	{
        synchronized(this)
		{
            isDone = true;
            notify ();
        }

        try
		{
            join ();
        }
		catch(InterruptedException e)
		{
            Thread.currentThread ().interrupt ();
        }
    }

	public boolean hasSurface ()
	{
        synchronized(this)
		{
            return hasSurface;
        }
	}
}

//************************************************************************************************
// VulkanSurfaceView
//************************************************************************************************

@Keep
public class VulkanSurfaceView extends SurfaceView
							   implements SurfaceHolder.Callback2
{
	private final long nativeViewPtr;

	private VulkanRenderThread renderThread = null;
	private boolean isAttached = false;
	private boolean startRequested = false;

	private int width = 0;
	private int height = 0;

	public VulkanSurfaceView (long nativeViewPtr, Context context)
	{
		super (context);

		this.nativeViewPtr = nativeViewPtr;

		setZOrderOnTop (true);
		setAlpha (1.f);

		getHolder ().setFormat (PixelFormat.TRANSPARENT);
		getHolder ().addCallback (this);
	}

	@Override
    protected void onAttachedToWindow ()
	{
		super.onAttachedToWindow ();

		if(!isAttached)
		{
			// start render thread
			renderThread = new VulkanRenderThread (this);
			renderThread.start ();

			if(startRequested)
				startRendering ();

			isAttached = true;
		}
    }
	
    @Override
    protected void onDetachedFromWindow ()
	{
        if(renderThread != null)
        	renderThread.requestExitAndWait ();

        isAttached = false;
        super.onDetachedFromWindow ();
    }
	
    public void startRendering ()
	{
		if(renderThread != null)
		{
			renderThread.resumeRendering ();

			startRequested = false;
		}
		else
			startRequested = true;
    }

    public void stopRendering ()
	{
		if(renderThread != null)
			renderThread.pauseRendering ();

		startRequested = false;
    }

    public boolean isAlive ()
	{
		if(renderThread == null)
			return false;

		return renderThread.hasSurface ();
    }

	// forward surface callbacks to render thread

	@Override
	public void surfaceCreated (@NonNull SurfaceHolder holder)
	{
		renderThread.surfaceCreated ();

		onSurfaceCreatedNative (nativeViewPtr, getHolder ().getSurface ());
		if(width > 0 && height > 0)
			onSurfaceResizedNative (nativeViewPtr, width, height);
	}

	@Override
	public void surfaceDestroyed (@NonNull SurfaceHolder holder)
	{
		renderThread.surfaceDestroyed ();

		onSurfaceDestroyedNative (nativeViewPtr);
	}

	@Override
	public void surfaceChanged (@NonNull SurfaceHolder holder, int format, int w, int h)
	{
		width = w;
		height = h;

		renderThread.surfaceResized (width, height);

		onSurfaceResizedNative (nativeViewPtr, width, height);
	}

	@Override
	public void surfaceRedrawNeeded (@NonNull SurfaceHolder holder)
	{}

	// forward render callback to native code

	public void onRender ()
	{
		onRenderNative (nativeViewPtr);
	}

	public native void onSurfaceCreatedNative (long nativeViewPtr, Surface surface);
	public native void onSurfaceDestroyedNative (long nativeViewPtr);
	public native void onSurfaceResizedNative (long nativeViewPtr, int width, int height);
	public native void onRenderNative (long nativeViewPtr);
}
