// GraphicsReport.cpp
// TeK Split from GraphicView.cpp 8/18/2007
// Functions in CGraphicView that set up 'Quick' Reports
#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"
#include "Main.h"
#include "ReportStyleMgr.h"
#include "Table.h"
#include "Summary.h"
#include "project.h"
#include "model.h"
#include "Servcase.h"
#include "MemLoad.h"
#include "PlnrLoad.h"
#include "Design/GroupManager.h"
#include "Design/mesh.h"
#include "DataUtil.h"


const char* CGraphicView::QuickReportName( void ) {
	SelectionCountData sel;
	CServiceCase* pSC = NULL;
	if( theProject.isValid( serviceCase() ) ) {
		pSC = dynamic_cast<CServiceCase*>(m_Filter.loadCase());
	}
	getCurrentSelectionCount( sel, pSC, false );
	int selModel = sel.Members + sel.Planars + sel.Springs + sel.Nodes + sel.Cables + 
		sel.RigidDiaphragms + sel.Areas + sel.Foundations;
	int selLoads = sel.NodalLoads + sel.MemberLoads + sel.PlanarLoads + sel.MovingLoads + sel.AreaLoads;
	int selTotal = selModel + selLoads;

	if( selTotal > 0 ) {
		if( sel.Nodes == selTotal  )
			return "&Report Selected Node";
		if( (DESIGN_WINDOW == m_Filter.windowType) && (sel.DesignGroups == selTotal) )
			return "&Report Design Checks";
		else if( sel.Members == selTotal )
			return "&Report Selected Member";
		if( (DESIGN_WINDOW == m_Filter.windowType) && (sel.DesignMeshes == selTotal) )
			return "&Report Design Checks";
		else if( sel.Planars == selTotal )
			return "&Report Selected Plate";
		else if( sel.Springs == selTotal )
			return "&Report Selected Spring";
		else if( sel.Cables == selTotal )
			return "&Report Selected Cable";
		else if( sel.Areas == selTotal )
			return "&Report Selected Area";
		else if( sel.Foundations == selTotal && m_Filter.windowType == MODEL_WINDOW )
			return "&Report Selected Foundations";
		else if( sel.RigidDiaphragms == selTotal )
			return "&Report Selected Rigid Diaphragm";
		else if( sel.NodalLoads == selTotal )
			return "&Report Selected Nodal Load";
		else if( sel.MemberLoads == selTotal )
			return "&Report Selected Member Load";
		else if( sel.MovingLoads == selTotal )
			return "&Report Selected Moving Load";
		else if( sel.PlanarLoads == selTotal )
			return "&Report Selected Plate Load";
		else if( sel.AreaLoads == selTotal )
			return "&Report Selected Area Load";
		else
			return "&Report Selected Items";
	}
	else {
		return "&Report Complete Model";
	}
}


