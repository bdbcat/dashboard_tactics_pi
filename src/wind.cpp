/******************************************************************************
 * $Id: wind.cpp, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#include <cmath>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "wx/tokenzr.h"

#ifndef __DERIVEDTIMEOUT_OVERRIDE__
#define __DERIVEDTIMEOUT_OVERRIDE__
#endif // __DERIVEDTIMEOUT_OVERRIDE__
#include "wind.h"

#include "dashboard_pi_ext.h"

// Display the arrow for MainValue (wind angle)
// We also want the extra value (wind speed) displayed inside the dial

DashboardInstrument_Wind::DashboardInstrument_Wind( wxWindow *parent, wxWindowID id, wxString title,
                                                    unsigned long long cap_flag
                                                    ) :
      DashboardInstrument_Dial( parent, id, title, cap_flag, 0, 360, 0, 360)
{
      SetOptionMarker(10, DIAL_MARKER_REDGREENBAR, 3);
      // Labels are set static because we've no logic to display them this way
      wxString labels[] = {_T(""), _T("30"), _T("60"), _T("90"), _T("120"), _T("150"), _T(""), _T("150"), _T("120"), _T("90"), _T("60"), _T("30")};
      SetOptionLabel(30, DIAL_LABEL_HORIZONTAL, wxArrayString(12, labels));
}

void DashboardInstrument_Wind::DrawBackground(wxGCDC* dc)
{
    DrawBoat( dc, m_cx, m_cy, m_radius );
}

DashboardInstrument_WindCompass::DashboardInstrument_WindCompass( wxWindow *parent, wxWindowID id, wxString title,
                                                                  unsigned long long cap_flag
    ) :
      DashboardInstrument_Dial( parent, id, title, cap_flag, 0, 360, 0, 360 )
{
      SetOptionMarker(5, DIAL_MARKER_SIMPLE, 2);
      wxString labels[] = {_("N"), _("NE"), _("E"), _("SE"), _("S"), _("SW"), _("W"), _("NW")};
      SetOptionLabel(45, DIAL_LABEL_HORIZONTAL, wxArrayString(8, labels));
}

void DashboardInstrument_WindCompass::DrawBackground(wxGCDC* dc)
{
      DrawCompassRose(dc, m_cx, m_cy, m_radius * 0.85, m_AngleStart, false);
}

// Display the arrow for MainValue (wind angle)
// We also want the extra value (wind speed) displayed inside the dial

DashboardInstrument_TrueWindAngle::DashboardInstrument_TrueWindAngle( wxWindow *parent, wxWindowID id, wxString title,
                                                                      unsigned long long cap_flag
    ) :
    DashboardInstrument_Dial( parent, id, title, cap_flag, 0, 360, 0, 360)
{
      SetOptionMarker(10, DIAL_MARKER_REDGREENBAR, 3);
      // Labels are set static because we've no logic to display them this way
      wxString labels[] = {_T(""), _T("30"), _T("60"), _T("90"), _T("120"), _T("150"), _T(""), _T("150"), _T("120"), _T("90"), _T("60"), _T("30")};
      SetOptionLabel(30, DIAL_LABEL_HORIZONTAL, wxArrayString(12, labels));
}

void DashboardInstrument_TrueWindAngle::DrawBackground(wxGCDC* dc)
{
    DrawBoat( dc, m_cx, m_cy, m_radius );
}

/*****************************************************************************
  Apparent & True wind angle combined in one dial instrument
  Author: Thomas Rauch
******************************************************************************/
DashboardInstrument_AppTrueWindAngle::DashboardInstrument_AppTrueWindAngle(wxWindow *parent, wxWindowID id, wxString title,
                                                                           unsigned long long cap_flag
    ) :
    DashboardInstrument_Dial(parent, id, title, cap_flag, 0, 360, 0, 360)
{
	SetOptionMarker(10, DIAL_MARKER_REDGREENBAR, 3);
	// Labels are set static because we've no logic to display them this way
	wxString labels[] = { _T(""), _T("30"), _T("60"), _T("90"), _T("120"), _T("150"), _T(""), _T("150"), _T("120"), _T("90"), _T("60"), _T("30") };
	SetOptionLabel(30, DIAL_LABEL_HORIZONTAL, wxArrayString(12, labels));
	m_MainValueApp = std::nan("1");
	m_MainValueTrue = std::nan("1");
	m_ExtraValueApp = std::nan("1");
	m_ExtraValueTrue = std::nan("1");
	m_TWD = std::nan("1");
	m_TWDUnit = _T("");
    m_MainValueOption1 = DIAL_POSITION_NONE;
    m_MainValueOption2 = DIAL_POSITION_NONE;
    m_ExtraValueOption1 = DIAL_POSITION_NONE;
    m_ExtraValueOption2 = DIAL_POSITION_NONE;
}

