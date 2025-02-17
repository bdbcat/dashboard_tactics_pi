/**************************************************************************
* $Id: PolarPerformance.cpp, v1.0 2016/06/07 tom_BigSpeedy Exp $
*
* Project:  OpenCPN
* Purpose:  tactics Plugin
* Author:   Thomas Rauch
***************************************************************************
*   Copyright (C) 2010 by David S. Register                               *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
***************************************************************************
*/
#include <map>
#include <cmath>
using namespace std;

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "dashboard_pi.h"

#include "TacticsEnums.h"
#include "TacticsFunctions.h"
#include "PerformanceSingle.h"
#include "Polar.h"
#include "PolarPerformance.h"

#include "dashboard_pi_ext.h"
#include "tactics_pi_ext.h"
#include "plugin_ids.h"

//*************************************************************************
// Polar Performance instrument
//*************************************************************************

#define SETDRAWSOLOINPANE true
wxBEGIN_EVENT_TABLE (TacticsInstrument_PolarPerformance, DashboardInstrument)
   EVT_TIMER (myID_THREAD_POLARPERFORMANCE, TacticsInstrument_PolarPerformance::OnPolarPerfUpdTimer)
wxEND_EVENT_TABLE ()

TacticsInstrument_PolarPerformance::TacticsInstrument_PolarPerformance(
    wxWindow *parent, wxWindowID id, wxString title) :
DashboardInstrument(
    parent, id, title,
    OCPN_DBP_STC_STW | OCPN_DBP_STC_TWA | OCPN_DBP_STC_TWS, SETDRAWSOLOINPANE)
{
    m_pconfig = GetOCPNConfigObject();
    
    alpha = 0.02;  //smoothing constant
    for (int idx = 0; idx < DATA_RECORD_COUNT; idx++) {
        m_ArrayPercentSpdHistory[idx] = -1;
        m_ArrayBoatSpdHistory[idx] = -1;
        m_ArrayTWAHistory[idx] = -1;
        m_ExpSmoothArrayBoatSpd[idx] = -1;
        m_ExpSmoothArrayPercentSpd[idx] = -1;
        m_ArrayRecTime[idx] = wxDateTime::UNow().GetTm( );
        m_ArrayRecTime[idx].year = 999;
    }
    m_PolarPerfUpdTimer = new wxTimer ( this, myID_THREAD_POLARPERFORMANCE );
    m_MaxBoatSpd = 0.0;
    m_MinBoatSpd = 0.0;
    m_MaxPercent = 0.0;
    m_AvgSpdPercent = 0.0;
    mExpSmAvgSpdPercent = new DoubleExpSmooth(alpha);
    m_AvgTWA = 0.0;
    mExpSmAvgTWA = new DoubleExpSmooth(alpha);
    m_AvgTWS = 0.0;
    mExpSmAvgTWS = new DoubleExpSmooth(alpha);
    
    m_TWA = std::nan("1");
    m_TWS = std::nan("1");
    m_STW = std::nan("1");
    m_PolarSpeedPercent = 0.0;
    m_PolarSpeed = 0.0;
    m_MaxPercentScale = 0.0;
    m_MaxBoatSpdScale = 0.0;
    num_of_scales = 6;
    m_ratioW = 0.0;
    m_IsRunning = false;
    m_SampleCount = 0;
    m_STWUnit = _T("--");
    m_PercentUnit = _T("%");

    m_WindowRect = GetClientRect();
    m_DrawAreaRect = GetClientRect();
    m_DrawAreaRect.SetHeight(
        m_WindowRect.height - m_TopLineHeight - m_TitleHeight);
    m_TopLineHeight = 35;
    m_TitleHeight = 10;
    m_ratioW = 0;
    m_LeftLegend = 3;
    m_RightLegend = 3;

    m_logfile = wxEmptyString;
    m_ostreamlogfile = new wxFile();
    m_exportInterval = 5;

    m_PolarPerfUpdTimer->Start(1000, wxTIMER_CONTINUOUS);

    //data export
    m_isExporting = false;
    wxPoint pos;
    pos.x = pos.y = 0;
    m_LogButton = new wxButton(
        this, wxID_ANY, _(">"), pos, wxDefaultSize,
        wxBU_TOP | wxBU_EXACTFIT | wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE);
    m_LogButton->SetToolTip(
        _("'>' starts data export. Creates a new, or append "
          "to an existing file,\n'X' stops data export"));
    m_LogButton->Connect(
        wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(
            TacticsInstrument_PolarPerformance::OnLogDataButtonPressed),
        NULL,
        this);
    if (LoadConfig() == false) {
        m_exportInterval = 5;
        SaveConfig();
    }
 
    m_pExportmenu = new wxMenu();
    btn1Sec = m_pExportmenu->AppendRadioItem(
        myID_PP_EXPORTRATE_1, _("Export rate 1 Second"));
    btn5Sec = m_pExportmenu->AppendRadioItem(
        myID_PP_EXPORTRATE_5, _("Export rate 5 Seconds"));
    btn10Sec = m_pExportmenu->AppendRadioItem(
        myID_PP_EXPORTRATE_10, _("Export rate 10 Seconds"));
    btn20Sec = m_pExportmenu->AppendRadioItem(
        myID_PP_EXPORTRATE_20, _("Export rate 20 Seconds"));
    btn60Sec = m_pExportmenu->AppendRadioItem(
        myID_PP_EXPORTRATE_60, _("Export rate 60 Seconds"));

    if (m_exportInterval == 1) btn1Sec->Check(true);
    if (m_exportInterval == 5) btn5Sec->Check(true);
    if (m_exportInterval == 10) btn10Sec->Check(true);
    if (m_exportInterval == 20) btn20Sec->Check(true);
    if (m_exportInterval == 60) btn60Sec->Check(true);
  
}

