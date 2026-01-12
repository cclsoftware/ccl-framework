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
// Filename    : ccl/platform/android/gui/java/GraphicsLayerView.java
// Description : Graphics Layer implemented as View
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.view.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.view.animation.*;
import android.animation.TimeInterpolator;
import android.animation.Animator;
import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

//************************************************************************************************
// TransformData
//************************************************************************************************

class TransformData
{
	public float angle;
	public float scaleX;
	public float scaleY;
	public float offsetX;
	public float offsetY;

	// extract from given transform matrix
	public TransformData (float a0, float a1, float b0, float b1, float t0, float t1)
	{
		angle = (float)(Math.atan (a1 / b1) * 180 / Math.PI);
		float sign = (float)Math.atan (-a1 / a0);
		if(angle > 90 && sign > 0)
			angle = (360 - angle);
		else if(angle < 90 && sign < 0)
			angle = (360 - angle);

		scaleX = (float)Math.sqrt (a0*a0 + a1*a1);
		scaleY = (float)Math.sqrt (b0*b0 + b1*b1);
		offsetX = t0;
		offsetY = t1;
	}

	// extract from View state
	public TransformData (View view)
	{
		angle = view.getRotation ();
		scaleX = view.getScaleX ();
		scaleY = view.getScaleY ();
		offsetX = view.getTranslationX ();
		offsetY = view.getTranslationY ();
	}

	public void apply (View view)
	{
		view.setRotation (angle);
		view.setScaleX (scaleX);
		view.setScaleY (scaleY);
		view.setTranslationX (offsetX);
		view.setTranslationY (offsetY);
	}

	public void apply (ViewPropertyAnimator animator)
	{
		animator.rotation (angle);
		animator.scaleX (scaleX);
		animator.scaleY (scaleY);
		animator.translationX (offsetX);
		animator.translationY (offsetY);
	}
}

//************************************************************************************************
// AutoReverseInterpolator
//************************************************************************************************

class AutoReverseInterpolator implements TimeInterpolator
{
	private final TimeInterpolator interpolator;

    public AutoReverseInterpolator (TimeInterpolator interpolator)
	{
		this.interpolator = interpolator;
    }
    
    public float getInterpolation (float input)
	{
		// divide the 0..1 interval into a forward and a backward pass of the original interpolator
		float scaledInput = (input <= 0.5f) ? input * 2 : (1 - input) * 2;
        return interpolator.getInterpolation (scaledInput);
    }
}

//************************************************************************************************
// LayerAnimation
//************************************************************************************************

class LayerAnimation implements Animator.AnimatorListener
{
	protected GraphicsLayerView view = null;
	private TimeInterpolator interpolator = null;
	private long duration = 0;
	private int repeatCount = 1;
	private boolean autoReverse = false;
	private long nativeListenerPtr = 0;

	public void run (GraphicsLayerView view, long duration, int timing, int repeatCount, boolean autoReverse, long nativeListenerPtr)
	{
		this.view = view;
		this.repeatCount = repeatCount;
		this.autoReverse = autoReverse;
		this.nativeListenerPtr = nativeListenerPtr;
		
		if(autoReverse)
		{
			this.duration = duration * 2;
			this.interpolator = new AutoReverseInterpolator (createInterpolator (timing));
		}
		else
		{
			this.duration = duration;
			this.interpolator = createInterpolator (timing);

			if(repeatCount > 1)
				saveViewState (); // for restarting from initial state
		}

		start ();
	}

	private TimeInterpolator createInterpolator (int timing)
	{
		if(timing == GuiConstants.kAnimationTimingTypeLinear)
			return new LinearInterpolator ();
		else if(timing == GuiConstants.kAnimationTimingTypeToggle)
		{
			Path path = new Path ();
			path.lineTo (0.5f, 0f);
			path.moveTo (0.5f, 1f);
			path.lineTo (1f, 1f);
			return new PathInterpolator (path);
		}
		else if(timing == GuiConstants.kAnimationTimingTypeEaseIn)
			return new AccelerateInterpolator ();
		else if(timing == GuiConstants.kAnimationTimingTypeEaseOut)
			return new DecelerateInterpolator ();
		else if(timing == GuiConstants.kAnimationTimingTypeEaseInOut)
			return new AccelerateDecelerateInterpolator ();
		else
			return new LinearInterpolator ();
	}

