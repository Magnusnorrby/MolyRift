//------------------------------------------------------------------------------
// <copyright file="BodyBasics.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "resource.h"

class CBodyBasics
{
    static const int        cDepthWidth  = 512;
    static const int        cDepthHeight = 424;

public:
    /// <summary>
    /// Constructor
    /// </summary>
    CBodyBasics();

    /// <summary>
    /// Destructor
    /// </summary>
    ~CBodyBasics();


	/// <summary>
	/// Initializes the default Kinect sensor
	/// </summary>
	void                 InitializeDefaultSensor();

	/// <summary>
	/// Main processing function
	/// </summary>
	void                    Update();

	/// <summary>
	/// Calculates the movement step from Kinect coordinates to Oculus rift coordinates
	/// <param name="f1">New coordinate</param>
	/// <param name="f2">Old coordinate</param>
	/// <returns>The Rift coordinate</returns>
	/// </summary>
	float                    calcDiff(float f1, float f2);

private:
    HWND                    m_hWnd;
    INT64                   m_nStartTime;
    INT64                   m_nLastCounter;
    double                  m_fFreq;
    INT64                   m_nNextStatusTime;
    DWORD                   m_nFramesSinceUpdate;

    // Current Kinect
    IKinectSensor*          m_pKinectSensor;
    ICoordinateMapper*      m_pCoordinateMapper;

    // Body reader
    IBodyFrameReader*       m_pBodyFrameReader;

    // Direct2D
    ID2D1Factory*           m_pD2DFactory;

    // Body/hand drawing
    ID2D1HwndRenderTarget*  m_pRenderTarget;
    ID2D1SolidColorBrush*   m_pBrushJointTracked;
    ID2D1SolidColorBrush*   m_pBrushJointInferred;
    ID2D1SolidColorBrush*   m_pBrushBoneTracked;
    ID2D1SolidColorBrush*   m_pBrushBoneInferred;
    ID2D1SolidColorBrush*   m_pBrushHandClosed;
    ID2D1SolidColorBrush*   m_pBrushHandOpen;
    ID2D1SolidColorBrush*   m_pBrushHandLasso;




    
    /// <summary>
    /// Handle new body data
    /// <param name="nTime">timestamp of frame</param>
    /// <param name="nBodyCount">body data count</param>
    /// <param name="ppBodies">body data in frame</param>
    /// </summary>
    void                    ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies);


};