void CGraphicView::QuickModelViewReport( CReport& report )
{
	SelectionCountData sel;
	CServiceCase* pSC = NULL;
	if( theProject.isValid( serviceCase() ) ) {
		pSC = dynamic_cast<CServiceCase*>(m_Filter.loadCase());
	}
	getCurrentSelectionCount( sel, pSC, false );
	int selModel = sel.Members + sel.Planars + sel.Springs + sel.Nodes + sel.Cables + 
		sel.RigidDiaphragms + sel.Areas + sel.Foundations;
	int selLoads = sel.NodalLoads + sel.MemberLoads + sel.PlanarLoads + sel.MovingLoads + sel.AreaLoads;
	int selTotal = selModel + selLoads;

	// Get non-selected, and specialty selection counts...
	int Nodes = theModel.nodes( );
	int Members = theModel.elements( MEMBER_ELEMENT );
	int Cables = theModel.elements( CABLE_ELEMENT );
	int Plates = theModel.elements( PLANAR_ELEMENT );
	int Springs = theModel.elements( SPRING_ELEMENT );
	int Areas = theModel.areas();
	int Foundations = theModel.foundations();
	int rigidD = theModel.rigidDiaphragms();
	int NodalLoads = 0;
	int ForceNodalLoads = 0;
	int SettlementNodalLoads = 0;
	int MemberLoads = 0;
	int MovingLoads = 0;
	int PlateLoads = 0;
	int AreaLoads = 0;
	int RDLoads = 0;
	int Loads = 0;
	int selNodalForceLoads = 0;
	int selNodalSettlementLoads = 0;
	if( pSC ) {
		NodalLoads = pSC->nodalLoads();
		ForceNodalLoads = pSC->nodalLoads( NODAL_FORCE );
		SettlementNodalLoads = pSC->nodalLoads( NODAL_SETTLEMENT );
		MemberLoads = pSC->elementLoads( MEMBER_ELEMENT );
		MovingLoads = pSC->movingLoads();
		PlateLoads = pSC->elementLoads( PLANAR_ELEMENT );
		AreaLoads = pSC->areaLoads();
		RDLoads = pSC->rigidDiaphragmLoads();
		Loads = NodalLoads + MemberLoads + PlateLoads + MovingLoads + AreaLoads + RDLoads;
		selNodalForceLoads = pSC->nodalLoads( NODAL_FORCE, OnlySelected );
		selNodalSettlementLoads = pSC->nodalLoads( NODAL_SETTLEMENT, OnlySelected );
	}

	// Start the report with a Project Summary (always!)
	CReportSummary* pS = new CReportSummary( PROJECT_SUMMARY );
	report.addObject( *pS );
	// TeK Removed: We don't want this stuff automatically in every quick report!
	//// And then a model summary
	//pS = new CReportSummary( MODEL_SUMMARY );
	//report.addObject( *pS );
	//// Add an analysis summary
	//pS = new CReportSummary( ANALYSIS_SETTINGS_SUMMARY );
	//report.addObject( *pS );

	if( sel.Nodes ) report.nodeOption( SELECTED_OBJECTS );
	if( Nodes && (0 == sel.Total || sel.Nodes > 0) && m_Filter.nodes ) {
		CReportTable* pT = new CReportTable( NODE_TABLE );
		report.addObject( *pT );
	}
	if( sel.Members ) report.elementOption( MEMBER_ELEMENT, SELECTED_OBJECTS );
	if( Members && (0 == sel.Total || sel.Members > 0) && m_Filter.members ) {
		CReportTable* pT = new CReportTable( MEMBER_TABLE );
		report.addObject( *pT );
	}
	if( sel.Cables ) report.elementOption( CABLE_ELEMENT, SELECTED_OBJECTS );
	if( Cables && (0 == sel.Total || sel.Cables > 0) && m_Filter.cables ) {
		CReportTable* pT = new CReportTable( CABLE_TABLE );
		report.addObject( *pT );
	}
	if( sel.Planars ) report.elementOption( PLANAR_ELEMENT, SELECTED_OBJECTS );
	if( Plates && (0 == sel.Total || sel.Planars > 0) && m_Filter.plates ) {
		CReportTable* pT = new CReportTable( PLANAR_TABLE );
		report.addObject( *pT );
	}
	if( sel.Springs ) report.elementOption( SPRING_ELEMENT, SELECTED_OBJECTS );
	if( Springs && (0 == sel.Total || sel.Springs > 0) && m_Filter.springs ) {
		CReportTable* pT = new CReportTable( SPRING_TABLE );
		report.addObject( *pT );
	}
	// TeK Note 8/18/2007: We can't filter areas in reports yet?!
#pragma message( TEK "Need to add area filters to reporting system, then fix here..." )
	//if( sel.Areas ) report.areaOption( AREA_ITEMS???, SELECTED_OBJECTS );
	if( Areas && (0 == sel.Total || sel.Areas > 0) && m_Filter.areas ) {
		CReportTable* pT = new CReportTable( AREA_TABLE );
		report.addObject( *pT );
	}
	if( Foundations && (0 == sel.Total || sel.Foundations > 0) && m_Filter.foundations ) {
		CReportTable* pT = new CReportTable( FOUNDATION_TABLE );
		report.addObject( *pT );
	}

	if( rigidD && (0 == sel.Total || sel.RigidDiaphragms > 0) && m_Filter.rigidDiaphragms ) {
		CReportTable* pT = new CReportTable( RIGID_DIAPHRAGM_TABLE );
		report.addObject( *pT );
	}
	// Include Load Case information only if we will show some loads
	if( Loads && (0 == sel.Total || selLoads > 0 ) && m_Filter.loads ) {
		report.loadCaseOption( SELECTED_OBJECTS );
		if( pSC ) report.addCase( *pSC );
		CReportTable* pT = new CReportTable( SERVICECASE_TABLE );
		report.addObject( *pT );
	}
	if( sel.NodalLoads ) report.nodalLoadOption( SELECTED_OBJECTS );
	if( NodalLoads && (0 == sel.Total || sel.NodalLoads > 0) && (m_Filter.node.loads || m_Filter.node.settlements) ) {
		if( (0 == sel.NodalLoads && ForceNodalLoads > 0) || selNodalForceLoads > 0  ) {
			CReportTable* pT = new CReportTable( NODALLOAD_TABLE );
			report.addObject( *pT );
		}
		if( (0 == sel.NodalLoads && SettlementNodalLoads > 0) || selNodalSettlementLoads > 0 ) {
			CReportTable* pT = new CReportTable( NODALSETTLEMENT_TABLE );
			report.addObject( *pT );
		}
	}

	if( pSC && pSC->haveMovingLoads() )
	{
		CReportSummary* pS = new CReportSummary( MOVING_LOADCASE_SUMMARY );
		report.addObject( *pS );
	}

	bool bShowMemberLoads = m_Filter.loads && (m_Filter.member.concentratedLoads || m_Filter.member.uniformLoads ||
		m_Filter.member.linearLoads || m_Filter.member.temperatureLoads );

	if( pSC && MemberLoads && (0 == sel.Total || sel.MemberLoads > 0) && bShowMemberLoads ) {
		if( sel.MemberLoads ) report.elementLoadOption( MEMBER_ELEMENT, SELECTED_OBJECTS );
		// Determine how many member loads of each type!
		int point = 0;
		int uniform = 0;
		int linear = 0;
		int tChange = 0;
		int tGrad = 0;
		int selPoint = 0;
		int selUniform = 0;
		int selLinear = 0;
		int selTChange = 0;
		int selTGrad = 0;
		for( CElementLoadIterator eli( *pSC, MEMBER_ELEMENT ); !eli; eli++ ) {
			const CMemberLoad* pML = (const CMemberLoad*)eli();
			if( pML ) {
				switch( pML->type() ) {
				case MEMBER_CONCENTRATED:
					point++;
					if( pML->isSelected() ) selPoint++;
					break;
				case MEMBER_UNIFORM:
					uniform++;
					if( pML->isSelected() ) selUniform++;
					break;
				case MEMBER_LINEAR:
					linear++;
					if( pML->isSelected() ) selLinear++;
					break;
				case MEMBER_TEMPERATURE:
					tChange++;
					if( pML->isSelected() ) selTChange++;
					break;
				case MEMBER_TEMPERATURE_GRADIENT:
					tGrad++;
					if( pML->isSelected() ) selTGrad++;
					break;
				}
			}
		}	// end for each member load
		if( point && (0 == selTotal || selPoint > 0) && m_Filter.member.concentratedLoads ) {
			CReportTable* pT = new CReportTable( POINTLOAD_TABLE );
			report.addObject( *pT );
		}
		if( uniform && (0 == selTotal || selUniform > 0) && m_Filter.member.uniformLoads ) {
			CReportTable* pT = new CReportTable( UNIFORMLOAD_TABLE );
			report.addObject( *pT );
		}
		if( linear && (0 == selTotal || selLinear > 0) && m_Filter.member.linearLoads ) {
			CReportTable* pT = new CReportTable( LINEARLOAD_TABLE );
			report.addObject( *pT );
		}
		if( tChange && (0 == selTotal || selTChange > 0) && m_Filter.member.temperatureLoads ) {
			CReportTable* pT = new CReportTable( TEMPCHANGELOAD_TABLE );
			report.addObject( *pT );
		}
		if( tGrad && (0 == selTotal || selTGrad > 0) && m_Filter.member.temperatureLoads ) {
			CReportTable* pT = new CReportTable( TEMPGRADIENTLOAD_TABLE );
			report.addObject( *pT );
		}
	}
	if( pSC && PlateLoads && (0 == selTotal || sel.PlanarLoads > 0) && 
		( m_Filter.plate.pressureLoads || m_Filter.plate.thermalLoads ) ) {
		if( sel.PlanarLoads ) report.elementLoadOption( PLANAR_ELEMENT, SELECTED_OBJECTS );
		// Determine how many member loads of each type!
		int pressure = 0;
		int thermalIncrease = 0;
		int thermalGradient = 0;
		int selPressure = 0;
		int selThermalChange = 0;
		int selThermalGradient = 0;
		for( CElementLoadIterator eli( *pSC, PLANAR_ELEMENT ); !eli; eli++ ) {
			const CPlanarLoad* pML = (const CPlanarLoad*)eli();
			if( pML ) {
				switch( pML->type() ) {
				case PLANAR_UNIFORM_PRESSURE:
				case PLANAR_LINEAR_PRESSURE:
				case PLANAR_HYDROSTATIC_PRESSURE:
				case PLANAR_LINEARVARYING_PRESSURE:
					pressure++;
					if( pML->isSelected() ) selPressure++;
					break;
				case PLANAR_TEMPERATURE_INCREASE:
					thermalIncrease++;
					if( pML->isSelected() ) selThermalChange++;
					break;
				case PLANAR_TEMPERATURE_GRADIENT:
					thermalGradient++;
					if( pML->isSelected() ) selThermalGradient++;
					break;
				}
			}
		}	// end for each load
		if( pressure && (0 == selTotal || selPressure > 0)) {
			CReportTable* pT = new CReportTable( PLATELOAD_PRESSURE_TABLE );
			report.addObject( *pT );
		}
		if( thermalIncrease && (0 == selTotal || selThermalChange > 0)) {
			CReportTable* pT = new CReportTable( PLANARTHERMALCHANGE_TABLE );
			report.addObject( *pT );
		}
		if( thermalGradient && (0 == selTotal || selThermalGradient > 0)) {
			CReportTable* pT = new CReportTable( PLANARTHERMALGRADIENT_TABLE );
			report.addObject( *pT );
		}
	}
	if( pSC && AreaLoads && (0 == selTotal || sel.AreaLoads > 0) && 
		( (m_Filter.areas || m_Filter.loadsOnFiltered) && m_Filter.areaLoads ) ) {
		CReportTable* pT = new CReportTable( AREA_LOAD_TABLE );
		report.addObject( *pT );
	}
	if( pSC && RDLoads && (0 == selTotal || sel.RigidDiaphragmLoads > 0) && 
		( (m_Filter.rigidDiaphragms || m_Filter.loadsOnFiltered) && m_Filter.rigidDiaphragmLoads ) ) {
		CReportTable* pT = new CReportTable( RIGID_DIAPHRAGMLOAD_TABLE );
		report.addObject( *pT );
	}
	return;
}	// QuickReport()