TacticsInstrument_PolarPerformance::~TacticsInstrument_PolarPerformance(void)
{
    this->m_PolarPerfUpdTimer->Stop();
    delete this->m_PolarPerfUpdTimer;
    m_LogButton->Disconnect(
        wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(
            TacticsInstrument_PolarPerformance::OnLogDataButtonPressed),
        NULL,
        this);
    if (m_isExporting)
        m_ostreamlogfile->Close();
    delete this->m_ostreamlogfile;
}

wxSize TacticsInstrument_PolarPerformance::GetSize(int orient, wxSize hint)
{
    wxClientDC dc(this);
    int w;
    dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
    if (orient == wxHORIZONTAL) {
        return wxSize(DefaultWidth, wxMax(m_TitleHeight + 140, hint.y));
    }
    else {
        return wxSize(wxMax(hint.x, DefaultWidth),
                      wxMax(m_TitleHeight + 140, hint.y));
  }
}

void TacticsInstrument_PolarPerformance::SetData(
    unsigned long long st, double data, wxString unit, long long timestamp )
{
    if (std::isnan(data))
        return;
  
    if (st == OCPN_DBP_STC_STW || st == OCPN_DBP_STC_TWA || st == OCPN_DBP_STC_TWS) {
        if (st == OCPN_DBP_STC_TWA) {
            m_TWA = data;
        }
        if (st == OCPN_DBP_STC_TWS) {
            //m_TWS = data;
            //convert to knots first
            m_TWS = fromUsrSpeed_Plugin(data, g_iDashWindSpeedUnit);
        }

        if (st == OCPN_DBP_STC_STW) {
            //m_STW = data;
            //convert to knots first
            m_STW = fromUsrSpeed_Plugin(data, g_iDashSpeedUnit);
            m_STWUnit = unit;
        }
    }
}

//***************************************************************************
// Timer callback function
//***************************************************************************

