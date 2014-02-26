// GraphicsEngine.cpp : implementation file


#include "stdafx.h"
#pragma hdrstop

#include "GraphicsEngine.h"
#include "GraphicsWindow.h"
#include "Graphics/GraphicsMember.h"

CGraphicsEngine theGraphicsEngine;
DWORD WINAPI graphicsThreadedTaskManager( LPVOID lpvThreadParm );

CGraphicsEngine::CGraphicsEngine():   // constructor for the multithreaded version of the engine
m_bExtrusionsCreated(false)
{
	// initialize our member variables
	m_bEngineActive = false;
	m_dwThreadID = 0;
	m_hTaskManagerHandle = NULL;
	m_bRunBackgroundEngine = false;
	m_parentGraphicsWindow = NULL;
}

CGraphicsEngine::~CGraphicsEngine()
{
	// shut down the background task manager thread
	if( m_hTaskManagerHandle ) {
		TerminateThread( m_hTaskManagerHandle, 0 );
		CloseHandle( m_hTaskManagerHandle );
	}
	return;
}

void CGraphicsEngine::UpdateDataTime( ) 
{ 
	m_engineDataTime = clock(); 
}
	
clock_t  CGraphicsEngine::DataTime( ) 
{ 
	return m_engineDataTime; 
}

bool CGraphicsEngine::startUpMultiThreadingEngine( CGraphicView* pWind )
{
	// now set up the background task manager thread
//	int  param = 0;
	if( m_hTaskManagerHandle == NULL )
	{
		m_hTaskManagerHandle = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)graphicsThreadedTaskManager, ( LPVOID )this, 
			CREATE_SUSPENDED, &m_dwThreadID );

		if( m_hTaskManagerHandle )
		{
			SetThreadPriority( m_hTaskManagerHandle, THREAD_PRIORITY_LOWEST );
			// may want THREAD_PRIORITY_IDLE if this takes too much time from system
		}
		else
		{
			return false;
			//AfxMessageBox( "Unable to start background thread in engine constructor!", MB_ICONEXCLAMATION | MB_OK ); 
		}
	}
	m_parentGraphicsWindow = pWind;
	ResumeThread( m_hTaskManagerHandle );
	return true;
}  // startUpMultiThreadingEngine()

void CGraphicsEngine::doThreadedTasks(void)
{
	if( !m_bRunBackgroundEngine )  // are we turned on?
		return;

	m_bEngineActive = false;
	clock_t startTime = clock();
	while( m_parentGraphicsWindow )  // so break gets us out easily!!
	{
			m_bEngineActive = true;  // we are "in the loop"
			// start off by looking at the parentWindow's graphic member map and update calculations there
			//GraphicsMemberMap::const_iterator iter;
			//for (iter=m_parentGraphicsWindow->m_GraphicsMemberMap.begin(); 
			//	iter != m_parentGraphicsWindow->m_GraphicsMemberMap.end(); ++iter)  {
			//	// check for member array time change and if one has occurred break out
			//	if( m_parentGraphicsWindow->DataTime() > startTime ) // time has changed
			//		break;
			//	CGraphicsMember* pM = m_parentGraphicsWindow->m_GraphicsMemberMap[iter->first];
			//	if( pM && !pM->bHaveExtrusions() ) {  // we've got work to do
			//		pM->SetupExtrusions();
			//	}
			//	else if( pM == NULL )
			//		break;
			//}
			//displaced shape
			//for (iter=m_parentGraphicsWindow->m_GraphicsDispMemberMap.begin(); 
			//	iter != m_parentGraphicsWindow->m_GraphicsDispMemberMap.end(); ++iter)  {
			//		// check for member array time change and if one has occurred break out
			//		if( m_parentGraphicsWindow->DataTime() > startTime ) // time has changed
			//			break;
			//		CGraphicsMember* pM = m_parentGraphicsWindow->m_GraphicsDispMemberMap[iter->first];
			//		if( pM && !pM->bHaveExtrusions() ) {  // we've got work to do
			//			pM->SetupExtrusions();
			//		}
			//		else if( pM == NULL )
			//			break;
			//}
			if( m_parentGraphicsWindow->DataTime() > startTime ) // time has changed
				break;
			// continue on with other future tasks here - make sure to check time in computation loops and break as needed
			// ...


			// we are all done, update the data's time stamp 
			m_parentGraphicsWindow->UpdateDataTime();
			break;  // done
	}
	m_bEngineActive = false;
	return;
}