	private void start ()
	{
		final ViewPropertyAnimator animator = view.animate ();
		animator.setDuration (duration);
		animator.setInterpolator (interpolator);
		animator.setListener (this);

		setupAnimation (animator);

		animator.start ();
	}

	/// to be implemented by derived classes:
	protected void setupAnimation (ViewPropertyAnimator animator) {}
	protected void saveViewState () {}
	protected void restoreViewState () {}

	public void onAnimationStart (@NonNull Animator animation)
	{}

	public void onAnimationRepeat (@NonNull Animator animation)
	{}

	public void onAnimationEnd (@NonNull Animator animation)
	{
		//CCL.log ("onAnimationEnd, repeatCount "+  repeatCount);
		if(repeatCount != 0xFFFF) // kRepeatForever
			repeatCount--;

		if(repeatCount > 0)
		{
			if(!autoReverse)
				restoreViewState ();

			start ();
		}
		else
		{
			if(nativeListenerPtr != 0)
			{
				onAnimationEndNative (nativeListenerPtr);
				nativeListenerPtr = 0;
			}
		}
	}

	public void onAnimationCancel (@NonNull Animator animation)
	{}

	public native void onAnimationEndNative (long nativeListenerPtr);
}


//************************************************************************************************
// BasicAnimation
//************************************************************************************************

class BasicAnimation extends LayerAnimation
{
	protected float value = 0;
	protected float savedValue = 0;
}

//************************************************************************************************
// AlphaAnimation
//************************************************************************************************

class AlphaAnimation extends BasicAnimation
{
	public AlphaAnimation (float value)								{ this.value = value; }

	protected void setupAnimation (ViewPropertyAnimator animator)	{ animator.alpha (value); }
	protected void saveViewState ()									{ savedValue = view.getAlpha (); }
	protected void restoreViewState ()								{ view.setAlpha (savedValue); }
}

//************************************************************************************************
// OffsetXAnimation
//************************************************************************************************

class OffsetXAnimation extends BasicAnimation
{
	OffsetXAnimation (float value)									{ this.value = value; }

	protected void setupAnimation (ViewPropertyAnimator animator)	{ animator.x (value); }
	protected void saveViewState ()									{ savedValue = view.getX (); }
	protected void restoreViewState ()								{ view.setX (savedValue); }
}

//************************************************************************************************
// OffsetYAnimation
//************************************************************************************************

class OffsetYAnimation extends BasicAnimation
{
	OffsetYAnimation (float value)									{ this.value = value; }

	protected void setupAnimation (ViewPropertyAnimator animator)	{ animator.y (value); }
	protected void saveViewState ()									{ savedValue = view.getY (); }
	protected void restoreViewState ()								{ view.setY (savedValue); }
}

//************************************************************************************************
// OffsetAnimation
//************************************************************************************************

class OffsetAnimation extends LayerAnimation
{
	protected float x;
	protected float y;
	protected float savedX;
	protected float savedY;
	
	OffsetAnimation (int x, int y)									{ this.x = x; this.y = y; }

	protected void setupAnimation (ViewPropertyAnimator animator)	{ animator.x (x).y (y); }
	protected void saveViewState ()									{ savedX = view.getY (); savedY = view.getY (); }
	protected void restoreViewState ()								{ view.setX (savedX); view.setY (savedY); }
}

//************************************************************************************************
// TransformAnimation
//************************************************************************************************

class TransformAnimation extends LayerAnimation
{
	protected TransformData transform;
	protected TransformData savedTransform;

	TransformAnimation (float a0, float a1, float b0, float b1, float t0, float t1)
	{
		transform = new TransformData (a0, a1, b0, b1, t0, t1);
	}

	protected void setupAnimation (ViewPropertyAnimator animator)	{ transform.apply (animator);	}
	protected void saveViewState ()									{ savedTransform = new TransformData (view); }
	protected void restoreViewState ()								{ savedTransform.apply (view); }
}

//************************************************************************************************
// GraphicsLayerView
//************************************************************************************************