void TacticsInstrument_PolarPerformance::OnPolarPerfUpdTimer(
    wxTimerEvent & event)
{
    if (wxIsNaN(m_STW) || wxIsNaN(m_TWA) || wxIsNaN(m_TWS))
        return;

    m_PolarSpeedPercent = std::nan("1");

    double m_PolarSpeed = BoatPolar->GetPolarSpeed(m_TWA, m_TWS);

    bool PolarperfOvershoot = false;
    
    if (std::isnan(m_PolarSpeed)) {
        m_PercentUnit = _T("no polar data");
        m_PolarSpeed = 0.;
    }
    else if (m_PolarSpeed <= 0.) {
        m_PercentUnit = _T("--");
        m_PolarSpeed = 0.;
    }
    else {
        m_PolarSpeedPercent = m_STW / m_PolarSpeed * 100;
        m_PercentUnit = _T("%");
        if ( std::isnan(m_PolarSpeedPercent) ||
             (m_PolarSpeedPercent >= POLAR_PERFORMANCE_PERCENTAGE_LIMIT) ) {
            m_PolarSpeedPercent = POLAR_PERFORMANCE_PERCENTAGE_LIMIT;
            m_PercentUnit = L"\u2191"; // show arrow up for overshoot
            PolarperfOvershoot = true;
        }
    }
    m_IsRunning = true;
    m_SampleCount = (m_SampleCount < DATA_RECORD_COUNT ?
                     m_SampleCount + 1 : DATA_RECORD_COUNT);
    m_MaxPercent = 0;
    m_MaxBoatSpd = 0;
    m_MinBoatSpd = 0;

    //data shifting
    for (int idx = 1; idx < DATA_RECORD_COUNT; idx++) {
        m_MaxPercent = wxMax(m_ArrayPercentSpdHistory[idx - 1], m_MaxPercent);
        m_ArrayPercentSpdHistory[idx - 1] = m_ArrayPercentSpdHistory[idx];
        m_ExpSmoothArrayPercentSpd[idx - 1] = m_ExpSmoothArrayPercentSpd[idx];
        m_MaxBoatSpd = wxMax(m_ArrayBoatSpdHistory[idx - 1], m_MaxBoatSpd);
        m_ArrayBoatSpdHistory[idx - 1] = m_ArrayBoatSpdHistory[idx];
        m_ExpSmoothArrayBoatSpd[idx - 1] = m_ExpSmoothArrayBoatSpd[idx];
        m_ArrayRecTime[idx - 1] = m_ArrayRecTime[idx];
    }
    if ( !PolarperfOvershoot )
        m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 1] = m_PolarSpeedPercent;
    else {
        if ( m_SampleCount >= 2 ) {
            m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 1] =
                m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 2];
            m_PolarSpeedPercent =
                m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 2];
        }
        else {
            m_PolarSpeedPercent = POLAR_PERFORMANCE_PERCENTAGE_LIMIT - 1;
            m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 1] =
                m_PolarSpeedPercent;
        }
    } // else overshooting polar performance percentage - roll back previous
    m_ArrayBoatSpdHistory[DATA_RECORD_COUNT - 1] = m_STW;
    if (m_SampleCount < 2) {
        m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 2] = m_PolarSpeedPercent;
        m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 2] =
            m_PolarSpeedPercent;
        m_ArrayBoatSpdHistory[DATA_RECORD_COUNT - 2] = m_STW;
        m_ExpSmoothArrayBoatSpd[DATA_RECORD_COUNT - 2] = m_STW;
    }
    m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 1] =
        alpha * m_ArrayPercentSpdHistory[DATA_RECORD_COUNT - 2] +
        (1 - alpha) * m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 2];
    if ( std::isnan(m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 1]) )
        m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 1] =
            m_ExpSmoothArrayPercentSpd[DATA_RECORD_COUNT - 2];  
    m_ExpSmoothArrayBoatSpd[DATA_RECORD_COUNT - 1] =
        alpha * m_ArrayBoatSpdHistory[DATA_RECORD_COUNT - 2] +
        (1 - alpha) * m_ExpSmoothArrayBoatSpd[DATA_RECORD_COUNT - 2];
    m_ArrayRecTime[DATA_RECORD_COUNT - 1] = wxDateTime::UNow().GetTm();
    //include the new/latest value in the max/min value test too
    m_MaxPercent = wxMax(m_PolarSpeedPercent, m_MaxPercent);
    m_MaxBoatSpd = wxMax(m_STW, m_MaxBoatSpd);
    /* Show smoothed average percentage instead of "overall max percentage"
       which is not really useful, especially if it uses the unsmoothed
       values ... */
    m_AvgSpdPercent = mExpSmAvgSpdPercent->GetSmoothVal(m_PolarSpeedPercent);
    m_AvgTWA = mExpSmAvgTWA->GetSmoothVal(m_TWA);
    m_AvgTWS = mExpSmAvgTWS->GetSmoothVal(m_TWS);

    // Data export  
    ExportData();
}

void TacticsInstrument_PolarPerformance::Draw(wxGCDC* dc)
{
    m_WindowRect = GetClientRect();
    m_DrawAreaRect = GetClientRect();
    m_DrawAreaRect.SetHeight(
        m_WindowRect.height - m_TopLineHeight - m_TitleHeight);
    m_DrawAreaRect.SetX(m_LeftLegend + 3);
    DrawBackground(dc);
    DrawForeground(dc);
}

