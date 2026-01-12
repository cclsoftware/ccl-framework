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
// Filename    : ccl/app/controls/itemviewmodel.h
// Description : Item View Model
//
//************************************************************************************************

#ifndef _ccl_itemviewmodel_h
#define _ccl_itemviewmodel_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

interface IParameter;
interface IAsyncOperation;
interface IColorPalette;
class AbstractTouchHandler;

//************************************************************************************************
// ItemModelPainter
//************************************************************************************************

class ItemModelPainter
{
public:
	typedef const IItemModel::DrawInfo& DrawInfoRef;

	constexpr static const int kDefaultTextTrimMode = Font::kTrimModeRight;

	void drawIcon (DrawInfoRef info, IImage* icon, bool enabled = true, bool fitImage = true, int margin = 0);
	void drawIconWithOverlay (DrawInfoRef info, IImage* icon, IImage* overlay = nullptr, bool enabled = true, bool fitImage = true, int margin = 0);
	void drawOverlay (DrawInfoRef info, RectRef iconRect, IImage* overlay);
	void drawButtonImage (DrawInfoRef info, IImage* image, bool pressed, bool enabled = true);
	
	void calcCheckBoxRect (Rect& checkRect, DrawInfoRef info, AlignmentRef alignment = Alignment::kHCenter);
	void drawCheckBox (DrawInfoRef info, bool checked, bool enabled = true, AlignmentRef alignment = Alignment::kHCenter);

	void calcButtonRect (Rect& buttonRect, DrawInfoRef info, StringRef title, Coord verticalMargin = 0);
	void drawButton (DrawInfoRef info, StringRef title, bool enabled = true, Coord verticalMargin = 0);
	
	void drawSelectBoxArrow (DrawInfoRef info, bool enabled = true, int margin = 0);

	SolidBrush getTextBrush (DrawInfoRef info, bool enabled);
	void drawTitle (DrawInfoRef info, StringRef title, bool enabled = true, int fontStyle = 0, 
					AlignmentRef alignment = Alignment::kLeft|Alignment::kVCenter, int trimMode = kDefaultTextTrimMode);
	void drawText (DrawInfoRef info, StringRef text, AlignmentRef alignment = Alignment::kCenter, bool enabled = true, int fontStyle = 0, Coord margin = 0);

	void calcTitleRects (Rect& titleRect, Rect& subTitleRect, DrawInfoRef info, Coord spacing);
	void drawTitleWithSubtitle (DrawInfoRef info, StringRef title, StringRef subTitle, bool enabled = true, int fontStyle = 0,
								Coord lineSpacing = 4,
								int trimMode = kDefaultTextTrimMode);

	void drawVerticalBar (IGraphics& graphics, RectRef rect, float value, Color backColor, Color hiliteColor, Coord margin = 1);
	void drawHorizontalBar (IGraphics& graphics, RectRef rect, float value, Color backColor, Color hiliteColor, Coord margin = 1);
};

//************************************************************************************************
// ItemModel
//************************************************************************************************

