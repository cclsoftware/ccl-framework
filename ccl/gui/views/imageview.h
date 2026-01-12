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
// Filename    : ccl/gui/views/imageview.h
// Description : Image View class
//
//************************************************************************************************

#ifndef _ccl_imageview_h
#define _ccl_imageview_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/framework/iusercontrol.h" // IBackgroundView
#include "ccl/gui/theme/visualstyleclass.h"

namespace CCL {

interface IParameter;
interface IImageProvider;

//************************************************************************************************
// ImageView
//************************************************************************************************

class ImageView: public View,
				 public IBackgroundView
{
public:
	DECLARE_CLASS (ImageView, View)

	ImageView (IImage* background = nullptr, const Rect& size = Rect (),
			   StyleRef style = 0, StringRef title = nullptr);
	~ImageView ();

	DECLARE_STYLEDEF (customStyles)

	IImage* getBackground () const;
	void setBackground (IImage* image);
	void setImageProvider (IImageProvider* provider);
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)
	
	IParameter* getSelectParam () const;
	void setSelectParam (IParameter* selectParam);

	PROPERTY_SHARED_AUTO (IDataTarget, dataTarget, DataTarget)

	// IBackgroundView
	// Used by CompositedRenderer to draw control backgrounds
	tbool CCL_API canDrawControlBackground () const override;
	void CCL_API drawControlBackground (IGraphics& graphics, RectRef r, PointRef offset) override;

	// View
	IGraphicsLayer* CCL_API getParentLayer (Point& offset) const override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void draw (const UpdateRgn& updateRgn) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	LayerHint CCL_API getLayerHint () const override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const CCL::ColorSchemeEvent& event) override;

	PROPERTY_FLAG (privateFlags, kHasBackgroundFromVisualStyle, hasBackgroundFromVisualStyle)

	CLASS_INTERFACE (IBackgroundView, View)

protected:
	SharedPtr<IImage> background;
	AutoPtr<IGraphicsLayer> backgroundLayer;
	IParameter* selectParam;
	IImageProvider* imageProvider;
	Color imageContrastTransparentColor;
	Color imageContrastBrightColor;
	Color imageContextColor;
	Color imageColor;
	Color imageColorOn;
	Color alphaBlendColor;
	float brightColorThreshold;
	int frame;
	int border;
	float imageFillSize;
	bool useModifiedImage;
	bool drawAsTemplate;
	bool initialized;

	class InsertDataDragHandler;
	class SelectMouseHandler;

	struct ModeSelector
	{
		ImageMode mode;
		ImageMode* modePtr;
		ModeSelector (const ImageView& imageView);
	};

	enum PrivateFlags
	{
		kHasBackgroundFromVisualStyle = 1<<(kLastPrivateFlag + 1)
	};

	void initialize ();
	void determineFrameIndex ();
	bool hasModifyBackgroundColor (Color& contextColor) const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

DECLARE_VISUALSTYLE_CLASS (ImageView)

} // namespace CCL

#endif // _ccl_form_h