@Keep
public class GraphicsLayerView extends ViewGroup
							   implements SurfaceHolder.Callback,
							   TextureView.SurfaceTextureListener 
{
	private final long nativeLayerPtr;
	private final FrameworkGraphics graphics;

	public int width = 0;
	public int height = 0;
	public boolean opaque = false;
	public boolean isSprite = false;

	private SurfaceView surfaceView = null;
	private TextureView textureView = null;
	
	private static final boolean useSurface = false;
	private static final boolean useTexture = false;

	private static int maxHardwareLayerWidth = -1;
	private static int maxHardwareLayerHeight = -1;

	public static void checkHardwareLayerSupport (Canvas canvas)
	{
		if(maxHardwareLayerWidth < 0)
		{
			maxHardwareLayerWidth = canvas.getMaximumBitmapWidth ();
			maxHardwareLayerHeight = canvas.getMaximumBitmapHeight ();
			//CCL.log ("max. hardware layer size: " + maxHardwareLayerWidth + " x " + maxHardwareLayerHeight);
		}
	}

	private static boolean isHardwareLayerSupported (int width, int height)
	{
		if(maxHardwareLayerWidth < 0)
			return width <= 8192 && height <= 8192;
		else
			return width <= maxHardwareLayerWidth && height <= maxHardwareLayerHeight;
	}

	public GraphicsLayerView (long nativeLayerPtr, Context context, int width, int height)
	{
		super (context);

		this.nativeLayerPtr = nativeLayerPtr;
		this.graphics = new FrameworkGraphics ();

		if(useSurface)
		{
			surfaceView = new SurfaceView (context);
			surfaceView.setZOrderOnTop (true);
			surfaceView.getHolder ().setFormat (PixelFormat.TRANSPARENT);
			surfaceView.getHolder ().addCallback (this);
			addView (surfaceView);
		}
		else if(useTexture)
		{
			textureView = new TextureView (context);
			textureView.setSurfaceTextureListener (this);
			textureView.setOpaque (false);
			addView (textureView);
		}
		else
		{
			if(isHardwareLayerSupported (width, height))
				setLayerType (LAYER_TYPE_HARDWARE, null);
			//else CCL.log ("NO LAYER: " + width + ", " +height);

			setWillNotDraw (false);
		}

		setSize (width, height);

		setPivotX (0);
		setPivotY (0);

		onViewCreatedNative (nativeLayerPtr, graphics);
    }

    @Override
    public void surfaceCreated (@NonNull SurfaceHolder holder)
    {}
 
    @Override
    public void surfaceChanged (@NonNull SurfaceHolder holder, int format, int width, int height)
    {
		redrawSurface (holder);
    }
	
    @Override
    public void surfaceDestroyed (@NonNull SurfaceHolder holder)
    {}

	void redrawSurface (SurfaceHolder holder)
	{
		synchronized(holder)
		{
			if(holder.getSurface ().isValid ())
			{
				final Canvas canvas = holder.lockCanvas ();
				if(canvas != null)
				{
					graphics.setCanvas (canvas);
					redrawNative (nativeLayerPtr);
					graphics.setCanvas (null);

					holder.unlockCanvasAndPost (canvas);
				}
			}
		}
	}

	void redrawTexture ()
	{
		if(textureView != null)
		{
			final Canvas canvas = textureView.lockCanvas ();
			if(canvas != null)
			{
				graphics.setCanvas (canvas);
				redrawNative (nativeLayerPtr);
				graphics.setCanvas (null);

				textureView.unlockCanvasAndPost (canvas);
			}
		}
	}

	@Override
	public void onSurfaceTextureAvailable (@NonNull SurfaceTexture texture, int width, int height)
	{
		redrawTexture ();
	}

	@Override
	public void onSurfaceTextureSizeChanged (@NonNull SurfaceTexture texture, int width, int height)
	{
		redrawTexture ();
	}

	@Override
	public boolean onSurfaceTextureDestroyed (@NonNull SurfaceTexture texture)
	{
		return true;
	}

	@Override
	public void onSurfaceTextureUpdated (@NonNull SurfaceTexture texture)
	{}

	public void setUpdateNeeded ()
	{
		if(surfaceView != null)
			redrawSurface (surfaceView.getHolder ());
		else if(textureView != null)
			redrawTexture ();
		else
			invalidate ();
	}

	public void setBackground (Bitmap bitmap)
	{
		BitmapDrawable drawable = new BitmapDrawable (getContext ().getResources (), bitmap);
		drawable.setGravity (Gravity.LEFT|Gravity.TOP); // don't stretch
		setBackground (drawable);
	}

	public void setSize (int width, int height)
	{
		if(!isHardwareLayerSupported (width, height))
		{
			setLayerType (LAYER_TYPE_NONE, null);
			//CCL.log ("setSize: layer too large" + width + ", " + height);
		}

		this.width = width;
		this.height = height;
		layout (0, 0, width, height);

		if(surfaceView != null)
			surfaceView.layout (0, 0, width, height);
		else if(textureView != null)
			textureView.layout (0, 0, width, height);
	}

	public void setMode (boolean ignoreAlpha, boolean clipToBounds)
	{
		// CCL layer mode flags
		opaque = ignoreAlpha && surfaceView != null;
		setClipChildren (clipToBounds);
	}

	@Override
	public boolean isOpaque ()
	{
		return opaque;
	}

	@Override
	public boolean hasOverlappingRendering ()
	{
        return false;
    }

	@Override
    protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec)
	{
		setMeasuredDimension (width, height);
    }

	@Override
	protected void onLayout (boolean changed, int l, int t, int r, int b)
	{
		// don't layout sublayer views
	}

	@Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
	{
		//CCL.log ("layerView.onSizeChanged: " + w + ", " + h + "(old " + oldw +", " + oldh +")" + " now: " + getX () + ", " + getY () + "(" + getWidth () +", " + getHeight () +")");
        super.onSizeChanged (w, h, oldw, oldh);
	}

	@Override
    protected void onDraw (@NonNull Canvas canvas) 
	{
		//CCL.log ("layerView.onDraw: " + getX () + ", " + getY () + "(" + getWidth () +", " + getHeight () +")");
		super.onDraw (canvas);

/*		Paint fillPaint = new Paint ();
		fillPaint.setStyle (Paint.Style.FILL);
		fillPaint.setColor (Color.GREEN);
		canvas.drawRect (0, 0, getWidth (), getHeight (), fillPaint);*/

		if(surfaceView != null || textureView != null)
			return;
			
		graphics.setCanvas (canvas);
		redrawNative (nativeLayerPtr);
		graphics.setCanvas (null);
	}

	public void setTransform (float a0, float a1, float b0, float b1, float t0, float t1)
	{
		TransformData transform = new TransformData (a0, a1, b0, b1, t0, t1);
		transform.apply (this);
	}

	public void addAlphaAnimation (long duration, int timing, int repeatCount, boolean autoReverse, long nativeListener, float alpha)
	{
		LayerAnimation animation = new AlphaAnimation (alpha);
		animation.run (this, duration, timing, repeatCount, autoReverse, nativeListener);
	}

	public void addOffsetXAnimation (long duration, int timing, int repeatCount, boolean autoReverse, long nativeListener, float offset)
	{
		LayerAnimation animation = new OffsetXAnimation (offset);
		animation.run (this, duration, timing, repeatCount, autoReverse, nativeListener);
	}

	public void addOffsetYAnimation (long duration, int timing, int repeatCount, boolean autoReverse, long nativeListener, float offset)
	{
		LayerAnimation animation = new OffsetYAnimation (offset);
		animation.run (this, duration, timing, repeatCount, autoReverse, nativeListener);
	}

	public void addOffsetAnimation (long duration, int timing, int repeatCount, boolean autoReverse, long nativeListener, int offsetX, int offsetY)
	{
		LayerAnimation animation = new OffsetAnimation (offsetX, offsetY);
		animation.run (this, duration, timing, repeatCount, autoReverse, nativeListener);
	}

	public void addTransformAnimation (long duration, int timing, int repeatCount, boolean autoReverse, long nativeListener, float a0, float a1, float b0, float b1, float t0, float t1)
	{
		LayerAnimation animation = new TransformAnimation (a0, a1, b0, b1, t0, t1);
		animation.run (this, duration, timing, repeatCount, autoReverse, nativeListener);
	}

	public void removeAnimation ()
	{
		animate ().cancel ();
	}

	public native void onViewCreatedNative (long nativeLayerPtr, FrameworkGraphics graphics);
    public native void redrawNative (long nativeLayerPtr);
}