//***************************************************************************
// Draw boat speed legend (right side)
//***************************************************************************
void  TacticsInstrument_PolarPerformance::DrawBoatSpeedScale(wxGCDC* dc)
{
    wxString label[41]; // max 800%
    wxColour cl;
    wxPen pen;
    int width, height;
    int tmpval = (int)(m_MaxBoatSpd + 2) % 2;
    m_MaxBoatSpdScale = (int)(m_MaxBoatSpd + 2 - tmpval);

    cl = wxColour(204, 41, 41, 255); //red, opague

    dc->SetTextForeground(cl);
    dc->SetFont(*g_pFontSmall);
    if (!m_IsRunning) {
        for (int i = 0; i < num_of_scales; i++)
            label[i].Printf(_T("--- %s"), m_STWUnit);
    }
    else {
        /* We round the speed up to the next full knot ==> the top and bottom
           line have full numbers as legend (e.g. 23 kn -- 0 kn),
           but the intermediate lines may have decimal values (e.g. center
           line : 23/2=11.5 or quarter line 23/4=5.75), so in the worst case
           we will end up with:
           23 - 17.25 - 11.5 - 5.75 - 0
           The goal is to draw the legend with decimals only, if we have them!
        */
        // label 1 : legend for bottom line. By definition always w/o decimals
        label[0].Printf(_T("%.0f %s"), toUsrSpeed_Plugin(
                            m_MinBoatSpd, g_iDashSpeedUnit),
                        m_STWUnit.c_str());
        for (int i = 1; i < num_of_scales; i++){
            // legend every 20 %
            double BoatSpdScale =
                m_MaxBoatSpdScale * i * 1. / (num_of_scales - 1);
            label[i].Printf(_T("%.1f %s"), toUsrSpeed_Plugin(
                                BoatSpdScale, g_iDashSpeedUnit),
                            m_STWUnit.c_str());
        }
    }
    /* Draw the legend with the labels; find the widest string
       and store it in m_RightLegend which is the basis for the
       horizontal lines.
    */
    dc->GetTextExtent(label[num_of_scales - 1],
                      &m_RightLegend, &height, 0, 0, g_pFontSmall);
    for (int i = 0; i < (num_of_scales - 1); i++) {
        dc->GetTextExtent(label[i], &width, &height, 0, 0, g_pFontSmall);
        m_RightLegend = wxMax(width, m_RightLegend);
    }
    m_RightLegend += 4; //leave some space to the edge
    for (int i = 0; i < num_of_scales; i++)
        dc->DrawText(
            label[i],
            (m_WindowRect.width - m_RightLegend),
            static_cast<int>(
                m_TopLineHeight + m_DrawAreaRect.height -
                (m_DrawAreaRect.height * i * 1. /
                 static_cast<double>(num_of_scales - 1) ) -
                height / 2) );
}
//***************************************************************************
// Draw percent boat speed scale (left side)
//***************************************************************************
void  TacticsInstrument_PolarPerformance::DrawPercentSpeedScale(wxGCDC* dc)
{
    wxString label[41],labeloor; // max 800%
    wxColour cl;
    int width, height;

    cl = wxColour(61, 61, 204, 255);
    dc->SetTextForeground(cl);
    dc->SetFont(*g_pFontSmall);
    int tmpval = (int)(m_MaxPercent + 20) % 20;
    m_MaxPercentScale = (int)(m_MaxPercent + 20 - tmpval);
    if (m_MaxPercentScale < 100.0){
        m_MaxPercentScale = 100.0; //show min 100% 
        num_of_scales = 6;
    } else
        num_of_scales = m_MaxPercentScale / 20 + 1;
    if (num_of_scales > 41) {
        num_of_scales = 41; //avoid overrun of label[41]
        dc->SetFont(*g_pFontData);
        labeloor.Printf(_T("Polar data out of range"));
        dc->GetTextExtent(labeloor, &width, &height, 0, 0, g_pFontData);
        dc->DrawText(labeloor, 4 + m_DrawAreaRect.width/2 - width/2,
                     (int)(m_TopLineHeight + m_DrawAreaRect.height / 2 -
                           height / 2));
    }
    if (!m_IsRunning) {
        for (int i = 0; i < num_of_scales; i++)
            label[i].Printf(_T("--- %s"), m_PercentUnit);
    }
    else {
        // We round the speed up to the next full knot 
        for (int i = 0; i < num_of_scales; i++)
            // legend every 20 %
            label[i].Printf(_T("%i %s"), (int)(i * 20), m_PercentUnit);

    }
    dc->GetTextExtent(
        label[num_of_scales-1], &m_LeftLegend, &height, 0, 0, g_pFontSmall);
    for (int i = 0; i < (num_of_scales - 1); i++) {
        dc->GetTextExtent(label[i], &width, &height, 0, 0, g_pFontSmall);
        m_LeftLegend = wxMax(width, m_LeftLegend);
    }
    m_LeftLegend += 4;

    for ( int i = 0; i < num_of_scales; i++ ) {
        dc->DrawText(
            label[i],
            4,
            static_cast<int>(
                m_TopLineHeight + m_DrawAreaRect.height -
                (m_DrawAreaRect.height* i *  1. /
                 static_cast<double>(num_of_scales - 1) ) -
                height / 2));
    }
}

//***************************************************************************
// Draw background
//***************************************************************************
void TacticsInstrument_PolarPerformance::DrawBackground(wxGCDC* dc)
{
    wxString label, label1, label2, label3, label4, label5;
    wxColour cl;
    wxPen pen;
    //-----------------------------------------------------------------------
    // draw legends for speed and direction
    //-----------------------------------------------------------------------
    DrawPercentSpeedScale(dc);
    DrawBoatSpeedScale(dc);
    //-----------------------------------------------------------------------
    // horizontal lines
    //-----------------------------------------------------------------------
    GetGlobalColor(_T("UBLCK"), &cl);
    pen.SetColour(cl);
    dc->SetPen(pen);
    for (int i = 0; i < num_of_scales; i++)
       dc->DrawLine(
           m_LeftLegend + 3,
           (int)(m_TopLineHeight + m_DrawAreaRect.height -
                 (m_DrawAreaRect.height* i *  1. / (num_of_scales - 1))),
           m_WindowRect.width - 3 - m_RightLegend,
           (int)(m_TopLineHeight + m_DrawAreaRect.height -
                 (m_DrawAreaRect.height* i *  1. / (num_of_scales - 1))));
}


