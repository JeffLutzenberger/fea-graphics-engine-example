#pragma once

#include "Model.h"
#include "Project.h"
#include "WinFiltr.h"

class CGraphicView;

class  CGraphicsEngine {
public:
	CGraphicsEngine();
	~CGraphicsEngine();

	// functions for the world..currently not using the thread capability
	// should consider this in the future
	bool startUpMultiThreadingEngine( CGraphicView* pWind );
	void doThreadedTasks(void);
	bool engineIsActive(void) { return m_bEngineActive; };
	void startBackgroundEngine( void );
	void stopBackgroundEngine( void );

	void CreateExtrusions(const CWindowFilter& filter);
	void ClearDisplacedExtrusions( const CResultCase* pResultCase );
	void CreateDisplacedExtrusions( const CResultCase* pResultCase, 
		CWindowFilter& filter, 
		double distFactor, 
		double memberMin, 
		double memberMax, 
		double cableMin, 
		double cableMax );

private:
	// time reference for data which engine is sensitive to
	clock_t			m_engineDataTime;
	void  UpdateDataTime( );
	clock_t  DataTime( );

	bool  m_bEngineActive;
	HANDLE	m_hTaskManagerHandle;
	DWORD	   m_dwThreadID;  // ID of the background thread
	bool m_bRunBackgroundEngine;
	CGraphicView* m_parentGraphicsWindow;
	bool m_bExtrusionsCreated;

};	// end class CGraphicsEngine

extern CGraphicsEngine theGraphicsEngine;