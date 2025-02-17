/******************************************************************************
* $Id: RaceStart.h, v1.0 2019/11/30 VaderDarth Exp $
*
* Project:  OpenCPN
* Purpose:  dahboard_tactics_pi plug-in
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

#ifndef __RACESTART_H__
#define __RACESTART_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#define __DERIVEDTIMEOUTJS_OVERRIDE__
#include "InstruJS.h"

#include "ocpn_plugin.h"

/*
  The default window size value are depending both of the HTML-file
  and the JavaScript C++ instrument, here we have set the values to
  the JustGauge scalable, SVG-drawn instrument.
  See enginedjg.css: skpath, gauge and bottom classes for the ratios.
*/
#ifdef __WXMSW__
#define RACESTART_V_TITLEH    18
#define RACESTART_V_WIDTH    900
#define RACESTART_V_HEIGHT   200
#define RACESTART_V_BOTTOM    25
#define RACESTART_H_TITLEH    19
#define RACESTART_H_WIDTH    900
#define RACESTART_H_HEIGHT   200
#define RACESTART_H_BOTTOM    25
#else
#define RACESTART_V_TITLEH    18
#define RACESTART_V_WIDTH    900
#define RACESTART_V_HEIGHT   200
#define RACESTART_V_BOTTOM    25
#define RACESTART_H_TITLEH    19
#define RACESTART_H_WIDTH    900
#define RACESTART_H_HEIGHT   200
#define RACESTART_H_BOTTOM    25
#endif // ifdef __WXMSW__
#define RACESTART_V_MIN_WIDTH  RACESTART_V_WIDTH
#define RACESTART_V_MIN_HEIGHT (RACESTART_V_TITLEH + RACESTART_V_HEIGHT + RACESTART_V_BOTTOM)
#define RACESTART_H_MIN_WIDTH  RACESTART_H_WIDTH
#define RACESTART_H_MIN_HEIGHT (RACESTART_H_TITLEH + RACESTART_H_HEIGHT + RACESTART_H_BOTTOM)

#define RACESTART_WAIT_NEW_HTTP_SERVER_TICKS 2 // if initially no server

#define RACESTART_GUID_STARTLINE_AS_ROUTE "DashT_RaceStart_0001_STARTLINE"
#define RACESTART_NAME_STARTLINE_AS_ROUTE "DashT Startline"
#define RACESTART_NAME_STARTLINE_AS_ROUTE_USER "My Startline"
#define RACESTART_GUID_WP_STARTSTBD "DashT_RaceStart_0001_STARTSTBD"
#define RACESTART_NAME_WP_STARTSTBD "StartS"
#define RACESTART_NAME_WP_STARTSTBD_USER "MyStartS"
#define RACESTART_GUID_WP_STARTPORT "DashT_RaceStart_0001_STARTPORT"
#define RACESTART_NAME_WP_STARTPORT "StartP"
#define RACESTART_NAME_WP_STARTPORT_USER "MyStartP"

// These values for first time start only
#define RACESTART_LAYLINE_WIDTH 3 // pen width
#define RACESTART_GRID_SIZE 1. // in nautical miles
#define RACESTART_GRID_STEP 0.026998 // 50 meters in nautical miles
#define RACESTART_GRID_STEP_MINIMUM 0.00486 // 9 meters in nautical miles
#define RACESTART_GRID_BOLD_INTERVAL 2 // 1 all bold, 2 every 2nd bold, etc.
#define RACESTART_GRID_LINE_WIDTH 1 // pen width
#define RACESTART_ZERO_BURN_BY_POLAR_SECONDS 60
#define RACESTART_ZERO_BURN_BY_POLAR_SECONDS_UPPER_LIMIT 240
#define RACESTART_ZERO_BURN_BY_POLAR_SECONDS_LOWER_LIMIT 30 // below, will turn to zero = disable

#define RACESTART_COG_MAX_JITTER 5 // in degrees, otherwise cannot calculate
#define RACESTART_USER_MOVING_WP_GRACETIME_CNT 10 // about ten seconds

