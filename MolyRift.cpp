//------------------------------------------------------------------------------
// <copyright file="BodyBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// for debugging
#include <iostream>
#include <windows.h>
#include <string>





//Kinect
#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include "BodyBasics.h"

//Oculus Rift

// Choose whether the SDK performs rendering/distortion, or the application.
#define SDK_RENDER 1


#include "Win32_DX11AppUtil.h"         // Include Non-SDK supporting utilities
#include "OVR_CAPI.h"                  // Include the OculusVR SDK



ovrHmd           HMD;                  // The handle of the headset
ovrEyeRenderDesc EyeRenderDesc[2];     // Description of the VR.
ovrRecti         EyeRenderViewport[2]; // Useful to remember when varying resolution
ImageBuffer    * pEyeRenderTexture[2]; // Where the eye buffers will be rendered
ImageBuffer    * pEyeDepthBuffer[2];   // For the eye buffers to use when rendered
ovrPosef         EyeRenderPose[2];     // Useful to remember where the rendered eye originated
float            YawAtRender[2];       // Useful to remember where the rendered eye originated
float            Yaw(3.141592f);       // Horizontal rotation of the player
Vector3f         Pos(0.0f, 1.6f, -5.0f); // Position of player

#include "Win32_RoomTiny_ExampleFeatures.h" // Include extra options to show some simple operations

#if SDK_RENDER
#define   OVR_D3D_VERSION 11
#include "OVR_CAPI_D3D.h"                   // Include SDK-rendered code for the D3D version
#endif

//Kinect
float diffx = 0.0;
float diffy = 0.0;
float diffz = 0.0;
CameraSpacePoint OldRightHandPos;
bool tracking = false;

