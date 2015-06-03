#include "main.h"

#include "SailingRobot.h"
#include "thread/ExternalCommand.h"
#include "thread/SystemState.h"
#include "xBeeSync.h"
#include "GPSupdater.h"
#include <thread>
#include <unistd.h>
#include <signal.h>
#include <sstream>
#include <iostream>     
#include <iomanip>      
#include <ctime>    

static void threadXBeeSyncRun() {
	xbee_handle->run();
}

static void threadGPSupdate() {
	try {
		gps_handle->run();
	}
	catch (const char * e) {
		std::cout << "ERROR while running static void threadGPSupdate()" << e << std::endl;
	}
}

void term(int signum)
{
	printf("\n-SIGINT detected, shutting down...\n");
	printf(" stopping main loop\n");
	sr_handle->shutdown();
	if (xbee_handle) {
		printf(" stopping xBee thread\n");
		xbee_handle->close();
	}
	printf(" stopping GPS thread\n");
	gps_handle->close();
	printf("-DONE\n");
}

int main(int argc, char *argv[]) {

	std::string path, db_name, errorLog;
	if (argc < 2) {
		path = "/root/sailingrobot";
		db_name = "/asr.db";
		errorLog = "/errors.log";
	} else {
		path = std::string(argv[1]);
		db_name = "/asr.db";
		errorLog = "/errors.log";
	}

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

	printf("\n");
	printf("  Sailing Robot\n");
	printf("=================\n");


	/* Default time */
	ExternalCommand externalCommand("1970-04-10T10:53:15.1234Z",true,0,0);
	SystemState systemstate(
		SystemStateModel(
			GPSModel("",0,0,0,0,0,0),
			WindsensorModel(0,0,0),
			CompassModel(0,0,0),
			0,
			0
		)
	);

	printf("-Creating database connection...\n");
	DBHandler db;
	try {
		db.openDatabase(path+db_name);
	} catch (const char * error) {
		printf("!DB ERROR:%s\n", error);
		throw;
	}
	printf("-DONE\n");


	// Create main sailing robot controller
	SailingRobot sr(&externalCommand, &systemstate, &db);
	sr_handle = &sr;

	GPSupdater gps_updater(&systemstate,true);
	gps_handle = &gps_updater;

	try {
		printf("-Initializing...\n");
		sr.init(path, errorLog);
		printf("-DONE\n");

		printf("-Starting threads...\n");

		//start xBeeSync thread

		//	Hämtar ett heltal (1 eller 0) som visar om xbeen skall skicka och ta emot data.
		bool xBee_sending = db.retriveCellAsInt("configs", "1", "xb_send");
		bool xBee_receiving = db.retriveCellAsInt("configs", "1", "xb_recv");

		xBee_sending = false;
		xBee_receiving = false;

		if (xBee_sending || xBee_receiving) {
			xbee_handle.reset(new xBeeSync(&externalCommand, &systemstate, xBee_sending, xBee_receiving));
			std::thread xbee_sync_thread (threadXBeeSyncRun);
			xbee_sync_thread.detach();
		}
		//start GPSupdater thread
		std::thread gps_reader_thread (threadGPSupdate);
		gps_reader_thread.detach();

		printf("-Starting main loop...\n");
		sr.run();
		printf("-DONE\n");

	} catch (const char * e) {
		printf("ERROR[%s]\n\n",e);
		term(1);
		return 1;
	}

	printf("-Finished.\n");
	return 0;
}
