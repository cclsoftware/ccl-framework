
package dev.ccl.cclgui;

/**
 * CCL GUI constants.
 */
public class GuiConstants
{
	/** Identifier for desktop application type. */
	public static final String kApplicationTypeDesktopID = "desktopapp";

	/** Identifier for mobile application type. */
	public static final String kApplicationTypeMobileID = "mobileapp";

	// ***********************************************************************************************
	// Source: ApplicationType
	// ***********************************************************************************************

	/** Desktop application. */
	public static final int kApplicationTypeDesktop = 0;
	/** Mobile application. */
	public static final int kApplicationTypeMobile = 1;

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
	// Source: DragSessionDropResult
	// Drag session drop results
	// ***********************************************************************************************

	/** No effect. */
	public static final int kDragSessionDropResultNone = 0;
	/** Drop causes a copy that share the data. */
	public static final int kDragSessionDropResultCopyShared = 1<<0;
	/** Drop causes a complete copy. */
	public static final int kDragSessionDropResultCopyReal = 1<<1;
	/** Drop causes move. */
	public static final int kDragSessionDropResultMove = 1<<2;

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

	// ***********************************************************************************************
	// Source: TextInputFlags
	// Text input flags
	// ***********************************************************************************************

	public static final int kTextInputFlagSensitiveData = 1<<0;
	public static final int kTextInputFlagMultiline = 1<<1;

}