void CGraphicView::QuickResultViewReport( CReport& report )
{
	int Nodes = theModel.nodes( );
	int Members = theModel.elements( MEMBER_ELEMENT );
	int Cables = theModel.elements( CABLE_ELEMENT );
	int Plates = theModel.elements( PLANAR_ELEMENT );
	int Springs = theModel.elements( SPRING_ELEMENT );
	int selNodes = theModel.nodes( OnlySelected );
	int selMembers = theModel.elements( MEMBER_ELEMENT, OnlySelected );
	int selCables = theModel.elements( CABLE_ELEMENT, OnlySelected );
	int selPlates = theModel.elements( PLANAR_ELEMENT, OnlySelected );
	int selSprings = theModel.elements( SPRING_ELEMENT, OnlySelected );
	int selTotal = selMembers + selPlates + selSprings + selNodes + selCables;

	// Start the report with a Project Summary (always!)
	CReportSummary* pS = new CReportSummary( PROJECT_SUMMARY );
	report.addObject( *pS );

	// Create a new result report for the selected objects.
	// (As the selection changes so does the report!)
	if( POST_WINDOW == m_Filter.windowType ) {
		theApp.recordCommand( RESULT_REPORT_CMD );
		report.nodeOption( (selNodes > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( MEMBER_ELEMENT, (selMembers > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( CABLE_ELEMENT, (selCables > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( PLANAR_ELEMENT, (selPlates > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( SPRING_ELEMENT, (selSprings > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.loadCaseOption( SELECTED_OBJECTS );
		CLoadCase* pLC = m_Filter.loadCase();
		const CResultCase* pRC = m_Filter.resultCase();
		CHECK_IF( pLC && pRC ) {
			report.addCase( *pLC );
			report.resultCaseOption( SELECTED_RC_OPTION );
			report.flushResultCases();
			report.addResultCase( *pRC );
		}

		ETLoadCase activeLCType = SERVICE_CASE;
		if( m_Filter.loadCase() ) {
			activeLCType = m_Filter.loadCase()->type();
		}

		CReportTable* pT = NULL;

		// General Dynamic results
		if( MODE_SHAPE_CASE == activeLCType ) {
			pT = new CReportTable( MODESHAPECASE_TABLE );
			report.addObject( *pT );
		}
		if( RESPONSE_CASE == activeLCType ) {
			pS = new CReportSummary( DYNAMIC_RESPONSE_ANALYSIS_SUMMARY );
			report.addObject( *pS );
			pT = new CReportTable( RESPONSECASE_TABLE );
			report.addObject( *pT );
		}

		// Nodal Results
		if( Nodes && (0 == selTotal || selNodes > 0) && m_Filter.nodes ) {
			bool dispTable = true;
			bool reacTable = true;
			if( selNodes == 1) {
				const CNode* pN = theModel.node( 1, OnlySelected );
				if( pN->isFixed() ) {
					dispTable = false;
				}
				if( pN->isFree() ) {
					reacTable = false;
				}
			}
			if( dispTable ) {
				pT = new CReportTable( NODALDISPLACEMENT_TABLE );
				report.addObject( *pT );
			}
			if( reacTable ) {
				pT = new CReportTable( NODALREACTION_TABLE );
				report.addObject( *pT );
			}
		}

		// Member Results
		if( Members && (0 == selTotal || selMembers > 0) && m_Filter.members ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( MEMBER_ELEMENT ) > 0 ) {
				pT = new CReportTable( MEMBERFORCE_TABLE );
				report.addObject( *pT );
			}
		}
		// Cable Results
		if( Cables && (0 == selTotal || selCables > 0) && m_Filter.cables ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( CABLE_ELEMENT ) > 0 ) {
				pT = new CReportTable( CABLE_RESULT_TABLE );
				report.addObject( *pT );
			}
		}
		// Planar Results
		if( Plates && (0 == selTotal || selPlates > 0) && m_Filter.plates ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( PLANAR_ELEMENT ) > 0 ) {
				pT = new CReportTable( PLANARPRINCIPAL_TABLE );
				report.addObject( *pT );
				pT = new CReportTable( PLANARLOCALSTRESS_TABLE );
				report.addObject( *pT );
				if( theStructure.canPlanarsBend() ) {
					pT = new CReportTable( PLANARLOCALFORCE_TABLE );
					report.addObject( *pT );
				}
			}
		}
		// Spring Results
		if( Springs && (0 == selTotal || selSprings > 0) && m_Filter.springs ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( SPRING_ELEMENT ) > 0 ) {
				pT = new CReportTable( SPRINGRESULT_TABLE );
				report.addObject( *pT );
			}
		}
		//// We had to dynamically create a new DCR report
		//if( updateDCR ) {  
		//	// save the current report as one of the predefined styles
		//	theStyleManager.updateDoubleClickStyle( newStyle, report );
		//}
	}	// end if POST_WINDOW
	else if( DESIGN_WINDOW == m_Filter.windowType ) {
		theApp.recordCommand( DESIGN_REPORT_CMD );
		int selGroups = 0;
		for( int g = 1; g <= theGroupManager.groups(); ++g ) {
			CDesignGroup* pDG = theGroupManager.group(g);
			if( pDG ) {
				if( pDG->disabled() ) continue; // TeK Add 1/8/2009
				pDG->unSelect();	// Unselect all groups first!
				if( pDG->isValidAndSelected() ) {
					pDG->select();
					selGroups++;
				}
			}
		}
		for( int g = 1; g <= theMeshManager.meshes(); ++g ) {
			CDesignMesh* pDM = theMeshManager.mesh(g);
			if( pDM ) {
				pDM->unSelect();	// Unselect all groups first!
				if( pDM->isValidAndSelected() ) {
					pDM->select();
					selGroups++;
				}
			}
		}
		report.designGroupOption( (selGroups > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );

		// Report only the DESIGN load cases
		report.loadCaseOption( SELECTED_OBJECTS );
		report.flushCases();
		for( int lct = 0; lct < LoadCaseTypes; ++lct ) {
			ETLoadCase type = ETLoadCase(lct);
			for( int lc = 1; lc <= theProject.loadCases( type ); ++lc ) {
				CLoadCase* pLC = theProject.loadCase( type, lc );
				if( pLC && pLC->isDesignCase() ) {
					report.addCase( *pLC );
				}
			}
		}

		CReportObject* pO;

		// Design Results
		if( theGroupManager.groups() > 0 ) {
			report.resultCaseOption( ALL_RC_OPTION );
			pO = new CReportTable( RESULTCASE_TABLE ); // Show result case id numbers for design cases!
			report.addObject( *pO );

			if( (1 == selMembers) || (1 == selGroups) ) {
				pO = new CReportTable( MEMBER_UNITY_TABLE );
				report.addObject( *pO );
			}

			if( (selMembers == 1) || ((selMembers >= 1) && !m_Filter.toggleDesignGroups) ) {
				report.elementOption( MEMBER_ELEMENT, SELECTED_OBJECTS );
				pO = new CReportDesign( MEMBER_DESIGN );
			} 
			else {
				pO = new CReportDesign( GROUP_DESIGN );
			}
			report.addObject( *pO );
			//}
		}
		if( theMeshManager.meshes() > 0 ) {
			if( (selPlates == 1) || ((selPlates >= 1) && !m_Filter.toggleDesignGroups) ) {
				report.elementOption( PLANAR_ELEMENT, SELECTED_OBJECTS );
				pO = new CReportDesign( PLATE_DESIGN );
			} 
			else {
				pO = new CReportDesign( MESH_DESIGN );
			}
			report.addObject( *pO );
		}

	}	// end if DESIGN_WINDOW
	return;
}

void CGraphicView::QuickReport( CReport& report ) {
	if( m_Filter.windowType == POST_WINDOW || m_Filter.windowType == DESIGN_WINDOW )
		QuickResultViewReport( report );
	else if( m_Filter.windowType == MODEL_WINDOW )
		QuickModelViewReport( report );
	else if( m_Filter.windowType == DESIGN_WINDOW )
		ASSERT( false );
	return;
}	// QuickReport()


void CGraphicView::QuickResultReport( CReport& report )
{
	//TRACE( "QuickReport() called" );

	int Nodes = theModel.nodes( );
	int Members = theModel.elements( MEMBER_ELEMENT );
	int Cables = theModel.elements( CABLE_ELEMENT );
	int Plates = theModel.elements( PLANAR_ELEMENT );
	int Springs = theModel.elements( SPRING_ELEMENT );
	int selNodes = theModel.nodes( OnlySelected );
	int selMembers = theModel.elements( MEMBER_ELEMENT, OnlySelected );
	int selCables = theModel.elements( CABLE_ELEMENT, OnlySelected );
	int selPlates = theModel.elements( PLANAR_ELEMENT, OnlySelected );
	int selSprings = theModel.elements( SPRING_ELEMENT, OnlySelected );
	int selTotal = selMembers + selPlates + selSprings + selNodes + selCables;

	// Start the report with a Project Summary (always!)
	CReportSummary* pS = new CReportSummary( PROJECT_SUMMARY );
	report.addObject( *pS );

	// Create a new result report for the selected objects.
	// (As the selection changes so does the report!)
	if( POST_WINDOW == m_Filter.windowType ) {
		theApp.recordCommand( RESULT_REPORT_CMD );
		report.nodeOption( (selNodes > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( MEMBER_ELEMENT, (selMembers > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( CABLE_ELEMENT, (selCables > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( PLANAR_ELEMENT, (selPlates > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( SPRING_ELEMENT, (selSprings > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.loadCaseOption( SELECTED_OBJECTS );
		CLoadCase* pLC = m_Filter.loadCase();
		const CResultCase* pRC = m_Filter.resultCase();
		CHECK_IF( pLC && pRC ) {
			report.addCase( *pLC );
			report.resultCaseOption( SELECTED_RC_OPTION );
			report.flushResultCases();
			report.addResultCase( *pRC );
		}

		ETLoadCase activeLCType = SERVICE_CASE;
		if( m_Filter.loadCase() ) {
			activeLCType = m_Filter.loadCase()->type();
		}

		CReportTable* pT = NULL;

		// General Dynamic results
		if( MODE_SHAPE_CASE == activeLCType ) {
			pT = new CReportTable( MODESHAPECASE_TABLE );
			report.addObject( *pT );
		}
		if( RESPONSE_CASE == activeLCType ) {
			pS = new CReportSummary( DYNAMIC_RESPONSE_ANALYSIS_SUMMARY );
			report.addObject( *pS );
			pT = new CReportTable( RESPONSECASE_TABLE );
			report.addObject( *pT );
		}

		// Nodal Results
		if( Nodes && (0 == selTotal || selNodes > 0) && m_Filter.nodes ) {
			bool dispTable = true;
			bool reacTable = true;
			if( selNodes == 1 ) {
				const CNode* pN = theModel.node( 1, OnlySelected );
				if( pN->isFixed() ) {
					dispTable = false;
				}
				if( pN->isFree() ) {
					reacTable = false;
				}
			}
			if( dispTable ) {
				pT = new CReportTable( NODALDISPLACEMENT_TABLE );
				report.addObject( *pT );
			}
			if( reacTable ) {
				pT = new CReportTable( NODALREACTION_TABLE );
				report.addObject( *pT );
			}
		}

		// Member Results
		if( Members && (0 == selTotal || selMembers > 0) && m_Filter.members ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( MEMBER_ELEMENT ) > 0 ) {
				pT = new CReportTable( MEMBERFORCE_TABLE );
				report.addObject( *pT );
			}
		}
		// Cable Results
		if( Cables && (0 == selTotal || selCables > 0) && m_Filter.cables ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( CABLE_ELEMENT ) > 0 ) {
				pT = new CReportTable( CABLE_RESULT_TABLE );
				report.addObject( *pT );
			}
		}
		// Planar Results
		if( Plates && (0 == selTotal || selPlates > 0) && m_Filter.plates ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( PLANAR_ELEMENT ) > 0 ) {
				pT = new CReportTable( PLANARPRINCIPAL_TABLE );
				report.addObject( *pT );
				pT = new CReportTable( PLANARLOCALSTRESS_TABLE );
				report.addObject( *pT );
				if( theStructure.canPlanarsBend() ) {
					pT = new CReportTable( PLANARLOCALFORCE_TABLE );
					report.addObject( *pT );
				}
			}
		}
		// Spring Results
		if( Springs && (0 == selTotal || selSprings > 0) && m_Filter.springs ) {
			if( MODE_SHAPE_CASE != activeLCType && theModel.elements( SPRING_ELEMENT ) > 0 ) {
				pT = new CReportTable( SPRINGRESULT_TABLE );
				report.addObject( *pT );
			}
		}
		//// We had to dynamically create a new DCR report
		//if( updateDCR ) {  
		//	// save the current report as one of the predefined styles
		//	theStyleManager.updateDoubleClickStyle( newStyle, report );
		//}
	}	// end if POST_WINDOW
	else if( DESIGN_WINDOW == m_Filter.windowType ) {
		theApp.recordCommand( DESIGN_REPORT_CMD );
		int selGroups = 0;
		for( int g = 1; g <= theGroupManager.groups(); ++g ) {
			CDesignGroup* pDG = theGroupManager.group(g);
			if( pDG ) {
				if( pDG->disabled() ) continue; // TeK Add 1/8/2009
				pDG->unSelect();	// Unselect all groups first!
				if( pDG->isValidAndSelected() ) {
					pDG->select();
					selGroups++;
				}
			}
		}
		for( int g = 1; g <= theMeshManager.meshes(); ++g ) {
			CDesignMesh* pDM = theMeshManager.mesh(g);
			if( pDM ) {
				pDM->unSelect();	// Unselect all groups first!
				if( pDM->isValidAndSelected() ) {
					pDM->select();
					selGroups++;
				}
			}
		}
		report.designGroupOption( (selGroups > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( MEMBER_ELEMENT, (selMembers > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );
		report.elementOption( PLANAR_ELEMENT, (selPlates > 0 ? SELECTED_OBJECTS : ALL_OBJECTS) );

		report.loadCaseOption( SELECTED_OBJECTS ); // we select them below!
		report.flushCases();
		for( int lct = 0; lct < LoadCaseTypes; ++lct ) {
			ETLoadCase type = ETLoadCase(lct);
			for( int lc = 1; lc <= theProject.loadCases( type ); ++lc ) {
				CLoadCase* pLC = theProject.loadCase( type, lc );
				if( pLC && pLC->isDesignCase() ) {
					report.addCase( *pLC );
				}
			}
		}

		CReportObject* pO = NULL;
		// Design Results
		if( theGroupManager.groups() > 0 ) {
			report.resultCaseOption( ALL_RC_OPTION );
			pO = new CReportTable( RESULTCASE_TABLE ); // Show result case id numbers for design cases!
			report.addObject( *pO );

			if( (selGroups > 1) && (selMembers > 1) ) {
				pO = new CReportTable( MEMBER_UNITY_TABLE );
				report.addObject( *pO );
			}

			if( (selMembers == 1) || ((selMembers >= 1) && !m_Filter.toggleDesignGroups) ) {
				report.elementOption( MEMBER_ELEMENT, SELECTED_OBJECTS );
				pO = new CReportDesign( MEMBER_DESIGN );
			} 
			else {
				pO = new CReportDesign( GROUP_DESIGN );
			}
			report.addObject( *pO );
		} 
		if( theMeshManager.meshes() > 0 ) {
			if( (selPlates == 1) || ((selPlates >= 1) && !m_Filter.toggleDesignGroups) ) {
				report.elementOption( PLANAR_ELEMENT, SELECTED_OBJECTS );
				pO = new CReportDesign( PLATE_DESIGN );
			} 
			else {
				pO = new CReportDesign( MESH_DESIGN );
			}
			report.addObject( *pO );
		}

	}	// end if DESIGN_WINDOW
	return;
}

