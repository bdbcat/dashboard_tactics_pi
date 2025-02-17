/******************************************************************************
 * $Id: wind_history.cpp, v1.0 2010/08/30 tom-r Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Thomas Rauch
 *   (Inspired by original work from Jean-Eudes Onfray)
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
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/fileconf.h>
#include "wind_history.h"

#include "tactics_pi_ext.h"
#include "dashboard_pi_ext.h"

#define SETDRAWSOLOINPANE true

#include "plugin_ids.h"
wxBEGIN_EVENT_TABLE (DashboardInstrument_WindDirHistory, DashboardInstrument)
EVT_TIMER (myID_THREAD_WINDHISTORY, DashboardInstrument_WindDirHistory::OnWindHistUpdTimer)
wxEND_EVENT_TABLE ()

//************************************************************************************************************************
// History of wind direction
//************************************************************************************************************************

DashboardInstrument_WindDirHistory::DashboardInstrument_WindDirHistory( wxWindow *parent, wxWindowID id, wxString title) :
    DashboardInstrument(parent, id, title, OCPN_DBP_STC_TWD | OCPN_DBP_STC_TWS, SETDRAWSOLOINPANE)
{
    m_SpdRecCnt = 0;
    m_DirRecCnt = 0;
    m_SpdStartVal = -1;
    m_DirStartVal = -1;
    m_pconfig = GetOCPNConfigObject();
    m_bWindSpeedUnitResetLogged = false;
    alpha=0.01;  //smoothing constant
    for (int idx = 0; idx < WIND_RECORD_COUNT; idx++) {
        m_ArrayWindDirHistory[idx] = -1;
        m_ArrayWindSpdHistory[idx] = -1;
        m_ExpSmoothArrayWindSpd[idx] = -1;
        m_ExpSmoothArrayWindDir[idx] = -1;
        m_ArrayRecTime[idx]=wxDateTime::UNow().GetTm();
        m_ArrayRecTime[idx].year=999;
    }
    m_MaxWindDir = -1;
    m_MinWindDir = -1;
    m_WindDirRange=90;
    m_MaxWindSpd = 0;
    m_TotalMaxWindSpd = 0;
    m_WindDir = -1;
    m_WindSpd = 0;
    m_TrueWindDir = NAN;
    m_TrueWindSpd = NAN;
    m_MaxWindSpdScale = NAN;
    m_ratioW = NAN;
    m_oldDirVal = NAN;
    m_IsRunning = false;
    m_SampleCount = 0;
    m_WindSpeedUnit = _("--");
    m_WindHistUpdTimer = new wxTimer( this, myID_THREAD_WINDHISTORY );
    m_WindowRect=GetClientRect();
    m_DrawAreaRect=GetClientRect();
    m_DrawAreaRect.SetHeight( m_WindowRect.height-m_TopLineHeight-m_TitleHeight );
    m_TopLineHeight=35;
    m_TitleHeight = 10;
    m_width = 0;
    m_height = 0;
    m_LeftLegend = 3;
    m_RightLegend = 3;
    m_logfile = wxEmptyString;
    m_ostreamlogfile = new wxFile();
    m_exportInterval = 0;

    m_WindHistUpdTimer->Start(1000, wxTIMER_CONTINUOUS);
    //data export
    wxPoint pos;
    pos.x = pos.y = 0;
    m_LogButton = new wxButton(this, wxID_ANY, _(">"), pos, wxDefaultSize,
                               wxBU_TOP | wxBU_EXACTFIT | wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE);
    m_LogButton->SetToolTip(
        _("'>' starts data export. Create a new, or append "
          "to an existing file,\n'X' stops data export") );
    m_LogButton->Connect(
        wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(
            DashboardInstrument_WindDirHistory::OnLogDataButtonPressed),
        NULL,
        this);
    m_pconfig = GetOCPNConfigObject();
    if (LoadConfig() == false) {
        m_exportInterval = 5;
        SaveConfig();
    }
    m_pExportmenu = new wxMenu();
    btn1Sec = m_pExportmenu->AppendRadioItem(myID_WH_EXPORTRATE_1, _("Export rate 1 Second"));
    btn5Sec = m_pExportmenu->AppendRadioItem(myID_WH_EXPORTRATE_5, _("Export rate 5 Seconds"));
    btn10Sec = m_pExportmenu->AppendRadioItem(myID_WH_EXPORTRATE_10, _("Export rate 10 Seconds"));
    btn20Sec = m_pExportmenu->AppendRadioItem(myID_WH_EXPORTRATE_20, _("Export rate 20 Seconds"));
    btn60Sec = m_pExportmenu->AppendRadioItem(myID_WH_EXPORTRATE_60, _("Export rate 60 Seconds"));
    
    if (m_exportInterval == 1) btn1Sec->Check(true);
    if (m_exportInterval == 5) btn5Sec->Check(true);
    if (m_exportInterval == 10) btn10Sec->Check(true);
    if (m_exportInterval == 20) btn20Sec->Check(true);
    if (m_exportInterval == 60) btn60Sec->Check(true);
}

DashboardInstrument_WindDirHistory::~DashboardInstrument_WindDirHistory(void) {
    this->m_WindHistUpdTimer->Stop();
    delete this->m_WindHistUpdTimer;
    if(m_isExporting)
        m_ostreamlogfile->Close();
    delete this->m_ostreamlogfile;
    m_LogButton->Disconnect(
        wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(DashboardInstrument_WindDirHistory::OnLogDataButtonPressed),
        NULL,
        this);

}

void DashboardInstrument_WindDirHistory::OnWindHistUpdTimer(wxTimerEvent &event)
{
    if (!std::isnan(m_TrueWindDir) && !std::isnan(m_TrueWindSpd)){
        m_WindDir = m_TrueWindDir;
        m_WindSpd = m_TrueWindSpd;
        //start working after we collected 5 records each, as start values for the smoothed curves
        if (m_SpdRecCnt >= 3 && m_DirRecCnt >= 3) {
            m_IsRunning = true;
            m_SampleCount = m_SampleCount < WIND_RECORD_COUNT ? m_SampleCount + 1 : WIND_RECORD_COUNT;
            m_MaxWindDir = 0;
            m_MinWindDir = 360;
            m_MaxWindSpd = 0;
            //data shifting
            for (int idx = 1; idx < WIND_RECORD_COUNT; idx++) {
                if (WIND_RECORD_COUNT - m_SampleCount <= idx)
                    m_MinWindDir = wxMin(m_ArrayWindDirHistory[idx], m_MinWindDir);
                m_MaxWindDir = wxMax(m_ArrayWindDirHistory[idx - 1], m_MaxWindDir);
                m_MaxWindSpd = wxMax(m_ArrayWindSpdHistory[idx - 1], m_MaxWindSpd);
                m_ArrayWindDirHistory[idx - 1] = m_ArrayWindDirHistory[idx];
                m_ArrayWindSpdHistory[idx - 1] = m_ArrayWindSpdHistory[idx];
                m_ExpSmoothArrayWindSpd[idx - 1] = m_ExpSmoothArrayWindSpd[idx];
                m_ExpSmoothArrayWindDir[idx - 1] = m_ExpSmoothArrayWindDir[idx];
                m_ArrayRecTime[idx - 1] = m_ArrayRecTime[idx];
            }
            double diff = m_WindDir - m_oldDirVal;
            if (diff < -270) {
                m_WindDir += 360;
            }
            else
                if (diff > 270) {
                    m_WindDir -= 360;
                }
            m_ArrayWindDirHistory[WIND_RECORD_COUNT - 1] = m_WindDir;
            m_ArrayWindSpdHistory[WIND_RECORD_COUNT - 1] = m_WindSpd;
            if (m_SampleCount < 2) {
                m_ArrayWindSpdHistory[WIND_RECORD_COUNT - 2] = m_WindSpd;
                m_ExpSmoothArrayWindSpd[WIND_RECORD_COUNT - 2] = m_WindSpd;
                m_ArrayWindDirHistory[WIND_RECORD_COUNT - 2] = m_WindDir;
                m_ExpSmoothArrayWindDir[WIND_RECORD_COUNT - 2] = m_WindDir;
            }
            m_ExpSmoothArrayWindSpd[WIND_RECORD_COUNT - 1] = alpha*m_ArrayWindSpdHistory[WIND_RECORD_COUNT - 2] + (1 - alpha)*m_ExpSmoothArrayWindSpd[WIND_RECORD_COUNT - 2];
            m_ExpSmoothArrayWindDir[WIND_RECORD_COUNT - 1] = alpha*m_ArrayWindDirHistory[WIND_RECORD_COUNT - 2] + (1 - alpha)*m_ExpSmoothArrayWindDir[WIND_RECORD_COUNT - 2];

            m_ArrayRecTime[WIND_RECORD_COUNT - 1] = wxDateTime::UNow().GetTm( );

            m_oldDirVal = m_ExpSmoothArrayWindDir[WIND_RECORD_COUNT - 1];
            //include the new/latest value in the max/min value test too
            m_MaxWindDir = wxMax(m_WindDir, m_MaxWindDir);
            m_MinWindDir = wxMin(m_WindDir, m_MinWindDir);
            m_MaxWindSpd = wxMax(m_WindSpd, m_MaxWindSpd);
            //get the overall max Wind Speed
            m_TotalMaxWindSpd = wxMax(m_WindSpd, m_TotalMaxWindSpd);
            // Data export  
            ExportData();

            /* set wind angle scale to full +/- 90 deg. depending on
               the real max/min value recorded */
            SetMinMaxWindScale();
        }
    }
}