class ItemModel: public Object,
				 public ItemViewObserver<AbstractItemModel>,
				 protected ItemModelPainter
{
public:
	DECLARE_CLASS (ItemModel, Object)
	DECLARE_METHOD_NAMES (ItemModel)

	CLASS_INTERFACE (IItemModel, Object)

	/** Helper methods related to attached ItemView */
	void invalidate ();
	void invalidateItem (ItemIndexRef index);
	void updateColumns ();

	/** Helpers for showing an edit control. */
	IAsyncOperation* editString (StringRef initialValue, RectRef rect, const EditInfo& info, IVisualStyle* visualStyle = nullptr);
	IAsyncOperation* editString (StringRef initialValue, RectRef rect, IItemView* view = nullptr, IVisualStyle* visualStyle = nullptr);
	IAsyncOperation* editValue (IParameter* param, const EditInfo& info, IVisualStyle* visualStyle = nullptr); // edit box or value box
	IAsyncOperation* startEditOperation (IParameter* param, ViewBox& editControl, const EditInfo& info, IVisualStyle* visualStyle = nullptr);
	IVisualStyle* createEditStyle ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	class BoxedEditInfo;
	class ItemVisitor;

	DECLARE_STRINGID_MEMBER (kSwipeEditDone)   

protected:
	/** Helpers to popup a parameter menu / slider in editCell(). */
	bool doPopup (IParameter* param, const EditInfo& info, IVisualStyle* visualStyle = nullptr);
	bool doPopupSlider (IParameter* param, const EditInfo& info, PointRef position, bool horizontal = false, StringID popupSliderDecorForm = nullptr);
	bool doPopupColorPalette (Color& color, IColorPalette* palette, const EditInfo& info);
	bool setEditControl (ViewBox& editControl, const EditInfo& info);

	// Helper to perform a swipe operation over a range of items
	enum class SwipeMethod { kMultiple, kSingle }; // multiple: include all items between mouse down and current pos, single: only item at current pos
	bool swipeItems (IView* itemView, const MouseEvent& mouseEvent, ItemVisitor* itemVisitor, SwipeMethod method = SwipeMethod::kMultiple);
	template<typename Lambda> bool swipeItems (IView* itemView, const MouseEvent& mouseEvent, Lambda visitItem);
	template<typename Lambda> bool swipeItemsSingle (IView* itemView, const MouseEvent& mouseEvent, Lambda visitItem);
	template<typename Lambda> class LambdaItemVisitor;
	class SwipeItemsMouseHandler;

	AbstractTouchHandler* wrapMouseHandler (IMouseHandler* mouseHandler, IView* view);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	class EditControlOperation;
	class TouchMouseHandler;

private:
	IAsyncOperation* editString (StringRef initialValue, RectRef rect, const EditInfo* info, IItemView* view, IVisualStyle* visualStyle);
	IAsyncOperation* startEditOperation (IParameter* param, ViewBox& editControl, const EditInfo* info, IItemView* itemView, IVisualStyle* visualStyle);
};

//************************************************************************************************
// ItemModel::BoxedEditInfo
//************************************************************************************************

class ItemModel::BoxedEditInfo: public Object,
								public EditInfo
{
public:
	DECLARE_CLASS_ABSTRACT (BoxedEditInfo, Object)
	DECLARE_PROPERTY_NAMES (BoxedEditInfo)

	BoxedEditInfo (const EditInfo& editInfo)
	: EditInfo (editInfo)
	{}

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// ItemModel::ItemVisitor
//************************************************************************************************

class ItemModel::ItemVisitor: public Unknown
{
public:
	virtual void visit (ItemIndexRef index) const = 0; ///< return true to stop traversal
};

//************************************************************************************************
// ListViewModelBase::LambdaItemVisitor
//************************************************************************************************

template<typename Lambda>
class ItemModel::LambdaItemVisitor: public ItemVisitor
{
public:
	LambdaItemVisitor (const Lambda& visitItem)
	: visitItem (visitItem)
	{}

	// ItemVisitor
	void visit (ItemIndexRef index) const override { visitItem (index); }

private:
	Lambda visitItem;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Lambda>
inline bool ItemModel::swipeItems (IView* itemView, const MouseEvent& mouseEvent, Lambda visitItem)
{ return swipeItems (itemView, mouseEvent, static_cast<ItemVisitor*> (NEW LambdaItemVisitor<Lambda> (visitItem))); }

template<typename Lambda>
inline bool ItemModel::swipeItemsSingle (IView* itemView, const MouseEvent& mouseEvent, Lambda visitItem)
{ return swipeItems (itemView, mouseEvent, static_cast<ItemVisitor*> (NEW LambdaItemVisitor<Lambda> (visitItem)), SwipeMethod::kSingle); }

} // namespace CCL

#endif // _ccl_itemviewmodel_h