void DashboardInstrument_AppTrueWindAngle::DrawBackground(wxGCDC* dc)
{
	DrawBoat(dc, m_cx, m_cy, m_radius);
}

void DashboardInstrument_AppTrueWindAngle::SetData(
    unsigned long long st,
    double data, wxString unit
    , long long timestamp
    )
{ 
    if ( std::isnan( data ) ) {
        if ( st == OCPN_DBP_STC_AWS ) {
            m_MainValueApp  = std::nan("1");
            m_MainValueAppUnit = _T("");
            m_ExtraValueApp = std::nan("1");
            m_MainValueAppUnit = _T("");
            Refresh();
        } // perhaps AWS watchdog has hit, reset both AWA and AWS.
        else if ( st == OCPN_DBP_STC_TWS ) {
            m_MainValueTrue  = std::nan("1");
            m_MainValueTrueUnit = _T("");
            m_ExtraValueTrue = std::nan("1");
            m_ExtraValueTrueUnit = _T("");
            Refresh();
        } // perhaps TWS watchdog has hit, reset both TWA and TWS.
        return;
    } // else invalid data received

    setTimestamp( timestamp );

    if ( (unit == _T("\u00B0l")) || (unit == _T("\u00B0lr")) ) {
        unit = DEGREE_SIGN + L"\u2192";
    }
    else if ( (unit == _T("\u00B0r")) || (unit == _T("\u00B0rl")) ) {
        unit = DEGREE_SIGN + L"\u2190";
    }
    else if (unit == _T("\u00B0u")){
        unit = DEGREE_SIGN + L"\u2191";
    }
    else if (unit == _T("\u00B0d")){
        unit = DEGREE_SIGN + L"\u2193";
    }

    if (st == OCPN_DBP_STC_TWA){
        m_MainValueTrue = data;
        m_MainValueTrueUnit = unit;
        m_MainValueOption2 = DIAL_POSITION_BOTTOMLEFT;
    }
    else if (st == OCPN_DBP_STC_AWA){
        m_MainValueApp = data;
        m_MainValueAppUnit = unit;
        m_MainValueOption1 = DIAL_POSITION_TOPLEFT;
    }
    else if ( (st == OCPN_DBP_STC_AWS) && (data < 200.0) ){
        m_ExtraValueApp = data;
        m_ExtraValueAppUnit = unit;
        m_ExtraValueOption1 = DIAL_POSITION_TOPRIGHT;
    }
    else if ( (st == OCPN_DBP_STC_TWS) && (data < 200.0) ){
        m_ExtraValueTrue = data;
        m_ExtraValueTrueUnit = unit;
        m_ExtraValueOption2 = DIAL_POSITION_BOTTOMRIGHT;
    }
    else if (st == OCPN_DBP_STC_TWD){
        m_TWD = data;
        m_TWDUnit = unit;
    }
    Refresh();
}

void DashboardInstrument_AppTrueWindAngle::derivedTimeoutEvent()
{

    m_MainValueTrue = std::nan("1");
    m_MainValueTrueUnit = _T("");
    m_MainValueApp = std::nan("1");
    m_MainValueAppUnit = _T("");
    m_ExtraValueTrue = std::nan("1");
    m_ExtraValueTrueUnit = _T("");
    m_ExtraValueApp = std::nan("1");
    m_ExtraValueAppUnit = _T("");
    m_TWD = std::nan("1");
	m_TWDUnit = _T("");
}