wxSize DashboardInstrument_WindDirHistory::GetSize( int orient, wxSize hint )
{
      wxClientDC dc(this);
      int w;
      dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
      if( orient == wxHORIZONTAL ) {
        return wxSize( DefaultWidth, wxMax(m_TitleHeight+140, hint.y) );
      }
      else {
        return wxSize( wxMax(hint.x, DefaultWidth), wxMax(m_TitleHeight+140, hint.y) );
      }
}
void DashboardInstrument_WindDirHistory::SetData(
    unsigned long long st,
    double data, wxString unit
    , long long timestamp
    )
{
    if ( ( (st == OCPN_DBP_STC_TWD) || (st == OCPN_DBP_STC_TWS) )
        && (!std::isnan(data))
        ) {
        if (st == OCPN_DBP_STC_TWD) {
            m_TrueWindDir = data;
            if (m_DirRecCnt <= 3){
                m_DirStartVal += data;
                m_DirRecCnt++;
            }
        }
        if (st == OCPN_DBP_STC_TWS && data < 200.0) {
            m_WindSpd = data;
            //convert to knots first
            m_TrueWindSpd = fromUsrSpeed_Plugin(data, g_iDashWindSpeedUnit);
            // if unit changes, reset everything ...
            if (unit != m_WindSpeedUnit && m_WindSpeedUnit != _T("--")) {
                if ( !m_bWindSpeedUnitResetLogged ) {
                    wxLogMessage(
                        "DashboardInstrument_WindDirHistory::SetData() - Reset! - WindSpeedUnit %s, was %s",
                        unit, m_WindSpeedUnit );
                    m_bWindSpeedUnitResetLogged = true; // stop whining
                }
                m_MaxWindDir = -1;
                m_WindDir = -1;
                m_WindDirRange = 90;
                m_MaxWindSpd = 0;
                m_TotalMaxWindSpd = 0;
                m_WindSpd = 0;
                m_SpdRecCnt = 0;
                m_DirRecCnt = 0;
                m_SpdStartVal = -1;
                m_DirStartVal = -1;
                m_IsRunning = false;
                m_SampleCount = 0;
                m_LeftLegend = 3;
                m_RightLegend = 3;
                for (int idx = 0; idx < WIND_RECORD_COUNT; idx++) {
                    m_ArrayWindDirHistory[idx] = -1;
                    m_ArrayWindSpdHistory[idx] = -1;
                    m_ExpSmoothArrayWindSpd[idx] = -1;
                    m_ExpSmoothArrayWindDir[idx] = -1;
                    m_ArrayRecTime[idx] = wxDateTime::Now().GetTm();
                    m_ArrayRecTime[idx].year = 999;
                }
            }
            m_WindSpeedUnit = unit;
            if (m_SpdRecCnt <= 3){
                m_SpdStartVal += data;
                m_SpdRecCnt++;
            }
        }
            if ( m_SpdRecCnt == 3 && m_DirRecCnt == 3) {
            m_WindSpd=  m_SpdStartVal/3;
            m_WindDir = m_DirStartVal/3;
            m_oldDirVal=m_TrueWindDir; // make sure we don't get a diff > or <180 in the initial run
        }
    }
}

