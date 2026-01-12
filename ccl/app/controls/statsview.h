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
// Filename    : ccl/app/controls/statsview.h
// Description : Statistics Graphing View
//
//************************************************************************************************

#ifndef _statsview_h
#define _statsview_h

#include "ccl/app/controls/usercontrol.h"

namespace CCL {

interface IStatisticsProvider;

//************************************************************************************************
// StatsGraphView
//************************************************************************************************

class StatsGraphView: public UserControl
{
public:
	DECLARE_CLASS (StatsGraphView, UserControl)
	
	StatsGraphView (IStatisticsProvider* provider = nullptr, RectRef size = Rect (), StringRef category = String::kEmpty);
	~StatsGraphView ();
	
	void setCategory (StringRef category);
	
	// UserControl
	void attached (IView* parent) override;
	void draw (const DrawEvent& event) override;	
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	SharedPtr<IStatisticsProvider> provider;
	String category;
	Color lineColor;
	Color brushColor;
	Color backColor;
};

} // namespace CCL

#endif // _statsview_h