//-------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR lpCmdLine,_In_ int nShowCmd)
{
	//Kinect
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CBodyBasics kinect;
	kinect.InitializeDefaultSensor();


	// Initializes LibOVR, and the Rift
	ovr_Initialize();
	HMD = ovrHmd_Create(0);

	if (!HMD)                       { MessageBoxA(NULL, "Oculus Rift not detected.", "", MB_OK); return(0); }
	if (HMD->ProductName[0] == '\0')  MessageBoxA(NULL, "Rift detected, display not enabled.", "", MB_OK);

	// Setup Window and Graphics - use window frame if relying on Oculus driver
	bool windowed = (HMD->HmdCaps & ovrHmdCap_ExtendDesktop) ? false : true;
	if (!DX11.InitWindowAndDevice(hInstance, Recti(HMD->WindowsPos, HMD->Resolution), windowed))
		return(0);

	DX11.SetMaxFrameLatency(1);
	ovrHmd_AttachToWindow(HMD, DX11.Window, NULL, NULL);
	ovrHmd_SetEnabledCaps(HMD, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

	// Start the sensor which informs of the Rift's pose and motion
	ovrHmd_ConfigureTracking(HMD, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);

	// Make the eye render buffers (caution if actual size < requested due to HW limits). 
	for (int eye = 0; eye<2; eye++)
	{
		Sizei idealSize = ovrHmd_GetFovTextureSize(HMD, (ovrEyeType)eye,
			HMD->DefaultEyeFov[eye], 1.0f);
		pEyeRenderTexture[eye] = new ImageBuffer(true, false, idealSize);
		pEyeDepthBuffer[eye] = new ImageBuffer(true, true, pEyeRenderTexture[eye]->Size);
		EyeRenderViewport[eye].Pos = Vector2i(0, 0);
		EyeRenderViewport[eye].Size = pEyeRenderTexture[eye]->Size;
	}

	// Setup VR components
#if SDK_RENDER
	ovrD3D11Config d3d11cfg;
	d3d11cfg.D3D11.Header.API = ovrRenderAPI_D3D11;
	d3d11cfg.D3D11.Header.BackBufferSize = Sizei(HMD->Resolution.w, HMD->Resolution.h);
	d3d11cfg.D3D11.Header.Multisample = 1;
	d3d11cfg.D3D11.pDevice = DX11.Device;
	d3d11cfg.D3D11.pDeviceContext = DX11.Context;
	d3d11cfg.D3D11.pBackBufferRT = DX11.BackBufferRT;
	d3d11cfg.D3D11.pSwapChain = DX11.SwapChain;

	if (!ovrHmd_ConfigureRendering(HMD, &d3d11cfg.Config,
		ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette |
		ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive,
		HMD->DefaultEyeFov, EyeRenderDesc))
		return(1);

#endif

	// Create the room model
	Scene roomScene(false); // Can simplify scene further with parameter if required.

	// MAIN LOOP
	// =========
	while (!(DX11.Key['Q'] && DX11.Key[VK_CONTROL]) && !DX11.Key[VK_ESCAPE])
	{
		DX11.HandleMessages();

		float       speed = 1.0f; // Can adjust the movement speed. 
		int         timesToRenderScene = 1;    // Can adjust the render burden on the app.
		ovrVector3f useHmdToEyeViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset,
			EyeRenderDesc[1].HmdToEyeViewOffset };
		// Start timing
#if SDK_RENDER
		ovrHmd_BeginFrame(HMD, 0);
#else
		ovrHmd_BeginFrameTiming(HMD, 0);
#endif
		kinect.Update();

		// Handle key toggles for re-centering, meshes, FOV, etc.
		ExampleFeatures1(&speed, &timesToRenderScene, useHmdToEyeViewOffset);

		// Keyboard inputs to adjust player orientation
		if (DX11.Key[VK_LEFT])  Yaw += 0.02f;
		if (DX11.Key[VK_RIGHT]) Yaw -= 0.02f;

		// Keyboard inputs to adjust player position
		if (DX11.Key['W'] || DX11.Key[VK_UP]) {
			Pos += Matrix4f::RotationY(Yaw).Transform(Vector3f(0, 0, -speed*0.05f));	
		}
		if (DX11.Key['S'] || DX11.Key[VK_DOWN]){
			Pos += Matrix4f::RotationY(Yaw).Transform(Vector3f(0, 0, +speed*0.05f));
		}
		if (DX11.Key['D']){
			Pos += Matrix4f::RotationY(Yaw).Transform(Vector3f(+speed*0.05f, 0, 0));
		}
		if (DX11.Key['A']){
			Pos += Matrix4f::RotationY(Yaw).Transform(Vector3f(-speed*0.05f, 0, 0));
		}
		Pos.y = ovrHmd_GetFloat(HMD, OVR_KEY_EYE_HEIGHT, Pos.y);

		//Move items according to kinect movement
		if (tracking){

			std::string s = "\n" + std::to_string(diffy);
			OutputDebugStringA(s.c_str());
			roomScene.Models[0]->Pos += Matrix4f::RotationY(Yaw).Transform(Vector3f(+speed*diffx, +speed*diffy, +speed*diffz));
		}
		

		// Get both eye poses simultaneously, with IPD offset already included. 
		ovrPosef temp_EyeRenderPose[2];
		ovrHmd_GetEyePoses(HMD, 0, useHmdToEyeViewOffset, temp_EyeRenderPose, NULL);

		// Render the two undistorted eye views into their render buffers.  
		for (int eye = 0; eye < 2; eye++)
		{
			ImageBuffer * useBuffer = pEyeRenderTexture[eye];
			ovrPosef    * useEyePose = &EyeRenderPose[eye];
			float       * useYaw = &YawAtRender[eye];
			bool          clearEyeImage = true;
			bool          updateEyeImage = true;

			// Handle key toggles for half-frame rendering, buffer resolution, etc.
			ExampleFeatures2(eye, &useBuffer, &useEyePose, &useYaw, &clearEyeImage, &updateEyeImage);

			if (clearEyeImage)
				DX11.ClearAndSetRenderTarget(useBuffer->TexRtv,
				pEyeDepthBuffer[eye], Recti(EyeRenderViewport[eye]));
			if (updateEyeImage)
			{
				// Write in values actually used (becomes significant in Example features)
				*useEyePose = temp_EyeRenderPose[eye];
				*useYaw = Yaw;

				// Get view and projection matrices (note near Z to reduce eye strain)
				Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
				Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
				Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
				Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
				Vector3f shiftedEyePos = Pos + rollPitchYaw.Transform(useEyePose->Position);

				Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
				Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov, 0.2f, 1000.0f, true);

				// Render the scene
				for (int t = 0; t<timesToRenderScene; t++)
					roomScene.Render(view, proj.Transposed());
			}
		}

		// Do distortion rendering, Present and flush/sync
#if SDK_RENDER    
		ovrD3D11Texture eyeTexture[2]; // Gather data for eye textures 
		for (int eye = 0; eye<2; eye++)
		{
			eyeTexture[eye].D3D11.Header.API = ovrRenderAPI_D3D11;
			eyeTexture[eye].D3D11.Header.TextureSize = pEyeRenderTexture[eye]->Size;
			eyeTexture[eye].D3D11.Header.RenderViewport = EyeRenderViewport[eye];
			eyeTexture[eye].D3D11.pTexture = pEyeRenderTexture[eye]->Tex;
			eyeTexture[eye].D3D11.pSRView = pEyeRenderTexture[eye]->TexSv;
		}
		ovrHmd_EndFrame(HMD, EyeRenderPose, &eyeTexture[0].Texture);

#endif
	}

	// Release and close down
	ovrHmd_Destroy(HMD);
	ovr_Shutdown();
	DX11.ReleaseWindow(hInstance);

	return(0);
}

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
//int APIENTRY wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR lpCmdLine,_In_ int nShowCmd)
//{
//    UNREFERENCED_PARAMETER(hPrevInstance);
//    UNREFERENCED_PARAMETER(lpCmdLine);
//
//    CBodyBasics application;
//    application.Run(hInstance, nShowCmd);
//}