//+------------------------------------------------------------------------
//|
//| CLASS:
//|    DashboardInstrument_RaceStart
//|
//| DESCRIPTION:
//|    This instrument provides assistance for race start line tactics
//+------------------------------------------------------------------------

class DashboardInstrument_RaceStart : public InstruJS
{
public:
    DashboardInstrument_RaceStart(
        TacticsWindow *pparent, wxWindowID id, wxString ids,
        PI_ColorScheme cs, wxString format = "", bool isInit = false  );
    ~DashboardInstrument_RaceStart(void);
    void SetData(
        unsigned long long, double, wxString,
        long long timestamp=0LL ) override {;};
#ifndef __RACESTART_DERIVEDTIMEOUT_OVERRIDE__
    virtual void derived2TimeoutEvent(void){};
#else
    virtual void derived2TimeoutEvent(void) = 0;
#endif // __RACESTART_DERIVEDTIMEOUT_OVERRIDE__
    virtual void derivedTimeoutEvent(void) override;
    bool CheckForValidStartLineGUID( wxString sGUID, wxString lineName,
                                     wxString portName, wxString stbdName);
    bool CheckForValidUserSetStartLine(void);
    bool CheckForPreviouslyDroppedStartLine(void);
    bool CheckStartLineStillValid(void);
    bool CheckForMovedUserDroppedWaypoints(void);

    virtual bool instruIsReady(void) override;
    virtual bool userHasStartline(void) override;
    virtual bool dropStarboardMark(void) override;
    virtual bool dropPortMark(void) override;
    virtual bool sendSlData(void) override;
    virtual bool stopSlData(void) override;
    virtual void getSlData( wxString& sCogDist, wxString& sDist,
                            wxString& sBias, wxString& sAdv ) override;
   