void DashboardInstrument_WindDirHistory::Draw(wxGCDC* dc)
{
   m_WindowRect = GetClientRect();
   m_DrawAreaRect=GetClientRect();
   m_DrawAreaRect.SetHeight(m_WindowRect.height-m_TopLineHeight-m_TitleHeight);
   m_DrawAreaRect.SetX (m_LeftLegend+3);
   DrawBackground(dc);
   DrawForeground(dc);
}

//*********************************************************************************
// determine and set  min and max values for the direction
//*********************************************************************************
void DashboardInstrument_WindDirHistory::SetMinMaxWindScale()
{
    /* set wind direction legend to full +/- 90 deg depending on the
       real max/min value recorded,
       example : max wind dir. = 45 deg  ==> max = 90 deg
       min wind dir. = 45deg  ==> min = 0 deg.
       First, calculate the max wind direction */
    int fulldeg = m_MaxWindDir / 90; /* we explicitly chop off the decimals
                                        by type conversion from double
                                        to int ! */
    if(fulldeg == 0)
        fulldeg=m_MaxWindDir<0 ?0:1;
    else if (m_MaxWindDir>0)
        fulldeg+=1;
    m_MaxWindDir=fulldeg*90;
    //now calculate the min wind direction
    fulldeg = m_MinWindDir / 90;
    if(fulldeg == 0)
        fulldeg=m_MinWindDir<0 ?-1:0;
    else
        fulldeg=m_MinWindDir>0 ? fulldeg:(fulldeg-1);
    m_MinWindDir=fulldeg*90;

    /* limit the visible wind dir range to 360 deg; remove the extra range
       on the opposite side of the current wind dir value */
    m_WindDirRange=m_MaxWindDir-m_MinWindDir;
    if (m_WindDirRange > 360) {
        int diff2min=m_WindDir-m_MinWindDir;    /* diff between min value
                                                   and current value */
        int diff2max=m_MaxWindDir-m_WindDir;    /* diff between max value
                                                   and current value */
        if(diff2min > diff2max) {
            while (m_WindDirRange > 360) {
                m_MinWindDir+=90;
                m_WindDirRange=m_MaxWindDir-m_MinWindDir;
            }
        }
        if(diff2min < diff2max) {
            while (m_WindDirRange > 360) {
                m_MaxWindDir-=90;
                m_WindDirRange=m_MaxWindDir-m_MinWindDir;
            }
        }
    }
}
//*********************************************************************************
// wind direction legend
//*********************************************************************************
void  DashboardInstrument_WindDirHistory::DrawWindDirScale(wxGCDC* dc)
{
    wxString label1,label2,label3,label4,label5;
    wxColour cl;
    wxPen pen;
    int width, height;
    cl=wxColour( 204,41,41,255 ); //red, opaque

    dc->SetTextForeground( cl );
    dc->SetFont( *g_pFontSmall );
    if( !m_IsRunning ) {
        label1=_T("---");
        label2=_T("---");
        label3=_T("---");
        label4=_T("---");
        label5=_T("---");
    }
    else {
        // label 1 : legend for bottom line. By definition always w/o decimals
        double tempdir=m_MinWindDir;
        while (tempdir < 0)    tempdir+=360;
        while (tempdir >= 360) tempdir-=360;
        label1=GetWindDirStr(wxString::Format(_T("%.1f"), tempdir));
        // label 2 : 1/4
        tempdir=m_MinWindDir+m_WindDirRange/4.;
        while (tempdir < 0)    tempdir+=360;
        while (tempdir >= 360) tempdir-=360;
        label2=GetWindDirStr(wxString::Format(_T("%.1f"), tempdir));
        // label 3 : legend for center line
        tempdir=m_MinWindDir+m_WindDirRange/2;
        while (tempdir < 0)    tempdir+=360;
        while (tempdir >= 360) tempdir-=360;
        label3=GetWindDirStr(wxString::Format(_T("%.1f"), tempdir));
        // label 4 :  3/4
        tempdir=m_MinWindDir+m_WindDirRange*0.75;
        while (tempdir < 0)    tempdir+=360;
        while (tempdir >= 360) tempdir-=360;
        label4=GetWindDirStr(wxString::Format(_T("%.1f"), tempdir));
        // label 5 : legend for top line
        tempdir=m_MaxWindDir;
        while (tempdir < 0)    tempdir+=360;
        while (tempdir >= 360) tempdir-=360;
        label5=GetWindDirStr(wxString::Format(_T("%.1f"), tempdir));
    }
    //draw the legend with the labels; find the widest string and store it in m_RightLegend.
    // m_RightLegend is the basis for the horizontal lines !
    dc->GetTextExtent(label5, &width, &height, 0, 0, g_pFontSmall);
    m_RightLegend=width;
    dc->GetTextExtent(label4, &width, &height, 0, 0, g_pFontSmall);
    m_RightLegend=wxMax(width,m_RightLegend);
    dc->GetTextExtent(label3, &width, &height, 0, 0, g_pFontSmall);
    m_RightLegend=wxMax(width,m_RightLegend);
    dc->GetTextExtent(label2, &width, &height, 0, 0, g_pFontSmall);
    m_RightLegend=wxMax(width,m_RightLegend);
    dc->GetTextExtent(label1, &width, &height, 0, 0, g_pFontSmall);
    m_RightLegend=wxMax(width,m_RightLegend);

    m_RightLegend+=4; //leave some space to the edge
    dc->DrawText(label5, m_WindowRect.width-m_RightLegend, m_TopLineHeight-height/2);
    dc->DrawText(label4, m_WindowRect.width-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height/4-height/2));
    dc->DrawText(label3, m_WindowRect.width-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height/2-height/2));
    dc->DrawText(label2, m_WindowRect.width-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.75-height/2));
    dc->DrawText(label1, m_WindowRect.width-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height-height/2));
}