void DashboardInstrument_AppTrueWindAngle::Draw(wxGCDC* bdc)
{
	wxColour c1;
	GetGlobalColor( g_sDialColorBackground, &c1 );
	wxBrush b1(c1);
	bdc->SetBackground(b1);
	bdc->Clear();

	wxSize size = GetClientSize();
	m_cx = size.x / 2;
	int availableHeight = size.y - m_TitleHeight - 6;
	int width, height;
	bdc->GetTextExtent(_T("000"), &width, &height, 0, 0, g_pFontLabel);
	m_cy = m_TitleHeight + 2;
	m_cy += availableHeight / 2;
	m_radius = availableHeight / 2.0 * 0.95;


	DrawLabels(bdc);
	DrawFrame(bdc);
	DrawMarkers(bdc);
	DrawBackground(bdc);
	DrawData(bdc, m_MainValueApp, m_MainValueAppUnit, m_MainValueFormat, m_MainValueOption1);
	DrawData(bdc, m_MainValueTrue, m_MainValueTrueUnit, m_MainValueFormat, m_MainValueOption2);
	DrawData(bdc, m_ExtraValueApp, m_ExtraValueAppUnit, m_ExtraValueFormat, m_ExtraValueOption1);
	DrawData(bdc, m_ExtraValueTrue, m_ExtraValueTrueUnit, m_ExtraValueFormat, m_ExtraValueOption2);
	DrawData(bdc, m_TWD, m_MainValueTrueUnit, _T("TWD:%.0f"), DIAL_POSITION_INSIDE);
	DrawForeground(bdc);
}
void DashboardInstrument_AppTrueWindAngle::DrawForeground(wxGCDC* dc)
{
	double data;
	double val;
	double value;
	// The default foreground is the arrow used in most dials
    DrawNeedleHub( dc, m_cx, m_cy, m_radius,
                   ( !std::isnan( m_ExtraValueTrue) ||  !std::isnan( m_ExtraValueApp) ) ? true : false  );

	/*True Wind*/
	if ( !std::isnan( m_ExtraValueTrue ) ) {  //m_ExtraValueTrue = True Wind Angle; we have a watchdog for TWS; if TWS becomes NAN, TWA must be NAN as well

        if (m_MainValueTrueUnit == (DEGREE_SIGN + L"\u2192") )
            data = 360 - m_MainValueTrue;
        else
            data = m_MainValueTrue;

        // The arrow should stay inside fixed limits
        if (data < m_MainValueMin) val = m_MainValueMin;
        else if (data > m_MainValueMax) val = m_MainValueMax;
        else val = data;

        value = deg2rad((val - m_MainValueMin) * m_AngleRange / (m_MainValueMax - m_MainValueMin)) + deg2rad(m_AngleStart - ANGLE_OFFSET);

        DrawNeedle( dc, m_cx, m_cy, m_radius, value, g_sDialSecondNeedleColor );

    }

	/* Apparent Wind*/
    if ( !std::isnan( m_ExtraValueApp ) ) { //m_ExtraValueApp=AWA; we have a watchdog for AWS; if AWS becomes NAN, AWA will also be NAN ...

        if (m_MainValueAppUnit == (DEGREE_SIGN + L"\u2192") )
            data = 360 - m_MainValueApp;
        else
            data = m_MainValueApp;

        // The arrow should stay inside fixed limits
        if (data < m_MainValueMin) val = m_MainValueMin;
        else if (data > m_MainValueMax) val = m_MainValueMax;
        else val = data;

        value = deg2rad((val - m_MainValueMin) * m_AngleRange / (m_MainValueMax - m_MainValueMin)) + deg2rad(m_AngleStart - ANGLE_OFFSET);

        DrawNeedle( dc, m_cx, m_cy, m_radius, value, g_sDialNeedleColor );

    }
}
void DashboardInstrument_AppTrueWindAngle::DrawData(wxGCDC* dc, double value,
	wxString unit, wxString format, DialPositionOption position)
{
	if (position == DIAL_POSITION_NONE)
		return;

	dc->SetFont(*g_pFontLabel);
	wxColour cl;
	GetGlobalColor( g_sDialColorForeground, &cl );
	dc->SetTextForeground(cl);

	wxSize size = GetClientSize();

	wxString text;
	if (!std::isnan(value))
	{
        if ( (unit != (DEGREE_SIGN + L"\u2192")) && (unit != (DEGREE_SIGN + L"\u2190")) &&
             (unit != (DEGREE_SIGN + L"\u2191")) && (unit != (DEGREE_SIGN + L"\u2193")) )
        {
            if (unit == _T("\u00B0"))
                text = wxString::Format(format, value) + DEGREE_SIGN;
            else if (unit == _T("\u00B0L")) // No special display for now, might be XXdeg< (as in text-only instrument)
                text = wxString::Format(format, value) + DEGREE_SIGN;
            else if (unit == _T("\u00B0R")) // No special display for now, might be >XXdeg
                text = wxString::Format(format, value) + DEGREE_SIGN;
            else if (unit == _T("\u00B0T"))
                text = wxString::Format(format, value) + DEGREE_SIGN + _T("T");
            else if (unit == _T("\u00B0M"))
                text = wxString::Format(format, value) + DEGREE_SIGN + _T("M");
            else if (unit == _T("N")) // Knots
                text = wxString::Format(format, value) + _T(" Kts");
            else
                text = wxString::Format(format, value) + _T(" ") + unit;
        } // then "unit" value is not set yet for wind SetData()data,
        else {
            text = wxString::Format(format, value) + unit;
        } // else "unit" value has been set for wind SetData()data,
	}
	else
		text = _T("---");

	int width, height;
	dc->GetMultiLineTextExtent(text, &width, &height, NULL, g_pFontLabel);

	wxRect TextPoint;
	TextPoint.width = width;
	TextPoint.height = height;
	wxColour c3;

	switch (position)
	{
	case DIAL_POSITION_NONE:
		// This case was already handled before, it's here just
		// to avoid compiler warning.
		return;
    case DIAL_POSITION_TOPINSIDE:
        return;
	case DIAL_POSITION_INSIDE:
	{
		TextPoint.x = m_cx - (width / 2) - 1;
		TextPoint.y = (size.y * .75) - height;
		GetGlobalColor( g_sDialColorLabel, &cl );
		int penwidth = size.x / 100;
		wxPen* pen = wxThePenList->FindOrCreatePen(cl, penwidth, wxPENSTYLE_SOLID);
		dc->SetPen(*pen);
		GetGlobalColor( g_sDialColorBackground, &cl );
		dc->SetBrush(cl);
		// There might be a background drawn below
		// so we must clear it first.
		dc->DrawRoundedRectangle(TextPoint.x - 2, TextPoint.y - 2, width + 4, height + 4, 3);
		break;
	}
	case DIAL_POSITION_TOPLEFT:
		GetGlobalColor( g_sDialNeedleColor, &c3 );
		TextPoint.x = 0;
		TextPoint.y = m_TitleHeight;
		text = _T("A:") + text;
		break;
	case DIAL_POSITION_TOPRIGHT:
		GetGlobalColor( g_sDialNeedleColor, &c3 );
		TextPoint.x = size.x - width - 1;
		TextPoint.y = m_TitleHeight;
		break;
	case DIAL_POSITION_BOTTOMLEFT:
		GetGlobalColor( g_sDialSecondNeedleColor, &c3 );
		text = _T("T:") + text;
		TextPoint.x = 0;
		TextPoint.y = size.y - height;
		break;
	case DIAL_POSITION_BOTTOMRIGHT:
		GetGlobalColor( g_sDialSecondNeedleColor, &c3 );
		TextPoint.x = size.x - width - 1;
		TextPoint.y = size.y - height;
		break;
	}
	wxColour c2;
	GetGlobalColor( g_sDialColorBackground, &c2 );
	wxStringTokenizer tkz(text, _T("\n"));
	wxString token;

	token = tkz.GetNextToken();
	while (token.Length()) {
		dc->GetTextExtent(token, &width, &height, NULL, NULL, g_pFontLabel);

#ifdef __WXMSW__
		if (g_pFontLabel->GetPointSize() <= 12) {
			wxBitmap tbm(width, height, -1);
			wxMemoryDC tdc(tbm);

			tdc.SetBackground(c2);
			tdc.Clear();
			tdc.SetFont(*g_pFontLabel);
			tdc.SetTextForeground(c3);

			tdc.DrawText(token, 0, 0);
			tdc.SelectObject(wxNullBitmap);

			dc->DrawBitmap(tbm, TextPoint.x, TextPoint.y, false);
		}
		else
#endif
			dc->DrawText(token, TextPoint.x, TextPoint.y);


		TextPoint.y += height;
		token = tkz.GetNextToken();
	}
}