//**************************************************************************
// Draw foreground
//**************************************************************************
void TacticsInstrument_PolarPerformance::DrawForeground(wxGCDC* dc)
{
    wxColour col;
    double ratioH;
    int degw, degh;
    int stwstart, perfend;
    int width, height, sec, min, hour;
    wxString  BoatSpeed, PercentSpeed, TWAString;
    wxPen pen;
    wxString label;

    //-----------------------------------------------------------------------
    // boat speed
    //-----------------------------------------------------------------------
    dc->SetFont(*g_pFontData);
    col = wxColour(204, 41, 41, 255); //red, opaque
    dc->SetTextForeground(col);
    if (!m_IsRunning)
        BoatSpeed = _T("STW ---");
    else {
        BoatSpeed = wxString::Format(_T("STW %3.2f %s"),
                                     toUsrSpeed_Plugin(
                                         m_STW, g_iDashSpeedUnit),
                                     m_STWUnit.c_str());
    }
    dc->GetTextExtent(BoatSpeed, &degw, &degh, 0, 0, g_pFontData);
    /* Remember the left end=startpoint of the string (used to place
       the avg.TWA) */
    stwstart = m_WindowRect.width - degw - m_RightLegend - 3;
    dc->DrawText(BoatSpeed, stwstart, m_TopLineHeight - degh);

    //-----------------------------------------------------------------------
    // live boat speed data
    //-----------------------------------------------------------------------
    pen.SetStyle(wxPENSTYLE_SOLID);
    pen.SetColour(wxColour(204, 41, 41, 96)); //red, transparent
    pen.SetWidth(1);
    dc->SetPen(pen);
    ratioH = (double)m_DrawAreaRect.height / m_MaxBoatSpdScale;
    m_DrawAreaRect.SetWidth(
        m_WindowRect.width - 6 - m_LeftLegend - m_RightLegend);
    m_ratioW = double(m_DrawAreaRect.width) / (DATA_RECORD_COUNT - 1);

    wxPoint points[DATA_RECORD_COUNT + 2], pointAngle_old;
    pointAngle_old.x = 3 + m_LeftLegend;
    pointAngle_old.y = m_TopLineHeight + m_DrawAreaRect.height -
        (m_ArrayBoatSpdHistory[0] - m_MinBoatSpd) * ratioH;
    // wxLogMessage(
    //     "Live:pointAngle_old.x=%d, pointAngle_old.y=%d",
    //     pointAngle_old.x, pointAngle_old.y);
    for (int idx = 1; idx < DATA_RECORD_COUNT; idx++) {
        points[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
        points[idx].y = m_TopLineHeight + m_DrawAreaRect.height -
            (m_ArrayBoatSpdHistory[idx] - m_MinBoatSpd) * ratioH;
        //  wxLogMessage("Live:points[%d].y=%d", idx, points[idx].y);
        if ( ((DATA_RECORD_COUNT - m_SampleCount) <= idx) &&
             (points[idx].y > m_TopLineHeight) &&
             (pointAngle_old.y > m_TopLineHeight) &&
             (points[idx].y <=(m_TopLineHeight + m_DrawAreaRect.height)) &&
             (pointAngle_old.y <= (m_TopLineHeight + m_DrawAreaRect.height)) )
            dc->DrawLine(
                pointAngle_old.x, pointAngle_old.y, points[idx].x,
                points[idx].y);
        pointAngle_old.x = points[idx].x;
        pointAngle_old.y = points[idx].y;
    }

    //-----------------------------------------------------------------------
    //exponential smoothing of boat speed
    //-----------------------------------------------------------------------
    pen.SetStyle(wxPENSTYLE_SOLID);
    pen.SetColour(wxColour(204, 41, 41, 255));
    pen.SetWidth(2);
    dc->SetPen(pen);
    pointAngle_old.x = 3 + m_LeftLegend;
    pointAngle_old.y = m_TopLineHeight + m_DrawAreaRect.height -
        (m_ExpSmoothArrayBoatSpd[0] - m_MinBoatSpd) * ratioH;
    for (int idx = 1; idx < DATA_RECORD_COUNT; idx++) {
        points[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
        points[idx].y = m_TopLineHeight + m_DrawAreaRect.height -
            (m_ExpSmoothArrayBoatSpd[idx] - m_MinBoatSpd) * ratioH;
        if ( ((DATA_RECORD_COUNT - m_SampleCount) <= idx) &&
             (points[idx].y > m_TopLineHeight) &&
             (pointAngle_old.y > m_TopLineHeight) &&
             (points[idx].y <= (m_TopLineHeight + m_DrawAreaRect.height)) &&
             (pointAngle_old.y <= (m_TopLineHeight + m_DrawAreaRect.height)) )
            dc->DrawLine(
                pointAngle_old.x, pointAngle_old.y, points[idx].x,
                points[idx].y);
        pointAngle_old.x = points[idx].x;
        pointAngle_old.y = points[idx].y;
    }

    //-----------------------------------------------------------------------
    // boat speed in percent polar
    //-----------------------------------------------------------------------
    col = wxColour(61, 61, 204, 255); //blue, opaque
    dc->SetFont(*g_pFontData);
    dc->SetTextForeground(col);
    PercentSpeed = wxString::Format(
        _T("Polar Perf. %3.2f %s "), m_PolarSpeedPercent, m_PercentUnit);
    dc->GetTextExtent(PercentSpeed, &degw, &degh, 0, 0, g_pFontData);
    dc->DrawText(PercentSpeed, m_LeftLegend + 3, m_TopLineHeight - degh);
    dc->SetFont(*g_pFontLabel);
    // Determine the time range of the available data (=oldest data value)
    int i = 0;
    while ( (i < (DATA_RECORD_COUNT - 1)) &&
            (m_ArrayRecTime[i].year == 999) ) i++;
    if ( i == (DATA_RECORD_COUNT - 1) ) {
        min = 0;
        hour = 0;
    }
    else {
        wxDateTime localTime( m_ArrayRecTime[i] );
        min = localTime.GetMinute( );
        hour = localTime.GetHour( );
    }
    // Single text var to facilitate correct translations:
    wxString s_Max = _("Max");
    wxString s_Since = _("since");
    wxString s_Avg = _("avg.");
    wxString s_PerfData = wxString::Format(
        _T("%s %.1f %s %s %02d:%02d  %s %.1f %s"), s_Max, m_MaxPercent,
        m_PercentUnit, s_Since, hour, min, s_Avg, m_AvgSpdPercent,
        m_PercentUnit);
    // dc->DrawText(wxString::Format(
    //                  _T("%s %.1f %s %s %02d:%02d  %s %.1f %s"),
    //                  s_Max, m_MaxPercent, m_PercentUnit, s_Since, hour, min,
    //                  s_OMax, m_TotalMaxSpdPercent, m_PercentUnit),
    //              m_LeftLegend + 3 + 2 + degw, m_TopLineHeight - degh + 5);
    dc->DrawText(
        s_PerfData, m_LeftLegend + 3 + 2 + degw, m_TopLineHeight - degh + 2);
    // Remember the right end of the string (used to place the avg.TWA)
    perfend = m_LeftLegend + 3 + 2 + degw;
    dc->GetTextExtent(s_PerfData, &degw, &degh, 0, 0, g_pFontData);
    perfend += degw;

    pen.SetStyle(wxPENSTYLE_SOLID);
    pen.SetColour(wxColour(61, 61, 204, 96)); //blue, transparent
    pen.SetWidth(1);
    dc->SetPen(pen);
    ratioH = (double)m_DrawAreaRect.height / m_MaxPercentScale;
    m_DrawAreaRect.SetWidth(
        m_WindowRect.width - 6 - m_LeftLegend - m_RightLegend);
    m_ratioW = double(m_DrawAreaRect.width) / (DATA_RECORD_COUNT - 1);
    wxPoint  pointsSpd[DATA_RECORD_COUNT + 2], pointSpeed_old;
    pointSpeed_old.x = m_LeftLegend + 3;
    pointSpeed_old.y = m_TopLineHeight + m_DrawAreaRect.height -
        m_ArrayPercentSpdHistory[0] * ratioH;

    //-----------------------------------------------------------------------
    // live speed data in percent polar
    //-----------------------------------------------------------------------
    for (int idx = 1; idx < DATA_RECORD_COUNT; idx++) {
        pointsSpd[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
        pointsSpd[idx].y = m_TopLineHeight + m_DrawAreaRect.height -
            m_ArrayPercentSpdHistory[idx] * ratioH;
        if ( ((DATA_RECORD_COUNT - m_SampleCount) <= idx) &&
             (pointsSpd[idx].y > m_TopLineHeight) &&
             (pointSpeed_old.y > m_TopLineHeight) &&
             (pointsSpd[idx].y <= (m_TopLineHeight +
                                   m_DrawAreaRect.height)) &&
             (pointSpeed_old.y <= (m_TopLineHeight + m_DrawAreaRect.height)) )
            dc->DrawLine(
                pointSpeed_old.x, pointSpeed_old.y, pointsSpd[idx].x,
                pointsSpd[idx].y);
        pointSpeed_old.x = pointsSpd[idx].x;
        pointSpeed_old.y = pointsSpd[idx].y;
    }

    //-----------------------------------------------------------------------
    //exponential smoothing of percentage
    //-----------------------------------------------------------------------
    pen.SetStyle(wxPENSTYLE_SOLID);
    pen.SetColour(wxColour(61, 61, 204, 255)); //blue, opaque
    pen.SetWidth(2);
    dc->SetPen(pen);
    pointSpeed_old.x = m_LeftLegend + 3;
    pointSpeed_old.y = m_TopLineHeight + m_DrawAreaRect.height -
        m_ExpSmoothArrayPercentSpd[0] * ratioH;
    for (int idx = 1; idx < DATA_RECORD_COUNT; idx++) {
        pointsSpd[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
        pointsSpd[idx].y = m_TopLineHeight + m_DrawAreaRect.height -
            m_ExpSmoothArrayPercentSpd[idx] * ratioH;
        if ( ((DATA_RECORD_COUNT - m_SampleCount) <= idx) &&
             (pointsSpd[idx].y > m_TopLineHeight) &&
             (pointSpeed_old.y > m_TopLineHeight) &&
             (pointsSpd[idx].y <= (m_TopLineHeight +
                                   m_DrawAreaRect.height)) &&
             (pointSpeed_old.y <= (m_TopLineHeight + m_DrawAreaRect.height)) )
            dc->DrawLine(
                pointSpeed_old.x, pointSpeed_old.y, pointsSpd[idx].x,
                pointsSpd[idx].y);
        pointSpeed_old.x = pointsSpd[idx].x;
        pointSpeed_old.y = pointsSpd[idx].y;
    }

    //-----------------------------------------------------------------------
    // Add avg.TWA in headline, in the middle of the free space,
    // between polarPerf. and STW
    //-----------------------------------------------------------------------
    dc->SetFont(*g_pFontData);
    col = wxColour(0, 0, 0, 255); //black, opaque
    dc->SetTextForeground(col);
    if (!m_IsRunning)
        TWAString = wxString::Format(_T("%sTWA ---"), s_Avg);
    else {
        TWAString = wxString::Format(_T("%sTWA %3.0f"),s_Avg, m_AvgTWA) +
            DEGREE_SIGN;
    }
    dc->GetTextExtent(TWAString, &degw, &degh, 0, 0, g_pFontData);
    dc->DrawText(TWAString, perfend + (stwstart-perfend)/2 - degw / 2,
                 m_TopLineHeight - degh);

    //-----------------------------------------------------------------------
    // Draw vertical timelines every 5 minutes
    //-----------------------------------------------------------------------
    GetGlobalColor(_T("UBLCK"), &col);
    pen.SetColour(col);
    pen.SetStyle(wxPENSTYLE_DOT);
    dc->SetPen(pen);
    dc->SetTextForeground(col);
    dc->SetFont(*g_pFontSmall);
    int done = -1;
    wxPoint pointTime;
    for (int idx = 0; idx < DATA_RECORD_COUNT; idx++) {
        if (m_ArrayRecTime[idx].year != 999) {
            wxDateTime localTime( m_ArrayRecTime[idx] );
            min = localTime.GetMinute( );
            hour = localTime.GetHour( );
            sec = localTime.GetSecond( );
            if ((hour * 100 + min) != done && (min % 5 == 0) &&
                (sec == 0 || sec == 1)) {
                pointTime.x = idx * m_ratioW + 3 + m_LeftLegend;
                dc->DrawLine(
                    pointTime.x, m_TopLineHeight + 1,
                    pointTime.x,(m_TopLineHeight+m_DrawAreaRect.height+1) );
                label.Printf(_T("%02d:%02d"), hour,min);
                dc->GetTextExtent(label, &width, &height, 0, 0, g_pFontSmall);
                dc->DrawText(label, pointTime.x-width/2,
                             m_WindowRect.height-height);
                done=hour*100+min;
            }
        }
    }
}

//**************************************************************************
// Data logging functions
//**************************************************************************

void TacticsInstrument_PolarPerformance::OnLogDataButtonPressed(
    wxCommandEvent& event)
{

    if (m_isExporting == false) {
        wxPoint pos;
        m_LogButton->GetSize(&pos.x, &pos.y);
        pos.x = 0;
        this->PopupMenu(m_pExportmenu, pos);
        if (btn1Sec->IsChecked()) m_exportInterval = 1;
        if (btn5Sec->IsChecked()) m_exportInterval = 5;
        if (btn10Sec->IsChecked()) m_exportInterval = 10;
        if (btn20Sec->IsChecked()) m_exportInterval = 20;
        if (btn60Sec->IsChecked()) m_exportInterval = 60;

        wxFileDialog fdlg(GetOCPNCanvasWindow(),
                          _("Choose a new or existing file"), wxT(""),
                          m_logfile, wxT("*.*"), wxFD_SAVE);
        if (fdlg.ShowModal() != wxID_OK) {
            return;
        }
        m_logfile.Clear();
        m_logfile = fdlg.GetPath();
        bool exists = m_ostreamlogfile->Exists(m_logfile);
        m_ostreamlogfile->Open(m_logfile, wxFile::write_append);
        if (!exists) {
            wxString str_ticks =
                g_bDataExportClockticks ? wxString::Format(
                    _("   ClockTicks%s"), g_sDataExportSeparator) :
                _T("");
            wxString str_utc =
                g_bDataExportUTC ? wxString::Format(
                    _("         UTC-ISO8601%s"), g_sDataExportSeparator) :
                _T("");
            wxString str = wxString::Format(
                _T(
                    "%s"    // ticks
                    "%s"    // utc
                    "%s%s"  // local date, separator
                    "%s%s"  // local time, separator
                    "%s%s"  // AvgTWA, separator
                    "%s%s"  // AvgTWS, separator
                    "%s%s"  // Smoothed Boastspeed, separator
                    "%s\n"),// Perf.Percentage
                str_ticks,
                str_utc,
                "      Date", g_sDataExportSeparator,
                " Local Time", g_sDataExportSeparator,
                " aTWA", g_sDataExportSeparator,
                "aTWS", g_sDataExportSeparator,
                " sSpd", g_sDataExportSeparator,
                " PerfP");
      m_ostreamlogfile->Write(str);
    }
    SaveConfig(); //save the new export-rate &filename to opencpn.ini
    m_isExporting = true;
    m_LogButton->SetLabel(_("X"));
    m_LogButton->Refresh();
  }
  else if (m_isExporting == true) {
    m_isExporting = false;

    m_ostreamlogfile->Close();
    m_LogButton->SetLabel(_(">"));
    m_LogButton->Refresh();
  }
}

bool TacticsInstrument_PolarPerformance::LoadConfig(void)
{
  wxFileConfig *pConf = m_pconfig;

  if (pConf) {
    pConf->SetPath(_T("/PlugIns/DashT/Tactics/PolarPerformance"));
    pConf->Read(_T("Exportrate"), &m_exportInterval, 5);
    pConf->Read(_T("PolarPerformanceExportfile"), &m_logfile, wxEmptyString);
    return true;
  }
  else
    return false;
}

bool TacticsInstrument_PolarPerformance::SaveConfig(void)
{
  wxFileConfig *pConf = m_pconfig;

  if (pConf)
  {
    pConf->SetPath(_T("/PlugIns/DashT/Tactics/PolarPerformance"));
    pConf->Write(_T("Exportrate"), m_exportInterval);
    pConf->Write(_T("PolarPerformanceExportfile"), m_logfile);
    return true;
  }
  else
    return false;
}

void TacticsInstrument_PolarPerformance::ExportData(void) {
  if ( !m_isExporting )
      return;
  wxDateTime localTime(m_ArrayRecTime[DATA_RECORD_COUNT - 1]);
  if (localTime.GetSecond() % m_exportInterval == 0) {
      wxString str_utc, ticks;
      if (g_bDataExportUTC) {
          wxDateTime utc = localTime.ToUTC();
          str_utc = wxString::Format(_T("%sZ%s"), utc.FormatISOCombined('T'),
                                     g_sDataExportSeparator);
      }
      else
          str_utc = _T("");
      if (g_bDataExportClockticks) {
          wxLongLong ti = wxGetUTCTimeMillis();
          wxString sBuffer = ti.ToString();
          wxString ticksString = sBuffer.wc_str();
          ticks = wxString::Format(_T("%s%s"), ticksString,
                                   g_sDataExportSeparator);
      }
      else
          ticks = _T("");
      /* Albeit we print on the screen the wind angle with %3.0f precision,
         for export let's have one decimal: %5.1f */
      wxString str = wxString::Format(
          _T(
                "%s"       // ticks
                "%s"       // utc
                "%s%s"     // local date, separator
                "%11s%s"   // local time, separator
                "%5.1f%s"  // AvgTWA, separator
                "%4.1f%s"  // AvgTWS, separator
                "%5.2f%s"  // Smoothed Boatspeed, separator
                "%6.2f\n"),// Polar Perf. Percentage
          ticks,
          str_utc,
          localTime.FormatDate(), g_sDataExportSeparator,
          localTime.FormatTime(), g_sDataExportSeparator,
          m_AvgTWA, g_sDataExportSeparator,
          toUsrSpeed_Plugin(
              m_AvgTWS, g_iDashWindSpeedUnit), g_sDataExportSeparator,
          toUsrSpeed_Plugin(
              m_ExpSmoothArrayBoatSpd[DATA_RECORD_COUNT - 1],
              g_iDashSpeedUnit), g_sDataExportSeparator,
          m_AvgSpdPercent);
      m_ostreamlogfile->Write(str);
  }
}