//*********************************************************************************
// draw wind speed scale
//*********************************************************************************
void  DashboardInstrument_WindDirHistory::DrawWindSpeedScale(wxGCDC* dc)
{
    wxString label1,label2,label3,label4,label5;
    wxColour cl;
    int width, height;

    cl=wxColour(61,61,204,255);
    dc->SetTextForeground(cl);
    dc->SetFont(*g_pFontSmall);
    //round maxWindSpd up to the next full knot; nicer view ...
    m_MaxWindSpdScale=(int)m_MaxWindSpd + 1;
    if(!m_IsRunning) {
        label1.Printf(_T("--- %s"), m_WindSpeedUnit.c_str());
        label2 = label1;
        label3 = label1;
        label4 = label1;
        label5 = label1;
    }
    else {
        /*we round the speed up to the next full knot ==> the top and bottom line have full numbers as legend (e.g. 23 kn -- 0 kn)
          but the intermediate lines may have decimal values (e.g. center line : 23/2=11.5 or quarter line 23/4=5.75), so in worst case
          we end up with 23 - 17.25 - 11.5 - 5.75 - 0
          The goal is to draw the legend with decimals only, if we really have them !
        */
        // top legend for max wind
        label1.Printf(_T("%.0f %s"), toUsrSpeed_Plugin(m_MaxWindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        // 3/4 legend
        double WindSpdScale=m_MaxWindSpdScale*3./4.;
        // do we need a decimal ?
        double val1=(int)((WindSpdScale-(int)WindSpdScale)*100);
        if(val1==25 || val1==75)  // it's a .25 or a .75
            label2.Printf(_T("%.2f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        else if (val1 == 50)
            label2.Printf(_T("%.1f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        else	
            label2.Printf(_T("%.0f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        // center legend
        WindSpdScale=m_MaxWindSpdScale/2.;
        // center line can either have a .0 or .5 value !
        if((int)(WindSpdScale*10) % 10 == 5)
            label3.Printf(_T("%.1f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        else
            label3.Printf(_T("%.0f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());

        // 1/4 legend
        WindSpdScale=m_MaxWindSpdScale/4.;
        // do we need a decimal ?
        val1=(int)((WindSpdScale-(int)WindSpdScale)*100);
        if(val1==25 || val1==75)
            label4.Printf(_T("%.2f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        else if (val1 == 50)
            label4.Printf(_T("%.1f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
        else
            label4.Printf(_T("%.0f %s"), toUsrSpeed_Plugin(WindSpdScale, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());

        //bottom legend for min wind, always 0
        label5.Printf(_T("%.0f %s"), 0.0, m_WindSpeedUnit.c_str());
    }
    dc->GetTextExtent(label1, &m_LeftLegend, &height, 0, 0, g_pFontSmall);
    dc->DrawText(label1, 4, (int)(m_TopLineHeight + 7 - height/2));
    dc->GetTextExtent(label2, &width, &height, 0, 0, g_pFontSmall);
    dc->DrawText(label2, 4, (int)(m_TopLineHeight+m_DrawAreaRect.height/4-height/2));
    m_LeftLegend = wxMax(width,m_LeftLegend);
    dc->GetTextExtent(label3, &width, &height, 0, 0, g_pFontSmall);
    dc->DrawText(label3, 4, (int)(m_TopLineHeight+m_DrawAreaRect.height/2-height/2));
    m_LeftLegend = wxMax(width,m_LeftLegend);
    dc->GetTextExtent(label4, &width, &height, 0, 0, g_pFontSmall);
    dc->DrawText(label4, 4, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.75-height/2));
    m_LeftLegend = wxMax(width,m_LeftLegend);
    dc->GetTextExtent(label5, &width, &height, 0, 0, g_pFontSmall);
    dc->DrawText(label5, 4,  (int)(m_TopLineHeight+m_DrawAreaRect.height-height/2));
    m_LeftLegend = wxMax(width,m_LeftLegend);
    m_LeftLegend+=4;
}

//*********************************************************************************
//draw background
//*********************************************************************************
void DashboardInstrument_WindDirHistory::DrawBackground(wxGCDC* dc)
{
    wxString label,label1,label2,label3,label4,label5;
    wxColour cl;
    wxPen pen;
    //---------------------------------------------------------------------------------
    // draw legends for speed and direction
    //---------------------------------------------------------------------------------
    DrawWindDirScale(dc);
    DrawWindSpeedScale(dc);

    //---------------------------------------------------------------------------------
    // horizontal lines
    //---------------------------------------------------------------------------------
    GetGlobalColor(_T("UBLCK"), &cl);
    pen.SetColour(cl);
    dc->SetPen(pen);
    dc->DrawLine(m_LeftLegend+3, m_TopLineHeight, m_WindowRect.width-3-m_RightLegend, m_TopLineHeight); // the upper line
    dc->DrawLine(m_LeftLegend+3, (int)(m_TopLineHeight+m_DrawAreaRect.height), m_WindowRect.width-3-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height));
    pen.SetStyle(wxPENSTYLE_DOT);
    dc->SetPen(pen);
    dc->DrawLine(m_LeftLegend+3, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.25), m_WindowRect.width-3-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.25));
    dc->DrawLine(m_LeftLegend+3, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.75), m_WindowRect.width-3-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.75));
#ifdef __WXMSW__
    pen.SetStyle(wxPENSTYLE_SHORT_DASH);
    dc->SetPen(pen);
#endif
    dc->DrawLine(m_LeftLegend+3, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.5), m_WindowRect.width-3-m_RightLegend, (int)(m_TopLineHeight+m_DrawAreaRect.height*0.5));
}

//***********************************************************************************
// convert numerical wind direction values to text, used in the wind direction legend
//***********************************************************************************
wxString DashboardInstrument_WindDirHistory::GetWindDirStr(wxString WindDir)
{
   if ( WindDir == _T("0.0") || WindDir == _T("360.0") )
     return _("N");
   else
   if (WindDir == _T("22.5"))
     return _("NNE");
   else
   if (WindDir == _T("45.0"))
     return _("NE");
   else
   if (WindDir == _T("67.5"))
     return _("ENE");
   else
   if (WindDir == _T("90.0"))
     return _("E");
   else
   if (WindDir == _T("112.5"))
     return _("ESE");
   else
   if (WindDir == _T("135.0"))
     return _("SE");
   else
   if (WindDir == _T("157.5"))
     return _("SSE");
   else
   if (WindDir == _T("180.0"))
     return _("S");
   else
   if (WindDir == _T("202.5"))
     return _("SSW");
   else
   if (WindDir == _T("225.0"))
     return _("SW");
   else
   if (WindDir == _T("247.5"))
     return _("WSW");
   else
   if (WindDir == _T("270.0"))
     return _("W");
   else
   if (WindDir == _T("292.5"))
     return _("WNW");
   else
   if (WindDir == _T("315.0"))
     return _("NW");
   else
   if (WindDir == _T("337.5"))
     return _("NNW");
   else
     return WindDir;
}

//*********************************************************************************
//draw foreground
//*********************************************************************************
void DashboardInstrument_WindDirHistory::DrawForeground(wxGCDC* dc)
{
  wxColour col;
  double ratioH;
  int degw,degh;
  int width,height,sec,min,hour;
  wxString WindAngle,WindSpeed;
  wxPen pen;
  wxString label;

  //---------------------------------------------------------------------------------
  // wind direction
  //---------------------------------------------------------------------------------
  dc->SetFont(*g_pFontData);
  col=wxColour(204,41,41,255); //red, opaque
  dc->SetTextForeground(col);
  if(!m_IsRunning)
    WindAngle=_T("TWD ---");
  else {
    double dir=m_WindDir;
    while(dir > 360) dir-=360;
    while(dir <0 ) dir+=360;
    WindAngle=wxString::Format(_T("TWD %3.0f"), dir)+DEGREE_SIGN;
  }
  dc->GetTextExtent(WindAngle, &degw, &degh, 0, 0, g_pFontData);
  dc->DrawText(WindAngle, m_WindowRect.width-degw-m_RightLegend-3, m_TopLineHeight-degh);
  pen.SetStyle(wxPENSTYLE_SOLID);
  pen.SetColour(wxColour(204,41,41 ,96)); //red, transparent
  pen.SetWidth(1);
  dc->SetPen( pen );
  ratioH = (double) m_DrawAreaRect.height / m_WindDirRange;
  m_DrawAreaRect.SetWidth(m_WindowRect.width - 6 - m_LeftLegend-m_RightLegend);
  m_ratioW = double(m_DrawAreaRect.width) / (WIND_RECORD_COUNT-1);

  //---------------------------------------------------------------------------------
  // live direction data
  //---------------------------------------------------------------------------------
  wxPoint points[WIND_RECORD_COUNT+2], pointAngle_old;
  pointAngle_old.x=3+m_LeftLegend;
  pointAngle_old.y = m_TopLineHeight+m_DrawAreaRect.height - (m_ArrayWindDirHistory[0]-m_MinWindDir) * ratioH;
  for (int idx = 1; idx < WIND_RECORD_COUNT; idx++) {
    points[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
    points[idx].y = m_TopLineHeight+m_DrawAreaRect.height - (m_ArrayWindDirHistory[idx]-m_MinWindDir) * ratioH;
    if(WIND_RECORD_COUNT-m_SampleCount <= idx && points[idx].y > m_TopLineHeight && pointAngle_old.y> m_TopLineHeight && points[idx].y <=m_TopLineHeight+m_DrawAreaRect.height && pointAngle_old.y<=m_TopLineHeight+m_DrawAreaRect.height)
      dc->DrawLine( pointAngle_old.x, pointAngle_old.y, points[idx].x,points[idx].y );
    pointAngle_old.x=points[idx].x;
    pointAngle_old.y=points[idx].y;
  }

  //---------------------------------------------------------------------------------
  //exponential smoothing of direction
  //---------------------------------------------------------------------------------
  pen.SetStyle(wxPENSTYLE_SOLID);
  pen.SetColour(wxColour(204,41,41 ,255));
  pen.SetWidth(2);
  dc->SetPen( pen );
  pointAngle_old.x=3+m_LeftLegend;
  pointAngle_old.y = m_TopLineHeight+m_DrawAreaRect.height - (m_ExpSmoothArrayWindDir[0]-m_MinWindDir) * ratioH;
  for (int idx = 1; idx < WIND_RECORD_COUNT; idx++) {
    points[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
    points[idx].y = m_TopLineHeight+m_DrawAreaRect.height - (m_ExpSmoothArrayWindDir[idx]-m_MinWindDir) * ratioH;
    if(WIND_RECORD_COUNT-m_SampleCount <= idx && points[idx].y > m_TopLineHeight && pointAngle_old.y > m_TopLineHeight && points[idx].y <=m_TopLineHeight+m_DrawAreaRect.height && pointAngle_old.y<=m_TopLineHeight+m_DrawAreaRect.height)
      dc->DrawLine( pointAngle_old.x, pointAngle_old.y, points[idx].x,points[idx].y );
    pointAngle_old.x=points[idx].x;
    pointAngle_old.y=points[idx].y;
  }

  //---------------------------------------------------------------------------------
  // wind speed
  //---------------------------------------------------------------------------------
  col=wxColour(61,61,204,255); //blue, opaque
  dc->SetFont(*g_pFontData);
  dc->SetTextForeground(col);
  WindSpeed = wxString::Format(_T("TWS %3.1f %s "), toUsrSpeed_Plugin(m_WindSpd, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str());
  dc->GetTextExtent(WindSpeed, &degw, &degh, 0, 0, g_pFontData);
  dc->DrawText(WindSpeed, m_LeftLegend+3, m_TopLineHeight-degh);
  dc->SetFont(*g_pFontLabel);
  //determine the time range of the available data (=oldest data value)
  int i=0;
  while( (i < (WIND_RECORD_COUNT - 1)) && (m_ArrayRecTime[i].year == 999) ) i++;
  if (i == (WIND_RECORD_COUNT - 1)) {
    min=0;
    hour=0;
  }
  else {
    wxDateTime localTime( m_ArrayRecTime[i] );
    min = localTime.GetMinute( );
    hour=localTime.GetHour( );
  }
  //Single text var to facilitate correct translations:
  wxString s_Max = _("Max");
  wxString s_Since = _("since");
  wxString s_OMax = _("Overall");
  dc->DrawText(wxString::Format(_T("%s %.1f %s %s %02d:%02d  %s %.1f %s"), s_Max, toUsrSpeed_Plugin(m_MaxWindSpd, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str(), s_Since, hour, min, s_OMax, toUsrSpeed_Plugin(m_TotalMaxWindSpd, g_iDashWindSpeedUnit), m_WindSpeedUnit.c_str()), m_LeftLegend + 3 + 2 + degw, m_TopLineHeight - degh +2);
  pen.SetStyle(wxPENSTYLE_SOLID);
  pen.SetColour(wxColour(61,61,204,96)); //blue, transparent
  pen.SetWidth(1);
  dc->SetPen( pen );
  ratioH = (double)m_DrawAreaRect.height / m_MaxWindSpdScale;
  wxPoint  pointsSpd[WIND_RECORD_COUNT+2],pointSpeed_old;
  pointSpeed_old.x=m_LeftLegend+3;
  pointSpeed_old.y = m_TopLineHeight+m_DrawAreaRect.height - m_ArrayWindSpdHistory[0] * ratioH;

  //---------------------------------------------------------------------------------
  // live speed data
  //---------------------------------------------------------------------------------
  for (int idx = 1; idx < WIND_RECORD_COUNT; idx++) {
    pointsSpd[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
    pointsSpd[idx].y = m_TopLineHeight+m_DrawAreaRect.height - m_ArrayWindSpdHistory[idx] * ratioH;
    if(WIND_RECORD_COUNT-m_SampleCount <= idx && pointsSpd[idx].y > m_TopLineHeight && pointSpeed_old.y > m_TopLineHeight && pointsSpd[idx].y <=m_TopLineHeight+m_DrawAreaRect.height && pointSpeed_old.y<=m_TopLineHeight+m_DrawAreaRect.height)
      dc->DrawLine( pointSpeed_old.x, pointSpeed_old.y, pointsSpd[idx].x,pointsSpd[idx].y );
    pointSpeed_old.x=pointsSpd[idx].x;
    pointSpeed_old.y=pointsSpd[idx].y;
  }

  //---------------------------------------------------------------------------------
  //exponential smoothing of speed
  //---------------------------------------------------------------------------------
  pen.SetStyle(wxPENSTYLE_SOLID);
  pen.SetColour(wxColour(61,61,204,255)); //blue, opaque
  pen.SetWidth(2);
  dc->SetPen( pen );
  pointSpeed_old.x=m_LeftLegend+3;
  pointSpeed_old.y = m_TopLineHeight+m_DrawAreaRect.height - m_ExpSmoothArrayWindSpd[0] * ratioH;
  for (int idx = 1; idx < WIND_RECORD_COUNT; idx++) {
    pointsSpd[idx].x = idx * m_ratioW + 3 + m_LeftLegend;
    pointsSpd[idx].y = m_TopLineHeight+m_DrawAreaRect.height - m_ExpSmoothArrayWindSpd[idx] * ratioH;
    if(WIND_RECORD_COUNT-m_SampleCount <= idx && pointsSpd[idx].y > m_TopLineHeight && pointSpeed_old.y > m_TopLineHeight && pointsSpd[idx].y <=m_TopLineHeight+m_DrawAreaRect.height && pointSpeed_old.y<=m_TopLineHeight+m_DrawAreaRect.height)
      dc->DrawLine( pointSpeed_old.x, pointSpeed_old.y, pointsSpd[idx].x,pointsSpd[idx].y );
    pointSpeed_old.x=pointsSpd[idx].x;
    pointSpeed_old.y=pointsSpd[idx].y;
  }

  //---------------------------------------------------------------------------------
  //draw vertical timelines every 5 minutes
  //---------------------------------------------------------------------------------
  GetGlobalColor(_T("UBLCK"), &col);
  pen.SetColour(col);
  pen.SetStyle(wxPENSTYLE_DOT);
  dc->SetPen(pen);
  dc->SetTextForeground(col);
  dc->SetFont(*g_pFontSmall);
  int done=-1;
  wxPoint pointTime;
  for (int idx = 0; idx < WIND_RECORD_COUNT; idx++) {
      if (m_ArrayRecTime[idx].year != 999) {
          wxDateTime localTime(m_ArrayRecTime[idx]);
          min = localTime.GetMinute( );
          hour=localTime.GetHour( );
          sec=localTime.GetSecond( );
          if ((hour * 100 + min) != done && (min % 5 == 0) && (sec == 0 || sec == 1)) {
              pointTime.x = idx * m_ratioW + 3 + m_LeftLegend;
              dc->DrawLine( pointTime.x, m_TopLineHeight+1, pointTime.x,(m_TopLineHeight+m_DrawAreaRect.height+1) );
              label.Printf(_T("%02d:%02d"), hour,min);
              dc->GetTextExtent(label, &width, &height, 0, 0, g_pFontSmall);
              dc->DrawText(label, pointTime.x-width/2, m_WindowRect.height-height);
              done=hour*100+min;
          }
      }
  }
}

void DashboardInstrument_WindDirHistory::OnLogDataButtonPressed(
    wxCommandEvent& event)
{
 
    if (m_isExporting == false) {
        wxPoint pos;
        m_LogButton->GetSize(&pos.x, &pos.y);
        pos.x = 0;
        this->PopupMenu(m_pExportmenu, pos);
        if(btn1Sec->IsChecked()) m_exportInterval=1;
        if (btn5Sec->IsChecked()) m_exportInterval = 5;
        if (btn10Sec->IsChecked()) m_exportInterval = 10;
        if (btn20Sec->IsChecked()) m_exportInterval = 20;
        if (btn60Sec->IsChecked()) m_exportInterval = 60;

        wxFileDialog fdlg(GetOCPNCanvasWindow(), _("Choose a new or existing file"), wxT(""), m_logfile, wxT("*.*"), wxFD_SAVE);
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
                    "%s%s"  // TWD, separator
                    "%s%s"  // TWS, separator
                    "%s%s"  // Smoothed TWD, separator
                    "%s\n"),// Smoothed TWS
                str_ticks,
                str_utc,
                "      Date", g_sDataExportSeparator,
                " Local Time", g_sDataExportSeparator,
                "  TWD", g_sDataExportSeparator,
                " TWS", g_sDataExportSeparator,
                " sTWD", g_sDataExportSeparator,
                "sTWS");
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
/***************************************************************************************
****************************************************************************************/
bool DashboardInstrument_WindDirHistory::LoadConfig(void)
{
  wxFileConfig *pConf = m_pconfig;

  if (pConf) {
    pConf->SetPath(_T("/PlugIns/DashT/Tactics/Windhistory"));
    pConf->Read(_T("Exportrate"), &m_exportInterval, 5);
    pConf->Read(_T("WindhistoryExportfile"), &m_logfile,wxEmptyString);
    return true;
  }
  else
    return false;
}
/***************************************************************************************
****************************************************************************************/
bool DashboardInstrument_WindDirHistory::SaveConfig(void)
{
  wxFileConfig *pConf = m_pconfig;

  if (pConf)
  {
    pConf->SetPath(_T("/PlugIns/DashT/Tactics/Windhistory"));
    pConf->Write(_T("Exportrate"), m_exportInterval);
    pConf->Write(_T("WindhistoryExportfile"), m_logfile);
    return true;
  }
  else
    return false;
}
void DashboardInstrument_WindDirHistory::ExportData(void)
{
    if ( !m_isExporting )
        return;
    wxDateTime localTime(m_ArrayRecTime[WIND_RECORD_COUNT - 1]);
    if (localTime.GetSecond() % m_exportInterval == 0) {
        wxString str_utc, ticks;
        if (g_bDataExportUTC) {
            wxDateTime utc = localTime.ToUTC();
            str_utc = wxString::Format(_T("%sZ%s"),
                                       utc.FormatISOCombined('T'),
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
        /* Albeit we print on the screen the wind direction with %3.0f
           precision, for export let's have one decimal: %5.1f */
        wxString str = wxString::Format(
            _T(
                "%s"       // ticks
                "%s"       // utc
                "%s%s"     // local date, separator
                "%11s%s"   // local time, separator
                "%5.1f%s"  // TWD, separator
                "%4.1f%s"  // TWS, separator
                "%5.1f%s"  // Smoothed TWD, separator
                "%4.1f\n"),// Smoothed TWS
            ticks,
            str_utc,
            localTime.FormatDate(), g_sDataExportSeparator,
            localTime.FormatTime(), g_sDataExportSeparator,
            m_WindDir, g_sDataExportSeparator,
            toUsrSpeed_Plugin(
                m_WindSpd, g_iDashWindSpeedUnit), g_sDataExportSeparator,
            m_ExpSmoothArrayWindDir[WIND_RECORD_COUNT - 1],
            g_sDataExportSeparator, toUsrSpeed_Plugin(
                m_ExpSmoothArrayWindSpd[WIND_RECORD_COUNT - 1],
                g_iDashWindSpeedUnit) );
        m_ostreamlogfile->Write(str);
    }
}