/// <summary>
/// Constructor
/// </summary>
CBodyBasics::CBodyBasics() :
    m_hWnd(NULL),
    m_nStartTime(0),
    m_nLastCounter(0),
    m_nFramesSinceUpdate(0),
    m_fFreq(0),
    m_nNextStatusTime(0LL),
    m_pKinectSensor(NULL),
    m_pCoordinateMapper(NULL),
    m_pBodyFrameReader(NULL),
    m_pD2DFactory(NULL),
    m_pRenderTarget(NULL),
    m_pBrushJointTracked(NULL),
    m_pBrushJointInferred(NULL),
    m_pBrushBoneTracked(NULL),
    m_pBrushBoneInferred(NULL),
    m_pBrushHandClosed(NULL),
    m_pBrushHandOpen(NULL),
    m_pBrushHandLasso(NULL)
{
    LARGE_INTEGER qpf = {0};
    if (QueryPerformanceFrequency(&qpf))
    {
        m_fFreq = double(qpf.QuadPart);
    }
}
  

/// <summary>
/// Destructor
/// </summary>
CBodyBasics::~CBodyBasics()
{

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    // done with body frame reader
    SafeRelease(m_pBodyFrameReader);

    // done with coordinate mapper
    SafeRelease(m_pCoordinateMapper);

    // close the Kinect Sensor
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    SafeRelease(m_pKinectSensor);
}


/// <summary>
/// Main processing function
/// </summary>
void CBodyBasics::Update()
{

    if (!m_pBodyFrameReader)
    {
        return;
    }

    IBodyFrame* pBodyFrame = NULL;

    HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);

    if (SUCCEEDED(hr))
    {
        INT64 nTime = 0;

        hr = pBodyFrame->get_RelativeTime(&nTime);

        IBody* ppBodies[BODY_COUNT] = {0};

        if (SUCCEEDED(hr))
        {
            hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
        }

        if (SUCCEEDED(hr))
        {
            ProcessBody(nTime, BODY_COUNT, ppBodies);
        }

        for (int i = 0; i < _countof(ppBodies); ++i)
        {
            SafeRelease(ppBodies[i]);
        }
    }

    SafeRelease(pBodyFrame);
}


/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
void CBodyBasics::InitializeDefaultSensor()
{
    HRESULT hr;

    hr = GetDefaultKinectSensor(&m_pKinectSensor);
    if (FAILED(hr))
    {
        return;
    }

    if (m_pKinectSensor)
    {
        // Initialize the Kinect and get coordinate mapper and the body reader
        IBodyFrameSource* pBodyFrameSource = NULL;

        hr = m_pKinectSensor->Open();

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);
        }

        if (SUCCEEDED(hr))
        {
            hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
        }

        SafeRelease(pBodyFrameSource);
    }

}

/// <summary>
/// Calculates the movement step from Kinect coordinates to Oculus rift coordinates
/// <param name="f1">New coordinate</param>
/// <param name="f2">Old coordinate</param>
/// <returns>The Rift coordinate</returns>
/// </summary>
float CBodyBasics::calcDiff(float f1, float f2){
	int sign = (f1 > f2) ? 1 : -1;
	float diff = abs(f1 - f2)*50;
	if (diff > 1){
		diff = 1.0;
	}
	else if (diff < 0.001){  // to make the movement less jumpy
		diff = 0.0;
	}


	return diff*sign;
}

/// <summary>
/// Handle new body data
/// <param name="nTime">timestamp of frame</param>
/// <param name="nBodyCount">body data count</param>
/// <param name="ppBodies">body data in frame</param>
/// </summary>
void CBodyBasics::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{
		tracking = false; //only move objects if the user signals it


        if (m_pCoordinateMapper)
        {


            for (int i = 0; i < nBodyCount; ++i)
            {
                IBody* pBody = ppBodies[i];
                if (pBody)
                {
                    BOOLEAN bTracked = false;
					HRESULT hr = pBody->get_IsTracked(&bTracked);

                    if (SUCCEEDED(hr) && bTracked)
                    {
                        Joint joints[JointType_Count]; 
                        HandState leftHandState = HandState_Unknown;
                        HandState rightHandState = HandState_Unknown;
                        pBody->get_HandLeftState(&leftHandState);
                        pBody->get_HandRightState(&rightHandState);

                        hr = pBody->GetJoints(_countof(joints), joints);
                        if (SUCCEEDED(hr))
                        {
							
							CameraSpacePoint rightHandPos;
                            for (int j = 0; j < _countof(joints); ++j)
                            {
								if (joints[j].JointType == JointType_HandRight){
									// saves the hand position if the hand is closed
									rightHandPos = joints[j].Position;

									diffx = calcDiff(rightHandPos.X, OldRightHandPos.X);
									diffy = calcDiff(rightHandPos.Y, OldRightHandPos.Y);
									diffz = calcDiff(rightHandPos.Z, OldRightHandPos.Z);
									OldRightHandPos = rightHandPos;


								}

                            }
							//OutputDebugString(L"\nHeeeej");
							switch (rightHandState)
							{
							case HandState_Closed:														
								tracking = true; //move objects
								break;

							case HandState_Open:

								break;

							case HandState_Lasso:

								break;
							}

                        }
                    }
                }
            }

        }

        if (!m_nStartTime)
        {
            m_nStartTime = nTime;
        }

}