void CGraphicsEngine::startBackgroundEngine( void )
{

	//if( m_hTaskManagerHandle )
	//{
	//	int  count = 2;
	//	while( count > 1 )  // make sure we are running since we start up the thread suspended
	//	{
	//		count = ResumeThread( m_hTaskManagerHandle );
	//	}
		m_bRunBackgroundEngine = true;
	//}
}


void CGraphicsEngine::stopBackgroundEngine( void )
{
	//if( m_hTaskManagerHandle )
	//{
		// update the data time stamp, then wait for the process loop to break out
		if( m_parentGraphicsWindow )
			m_parentGraphicsWindow->UpdateDataTime();
		// while loop was causing hangs in VA - JL 4/24/2006
		//while( m_bEngineActive ) {
		//	Sleep(0);
		//}
//		if( SuspendThread( m_hTaskManagerHandle ) != -1 )
		m_bRunBackgroundEngine = false;
	//}
}

#include "Core\UI\ProgressDlg.h"
void CGraphicsEngine::CreateExtrusions(const CWindowFilter& filter)
{
	//if( filter.drawDetail == DETAIL_LOW )
	//	return;
	//called from the graphic view constructor
	bool aborted = false;
	m_bExtrusionsCreated = false;
	int nMembers = theModel.elements( MEMBER_ELEMENT );
	int nCables = theModel.elements( CABLE_ELEMENT );
	if( nMembers + nCables > 0 ){
		//count the number of members that need extrusions. if it's more than 10
		//then put up a progress dialog otherwise it should be ok to 
		//omit it (and thus avoid having the dialog box flash on the screen )
		int membersWithoutGraphics = 0;
		int cablesWithoutGraphics = 0;
		for( int i = 1; i <= theModel.elements( MEMBER_ELEMENT ); i++ )
		{
			CMember* pM = (CMember*)(theModel.element( MEMBER_ELEMENT, i ));
			if( !pM->mpGraphicsMember )
				membersWithoutGraphics++;
		}
		for( int i = 1; i <= theModel.elements( CABLE_ELEMENT ); i++ )
		{
			CCable* pC = (CCable*)(theModel.element( CABLE_ELEMENT, i ));
			if( !pC->mpGraphicsCable )
				cablesWithoutGraphics++;
		}
		bool doProgress = false;
		if( membersWithoutGraphics + cablesWithoutGraphics > 10 )
			doProgress = true;
		if( doProgress )
		{
			double incValue = 1./(double)(nMembers + nCables);
			StartProgressDialog( "Rendering Model", false, false, true );
			SetProgressDialogIncrement( incValue );
			//if( UpdateProgressDialog( 0.0, "Creating Graphics Model" ) ) {
			//	return;
			//}
		}
		bool bDoExtrude = (filter.drawDetail == DETAIL_HI);
		//members
		for( int i = 1; i <= theModel.elements( MEMBER_ELEMENT ); i++ )
		{
			if( doProgress ){
				aborted = IncrementProgressDialog(0, "Members..." );
				if( aborted )
					break;
			}
			CMember* pM = (CMember*)theModel.element( MEMBER_ELEMENT, i );
			if( pM->mpGraphicsMember )
				continue;
			bool bMatch = false;
			//check all previously created members to see if we can use an existing extrusion.
			// TeK Performance test 7/13/2009, don't check for matches, just create them all!
			// Performance is faster with the GRAEF huge model, when initially setting up a model
			// without looping over all the members to find similar ones.  I think the isGeometrySimilar()
			// method may be slow, but maybe it is only faster if DETAIL_HI is not ON??
//#pragma message( JL "TeK Note: Uncomment this code again, if you want to. Seems faster without it?" )
			//for( int j = 1; j < i; j++ )
			//{
			//	CMember* pExisting = (CMember*)(theModel.element( MEMBER_ELEMENT, j ) );
			//	CGraphicsMember* pGM = pExisting->mpGraphicsMember;
			//	ASSERT( pGM ); //we should definitely have a graphics member for these
			//	if( pGM && pExisting->isGeometrySimilar( *pM ) )
			//	{
			//		CGraphicsMember* pNewGM = new CGraphicsMember( pM, pGM );
			//		ASSERT( pNewGM );
			//		if( pNewGM ){
			//			pM->mpGraphicsMember = pNewGM;
			//			bMatch = true;
			//			break;
			//		}
			//	}
			//}
			if( !bMatch ){
				CGraphicsMember* pGM = new CGraphicsMember( pM, NULL, 0., bDoExtrude );
				ASSERT( pGM );
				if( pGM ){
					pM->mpGraphicsMember = pGM;
				}
			}	
		}
		//cables
		if( !aborted )
		{
			for( int i = 1; i <= theModel.elements( CABLE_ELEMENT ); i++ )
			{
				if( doProgress ){
					aborted = IncrementProgressDialog(0, "Cables..." );
					if( aborted )
						break;
				}
				CCable* pC = (CCable*)theModel.element( CABLE_ELEMENT, i );
				if( pC->mpGraphicsCable )
					continue;
				bool bMatch = false;
				// TeK Commented 7/13/2009, code in GraphicsModel.cpp indicates we shouldn't do this for cables anyway...
				////check all previously created members to see if we can use an existing extrusion.
				//for( int j = 1; j < i; j++ )
				//{
				//	CCable* pExisting = (CCable*)(theModel.element( CABLE_ELEMENT, j ) );
				//	CGraphicsCable* pGC = pExisting->mpGraphicsCable;
				//	ASSERT( pGC ); //we should definitely have a graphics member for these
				//	if( pGC && pExisting->isGeometrySimilar( *pC ) )
				//	{
				//		CGraphicsCable* pNewGC = new CGraphicsCable( pC, pGC );
				//		ASSERT( pNewGC );
				//		if( pNewGC ){
				//			pC->mpGraphicsCable = pNewGC;
				//			bMatch = true;
				//			break;
				//		}
				//	}
				//}
				if( !bMatch ){
					CGraphicsCable* pGC = new CGraphicsCable( pC, NULL, 0., bDoExtrude );
					ASSERT( pGC );
					if( pGC )
						pC->mpGraphicsCable = pGC;
				}	
			}
		}
		if( doProgress )
			KillProgressDialog( );
	}
	m_bExtrusionsCreated = true;
}

