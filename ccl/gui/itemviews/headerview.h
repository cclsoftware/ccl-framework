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
// Filename    : ccl/gui/itemviews/headerview.h
// Description : Header View
//
//************************************************************************************************

#ifndef _ccl_headerview_h
#define _ccl_headerview_h

#include "ccl/gui/views/view.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class ThemeRenderer;

//************************************************************************************************
// IColumnCalculator
//************************************************************************************************

interface IColumnCalculator: IUnknown
{
	virtual tbool CCL_API calcColumnWidth (int& width, int columnIndex) = 0;
	
	DECLARE_IID (IColumnCalculator)
};

//************************************************************************************************
// ColumnHeader
//************************************************************************************************

class ColumnHeader: public Object
{
public:
	DECLARE_CLASS (ColumnHeader, Object)
		
	ColumnHeader (int width = 0, StringRef title = nullptr, StringID id = nullptr, int minWidth = 0, int flags = 0);
	ColumnHeader (const ColumnHeader& h);

	enum ExtraFlags
	{
		kSortedUp   = 1<<9,
		kSortedDown = 1<<10
	};

	PROPERTY_STRING (title, Title)
	PROPERTY_MUTABLE_CSTRING (columnId, ID)
	PROPERTY_VARIABLE (int, index, Index)
	PROPERTY_VARIABLE (int, width, Width)
	PROPERTY_VARIABLE (int, minWidth, MinWidth)
	PROPERTY_VARIABLE (int, defaultWidth, DefaultWidth)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_OBJECT (Variant, userData, UserData)

	PROPERTY_FLAG (flags, IColumnHeaderList::kSizable,  canResize)
	PROPERTY_FLAG (flags, IColumnHeaderList::kMoveable, canMove)
	PROPERTY_FLAG (flags, IColumnHeaderList::kFill, canFill)
	PROPERTY_FLAG (flags, IColumnHeaderList::kHideable, canHide)
	PROPERTY_FLAG (flags, IColumnHeaderList::kSortable, canSort)
	PROPERTY_FLAG (flags, IColumnHeaderList::kHidden, isHidden)
	PROPERTY_FLAG (flags, IColumnHeaderList::kCanFit, canFit)
	PROPERTY_FLAG (flags, IColumnHeaderList::kEditMode, isEditMode)
	PROPERTY_FLAG (flags, IColumnHeaderList::kCanEditMultiple, canEditMultiple)
	PROPERTY_FLAG (flags, IColumnHeaderList::kCentered, drawCentered)
	PROPERTY_FLAG (flags, kSortedUp, isSortedUp)
	PROPERTY_FLAG (flags, kSortedDown, isSortedDown)
	bool isSorted () const { return isSortedUp () || isSortedDown (); }

	// Object
	int compare (const Object& obj) const override; ///< compares index
};

//************************************************************************************************
// ColumnHeaderList
//************************************************************************************************

class ColumnHeaderList: public Object,
						public IColumnHeaderList
{
public:
	DECLARE_CLASS (ColumnHeaderList, Object)
	DECLARE_METHOD_NAMES (ColumnHeaderList)

	ColumnHeaderList ();

	ObjectArray& getColumns ();
	
	PROPERTY_SHARED_AUTO (IColumnCalculator, columnCalculator, ColumnCalculator)

	int getCount (bool visible) const;
	ColumnHeader* getColumnByIndex (int columnIndex) const;
	ColumnHeader* getColumnAtPosition (int position, bool visible) const;
	ColumnHeader* getColumnWithID (StringID id) const;

	int getFlatPositionFromVisible (int position) const;
	int getVisiblePositionFromFlat (int position) const;

	int getTotalWidth () const;
	void getColumnRange (Coord& left, Coord& right, int columnIndex) const;
	int getColumnIndex (Coord x) const;
	int columnIndexToPosition (int columnIndex, bool visible) const;
	bool canHideColumns () const;
	bool canFitColumns (bool& multiple) const;
	bool canResetColumns () const;

	void setColumnMinWidth (ColumnHeader* column, int minWidth);
	void setColumnWidth (ColumnHeader* column, int width);
	void moveColumn (ColumnHeader* column, int newVisiblePosition);
	void showColumn (ColumnHeader* column, bool state);

	void setSortColumn (StringID id, bool upwards, bool signalNeeded = true);
	StringID getSortColumn (bool& upwards) const;

	void storeState (IAttributeList& a) const;
	void restoreState (const IAttributeList& a);

	// IColumnHeaderList
	void CCL_API addColumn (int width, StringRef title, StringID id = nullptr, int minWidth = 0, int flags = 0) override;
	void CCL_API copyFrom (const IColumnHeaderList& other) override;
	StringID CCL_API getColumnID (int columnIndex) const override;
	void CCL_API removeAll () override;
	void CCL_API setColumnWidth (StringID column, int width) override;
	void CCL_API hideColumn (StringID column, tbool state) override;
	void CCL_API moveColumn (StringID column, int newVisiblePosition) override;
	tbool CCL_API setColumnDataAt (int columnIndex, VariantRef data) override;
	tbool CCL_API getColumnDataAt (Variant& data, int columnIndex) const override;

	CLASS_INTERFACE (IColumnHeaderList, Object)

protected:
	ObjectArray columns;
	ObjectArray indexColumns; // columns in original order

	// IColumnHeaderList
	int CCL_API getColumnCount () const override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// HeaderView
//************************************************************************************************

class HeaderView: public View
{
public:
	DECLARE_CLASS (HeaderView, View)

	HeaderView (ColumnHeaderList* columnList = nullptr);
	~HeaderView ();

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	ColumnHeaderList* getColumnHeaders () const;
	void setColumnHeaders (ColumnHeaderList* list);
	void storeColumnState ();

	bool getColumnRect (Rect& rect, ColumnHeader* column);
	int getVisibleTargetColumnPos (ColumnHeader* column, Coord x); ///< for moving columns

	ThemeRenderer* getRenderer ();

	// View
	void calcAutoSize (Rect& r) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void draw (const UpdateRgn& updateRgn) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	void onSize (const Point& delta) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	AccessibilityProvider* getAccessibilityProvider () override;

protected:
	bool getColumnAction (const MouseEvent& event, ColumnHeader*& column, int& action);
	ColumnHeader* findColumnAt (const Point& where) const;

	ColumnHeaderList* columnList;
	ThemeRenderer* renderer;

	IAttributeList* getViewState (bool create);
	void restoreColumnState ();

	bool onSetupColumn (CmdArgs args, VariantRef data);
	bool onFitColumn (CmdArgs args, VariantRef data);
	bool onResetColumns (CmdArgs args, VariantRef data);
};

//************************************************************************************************
// IHeaderViewRenderer
//************************************************************************************************

interface IHeaderViewRenderer: IUnknown
{
	/** Draw column header. */
	virtual void drawHeader (View* view, GraphicsDevice& port, RectRef r, StringRef label, BrushRef textBrush, FontRef font) = 0;

	DECLARE_IID (IHeaderViewRenderer)
};

} // namespace CCL

#endif // _ccl_headerview_h