    virtual wxSize GetSize( int orient, wxSize hint ) override;
    void DoRenderGLOverLay(wxGLContext* pcontext, PlugIn_ViewPort* vp );
    virtual void PushTwaHere(
        double data, wxString unit, long long timestamp=0LL);
    virtual void PushTwsHere(
        double data, wxString unit, long long timestamp=0LL);
    virtual void PushCogHere(
        double data, wxString unit, long long timestamp=0LL);
    virtual void PushLatHere(
        double data, wxString unit, long long timestamp=0LL);
    virtual void PushLonHere(
        double data, wxString unit, long long timestamp=0LL);
    
protected:
    TacticsWindow       *m_pparent;
    int                  m_orient;
    bool                 m_htmlLoaded;
    wxTimer             *m_pThreadRaceStartTimer;
    std::mutex           m_mtxRaceStartRun;
    bool                 m_closingRaceStart;
    int                  m_goodHttpServerDetects;
    wxFileConfig        *m_pconfig;
    wxString             m_fullPathHTML;
    wxString             m_httpServer;
    wxString             m_sStartLineAsRouteGuid;
    PlugIn_Route        *m_startLineAsRoute;
    wxString             m_sStartStbdWpGuid;
    PlugIn_Waypoint     *m_startStbdWp;
    wxString             m_sStartPortWpGuid;
    PlugIn_Waypoint     *m_startPortWp;
    int                  m_startPointMoveDelayCount;
    wxString             m_sRendererCallbackUUID;
    glRendererFunction   m_rendererIsHere;
    bool                 m_renStartLineDrawn;
    wxPoint              m_renPointStbd;
    wxPoint              m_renPointPort;
    bool                 m_renWindBiasDrawn;
    PlugIn_Waypoint     *m_startWestWp;
    PlugIn_Waypoint     *m_startEastWp;
    wxPoint              m_renPointWest;
    wxRealPoint          m_renRealPointWest;
    wxPoint              m_renPointEast;
    wxRealPoint          m_renRealPointEast;
    bool                 m_renbNorthSector;
    double               m_renSlineLength;
    double               m_renSlineDir;
    double               m_renOppositeSlineDir;
    double               m_renBiasSlineDir;
    double               m_renWindBias;
    double               m_renWindBiasAdvDist;
    double               m_renWindBiasAdvDir;
    double               m_renWindBiasLineDir;
    wxPoint              m_renPointBiasStart;
    wxPoint              m_renPointBiasStop;
    bool                 m_renDrawLaylines;
    int                  m_renLaylineWidth;
    bool                 m_renLaylinesCalculated;
    bool                 m_renLaylinesDrawn;
    double               m_renGridBoxDir;
    double               m_renGridDirEast;
    double               m_renGridDirWest;
    double               m_renGridEndOffset;
    double               m_renGridLineMaxLen;
    double               m_gridStepWestOnStartline;
    double               m_gridStepEastOnStartline;
    double               m_gridStepEastOnWestEdge;
    double               m_gridStepEastOnEastEdge;
    double               m_renGridEndPointStartlineWest_lat;
    double               m_renGridEndPointStartlineWest_lon;
    wxRealPoint          m_renGridEndRealPointStartlineWest;
    double               m_renGridEndPointStartlineEast_lat;
    double               m_renGridEndPointStartlineEast_lon;
    wxRealPoint          m_renGridEndRealPointStartlineEast;
    double               m_renGridEndPointOtherWest_lat;
    double               m_renGridEndPointOtherWest_lon;
    wxRealPoint          m_renGridEndRealPointOtherWest;
    double               m_renGridEndPointOtherEast_lat;
    double               m_renGridEndPointOtherEast_lon;
    wxRealPoint          m_renGridEndRealPointOtherEast;
    bool                 m_renGridBoxCalculated;
    double               m_renLLPortDir;
    double               m_renLLStbdDir;
    double               m_renGridSize;
    double               m_renGridStep;
    int                  m_renGridBoldInterval;
    int                  m_renGridLineWidth;
    bool                 m_renDrawGrid;
    bool                 m_renGridDrawn;
    int                  m_renZeroBurnSeconds;
    double               m_renDistanceToStartLine;
    double               m_renDistanceCogToStartLine;
    bool                 m_renIsOnWrongSide;
    wxRealPoint          m_renCogCrossingStartlineRealPoint;
    double               m_renCogCrossingStartlinePoint_lat;
    double               m_renCogCrossingStartlinePoint_lon;
    double               m_renPolarDistance;
    bool                 m_renZeroBurnDrawn;
    bool                 m_dataRequestOn;
    bool                 m_jsCallBackAsHeartBeat;
    bool                 m_userDropsMarks;
    bool                 m_suggestedOldMarks;
    
    callbackFunction     m_pushTwaHere;
    wxString             m_pushTwaUUID;
    double               m_Twa;
    callbackFunction     m_pushTwsHere;
    wxString             m_pushTwsUUID;
    double               m_Tws;
    callbackFunction     m_pushCogHere;
    wxString             m_pushCogUUID;
    double               m_Cog;
    callbackFunction     m_pushLatHere;
    wxString             m_pushLatUUID;
    double               m_Lat;
    callbackFunction     m_pushLonHere;
    wxString             m_pushLonUUID;
    double               m_Lon;


    wxDECLARE_EVENT_TABLE();

    void ClearRoutesAndWPs(void);
    void ClearRendererCalcs(void);
    void OnThreadTimerTick(wxTimerEvent& event);
    void OnClose(wxCloseEvent& event);
    bool LoadConfig(void);
    void SaveConfig(void);
    void RenderGLStartLine(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    void RenderGLWindBias(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    void RenderGLLaylines(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    bool CalculateGridBox(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    bool IsSlineWbiasLaylinesGridbox(void);
    void RenderGLGrid(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    void RenderGLZeroBurn(wxGLContext* pcontext, PlugIn_ViewPort* vp);
    bool IsAllMeasurementDataValid(void);
    void CalculateDistancesToStartlineGLDot(
        wxGLContext* pcontext, PlugIn_ViewPort* vp);
};

#endif // __RACESTART_H__