void CGraphicsEngine::ClearDisplacedExtrusions( const CResultCase* pResultCase )
{
	for( int i = 1; i <= theModel.elements( MEMBER_ELEMENT ); i++ )
	{
		CMember* pM = (CMember*)(theModel.element( MEMBER_ELEMENT, i ));
		if( pM->m_ResultGraphicsMembers[ pResultCase ] )
		{
			delete pM->m_ResultGraphicsMembers[ pResultCase ];
			pM->m_ResultGraphicsMembers.erase( pResultCase );
		}
	}
	for( int i = 1; i <= theModel.elements( CABLE_ELEMENT ); i++ )
	{
		CCable* pC = (CCable*)(theModel.element( CABLE_ELEMENT, i ));
		if( pC->m_ResultGraphicsCables[ pResultCase ] )
		{
			delete pC->m_ResultGraphicsCables[ pResultCase ];
			pC->m_ResultGraphicsCables.erase( pResultCase );
		}
	}
}

void CGraphicsEngine::CreateDisplacedExtrusions( const CResultCase* pResultCase, 
												CWindowFilter& filter, 
												double distFactor, 
												double memberMin, 
												double memberMax, 
												double cableMin, 
												double cableMax  )
{
	bool aborted = false;
	//double distFactor = GetDisplacementFactor();
	int nMembers = theModel.elements( MEMBER_ELEMENT );
	int nCables = theModel.elements( CABLE_ELEMENT );
	if( nMembers + nCables > 0 ){
		//count the number of members that need extrusions. if it's more than 10
		//then put up a progress dialog otherwise it should be ok to 
		//omit it (and thus avoid having the progress bar flash on the screen )
		int membersWithoutGraphics = 0;
		int cablesWithoutGraphics = 0;
		for( int i = 1; i <= theModel.elements( MEMBER_ELEMENT ); i++ )
		{
			CMember* pM = (CMember*)(theModel.element( MEMBER_ELEMENT, i ));
			if( !pM->m_ResultGraphicsMembers[ pResultCase ] )
				membersWithoutGraphics++;
		}
		for( int i = 1; i <= theModel.elements( CABLE_ELEMENT ); i++ )
		{
			CCable* pC = (CCable*)(theModel.element( CABLE_ELEMENT, i ));
			if( !pC->mpGraphicsCable )
				cablesWithoutGraphics++;
		}
		bool doProgress = false;
		if( membersWithoutGraphics + cablesWithoutGraphics > 10 )
			doProgress = true;
		if( doProgress )
		{
			double incValue = 1./(double)(nMembers + nCables);
			StartProgressDialog( "Creating Displaced Graphics Model" );
			SetProgressDialogIncrement( incValue );
			//if( UpdateProgressDialog( 0.0, "Creating Graphics Model" ) ) {
			//	return;
			//}
		}
		//members
		bool bDoExtrude = (filter.drawDetail == DETAIL_HI);
		//JL Note 4/14/2009 - OpenMP support for threading...see:
		// http://www.codeproject.com/KB/cpp/BeginOpenMP.aspx
		// http://msdn.microsoft.com/en-us/library/tt15eb9t(VS.80).aspx
		#pragma omp parallel for
		for( int i = 1; i <= theModel.elements( MEMBER_ELEMENT ); i++ )
		{
			if( doProgress ){
				aborted = IncrementProgressDialog(0, "Members..." );
				if( aborted )
					break;
			}
			CMember* pM = (CMember*)theModel.element( MEMBER_ELEMENT, i );
			if( pM->m_ResultGraphicsMembers[ pResultCase ] )
				continue;
	
			CGraphicsMember* pGM = new CGraphicsMember( pM, pResultCase, distFactor, bDoExtrude );
			ASSERT( pGM );
			if( pGM )
			{
				const CResult* pR = pResultCase->result( *pM );
				pGM->SetupTextureCoords( memberMin, memberMax, filter.member.GetGraphType(), pR, filter.member.diagramResults ); 
				pM->m_ResultGraphicsMembers[ pResultCase ] = pGM;
			}
		}
		//cables
		if( !aborted )
		{
			#pragma omp parallel for
			for( int i = 1; i <= theModel.elements( CABLE_ELEMENT ); i++ )
			{
				if( doProgress ){
					aborted = IncrementProgressDialog(0, "Cables...");
					if( aborted )
						break;
				}
				CCable* pC = (CCable*)theModel.element( CABLE_ELEMENT, i );
				if( pC->m_ResultGraphicsCables[ pResultCase ] )
					continue;

				CGraphicsCable* pGC = new CGraphicsCable( pC, pResultCase, distFactor, bDoExtrude );
				ASSERT( pGC );
				if( pGC )
				{
					const CResult* pR = pResultCase->result( *pC );
					pGC->SetupTextureCoords( cableMin, cableMax, filter.cable.GetGraphType(), pR, filter.cable.diagramResults ); 
					pC->m_ResultGraphicsCables[ pResultCase ] = pGC;
				}	
			}
		}
		if( doProgress )
			KillProgressDialog( );
	}
	UpdateDataTime();
	//bExtrusionsCreated = true;
}
////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// The ThreadFunction!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
DWORD WINAPI graphicsThreadedTaskManager( LPVOID pvThreadParm )
{
	// the background thread function
	CGraphicsEngine* pEngine = dynamic_cast<CGraphicsEngine*>((CGraphicsEngine*)pvThreadParm);
	if( pEngine ) {
		while( true )
		{   // our infinite loop
			// We now politely sleep( 1000 ) if we have nothing to do.  
			// 
			if( pEngine->engineIsActive() ) 
				Sleep( 0 );   // Relinquish the remainder of this time slice
			else 
				Sleep( 1000 ); // sleep for a "long" time if there is nothing to analyze?
			//pEngine->doThreadedTasks();
		}
	}
	return 0;
}
