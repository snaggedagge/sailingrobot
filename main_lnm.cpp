
#include <string>
#include "SystemServices/Logger.h"
#include "MessageBus/MessageBus.h"

#include "WorldState/CollidableMgr/CollidableMgr.h"

#if SIMULATION == 1
 #include "Nodes/SimulationNode.h"
#else
 #include "Hardwares/CV7Node.h"
 #include "Hardwares/HMC6343Node.h"
 #include "Hardwares/GPSDNode.h"
 #include "Hardwares/ActuatorNode.h"
 #include "Hardwares/ArduinoNode.h"
#endif

#include "Navigation/WaypointMgrNode.h"
#include "WorldState/VesselStateNode.h"
#include "Nodes/HTTPSyncNode.h"
#include "Nodes/XbeeSyncNode.h"
#include "Navigation/LineFollowNode.h"

#include "Messages/DataRequestMsg.h"
#include "DataBase/DBHandler.h"
#include "Hardwares/MaestroController/MaestroController.h"
#include "xBee/Xbee.h"

#include "Navigation/LocalNavigationModule/LocalNavigationModule.h"
#include "Navigation/LocalNavigationModule/Voters/WaypointVoter.h"
#include "Navigation/LocalNavigationModule/Voters/WindVoter.h"
#include "Navigation/LocalNavigationModule/Voters/ChannelVoter.h"
#include "Navigation/LocalNavigationModule/Voters/ProximityVoter.h"
#include "Navigation/LocalNavigationModule/Voters/MidRangeVoter.h"
#include "LowLevelControllers/LowLevelController.h"

#define DISABLE_LOGGING 0

enum class NodeImportance {
	CRITICAL,
	NOT_CRITICAL
};

///----------------------------------------------------------------------------------
/// Initialises a node and shutsdown the program if a critical node fails.
///
/// @param node 			A pointer to the node to initialise
/// @param nodeName 		A string name of the node, for logging purposes.
/// @param importance 		Whether the node is a critcal node or not critical. If a
///							critical node fails to initialise the program will
///							shutdown.
///
///----------------------------------------------------------------------------------
void initialiseNode(Node& node, const char* nodeName, NodeImportance importance)
{
	if(node.init())
	{
		Logger::info("Node: %s - init\t[OK]", nodeName);
	}
	else
	{
		Logger::error("Node: %s - init\t\t[FAILED]", nodeName);

		if(importance == NodeImportance::CRITICAL)
		{
			Logger::error("Critical node failed to initialise, shutting down");
			Logger::shutdown();
			exit(1);
		}
	}
}

///----------------------------------------------------------------------------------
/// Used for development of the Local Navigation Module
///
///----------------------------------------------------------------------------------
void development_LocalNavigationModule( MessageBus& messageBus, DBHandler& dbHandler)
{
	const double PGAIN = 0.20;
	const double IGAIN = 0.30;
	const int16_t MAX_VOTES = 25;

	Logger::info( "Using Local Navigation Module" );

	VesselStateNode vesselState	( messageBus, 0.2 );
	WaypointMgrNode waypoint	( messageBus, dbHandler );
	LocalNavigationModule lnm	( messageBus );
	LowLevelController llc		( messageBus, dbHandler, PGAIN, IGAIN );
	CollidableMgr collidableMgr;

	#if SIMULATION == 1
	SimulationNode 	simulation	( messageBus, &collidableMgr );
	#endif

	initialiseNode( vesselState, 	"Vessel State Node", 		NodeImportance::CRITICAL );
	initialiseNode( waypoint, 		"Waypoint Node", 			NodeImportance::CRITICAL );
	initialiseNode( lnm,			"Local Navigation Module",	NodeImportance::CRITICAL );
	initialiseNode( llc,			"Low Level Controller",		NodeImportance::CRITICAL );

	#if SIMULATION == 1
	initialiseNode( simulation, 	"Simulation Node", 			NodeImportance::CRITICAL );
	#endif

	WaypointVoter waypointVoter( MAX_VOTES, 1 );
	WindVoter windVoter( MAX_VOTES, 1 );
	ChannelVoter channelVoter( MAX_VOTES, 1 );
	MidRangeVoter midRangeVoter( MAX_VOTES, 1, collidableMgr );
	ProximityVoter proximityVoter( MAX_VOTES, 2, collidableMgr);

	lnm.registerVoter( &waypointVoter );
	lnm.registerVoter( &windVoter );
	lnm.registerVoter( &channelVoter );
	lnm.registerVoter( &proximityVoter );
	lnm.registerVoter( &midRangeVoter );


	vesselState.start();
	lnm.start();
	
	collidableMgr.startGC();

	#if SIMULATION == 1
	simulation.start();
	#endif

	Logger::info("Message bus started!");

	// Returns when the program has been closed (Does the program ever close gracefully?)
	messageBus.run();
}

///----------------------------------------------------------------------------------
/// Entry point, can accept one argument containing a relative path to the database.
///
///----------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	// This is for eclipse development so the output is constantly pumped out.
	setbuf(stdout, NULL);

	// Database Path
	std::string db_path;
	if (argc < 2) {
		db_path = "./asr.db";
	} else {
		db_path = std::string(argv[1]);
	}

	printf("================================================================================\n");
	printf("\t\t\t\tSailing Robot\n");
	printf("\n");
	printf("================================================================================\n");

	if (Logger::init()) {
		Logger::info("Built on %s at %s", __DATE__, __TIME__);
		Logger::info("Logger init\t\t[OK]");
	}
	else {
		Logger::error("Logger init\t\t[FAILED]");
	}

	MessageBus messageBus;
	DBHandler dbHandler(db_path);

	if(dbHandler.initialise())
	{
		Logger::info("Database init\t\t[OK]");
	}
	else
	{
		Logger::error("Database init\t\t[FAILED]");
		Logger::shutdown();
		exit(1);
	}
	development_LocalNavigationModule( messageBus, dbHandler );

	Logger::shutdown();
	exit(0);
}
