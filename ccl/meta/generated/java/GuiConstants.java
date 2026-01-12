
package dev.ccl.cclgui;

/**
 * CCL GUI constants.
 */
public class GuiConstants
{
	// ***********************************************************************************************
	// Source: AnimationTimingType
	// Animation timing types
	// ***********************************************************************************************

	/** FourCC ('linr'), constant speed from start to end. */
	public static final int kAnimationTimingTypeLinear = 1818848882;
	/** FourCC ('togg'), a single abrupt jump from start to end in the middle of the duration. */
	public static final int kAnimationTimingTypeToggle = 1953458023;
	/** FourCC ('eain'), slower start. */
	public static final int kAnimationTimingTypeEaseIn = 1700882798;
	/** FourCC ('eaou'), slower end. */
	public static final int kAnimationTimingTypeEaseOut = 1700884341;
	/** FourCC ('eaio'), slower start and end. */
	public static final int kAnimationTimingTypeEaseInOut = 1700882799;
	/** FourCC ('cubb'), cubic bezier timing. */
	public static final int kAnimationTimingTypeCubicBezier = 1668637282;

	// ***********************************************************************************************
	// Source: OrientationType
	// GUI orientation types
	// ***********************************************************************************************

	/** Orientation not specified. */
	public static final int kOrientationTypeAnyOrientation = 0;
	/** The horizontal edge is longer. */
	public static final int kOrientationTypeLandscape = 1;
	/** The vertical edge is longer. */
	public static final int kOrientationTypePortrait = 2;

	// ***********************************************************************************************
	// Source: OrientationFlags
	// GUI orientation flags
	// ***********************************************************************************************

	public static final int kOrientationFlagLandscapeAllowed = 1<<kOrientationTypeLandscape;
	public static final int kOrientationFlagPortraitAllowed = 1<<kOrientationTypePortrait;

}