/****************************************************************************************
*
* File:
* 		IntegrationTestASPire.cpp
*
* Purpose:
		 Global Integrationtest for the ASPire
*		 Monitor the values from the CAN-bus and able to sen comands to the actuators
*
* Developer Notes:
		 Currently monetriring windsensor, actuator feedbacka and if the radio controller is in manual mode.
		 It is a interface between the messagebus and the CAN-bus that can be monitored.
*
*
*
***************************************************************************************/



#include "Hardwares/CAN_Services/CANService.h"
#include "Hardwares/CAN_Services/N2kMsg.h"
#include "SystemServices/Timer.h"
#include "SystemServices/Logger.h"

#include "Messages/ASPireActuatorFeedbackMsg.h"
#include "Messages/RudderCommandMsg.h"
#include "Messages/WingSailCommandMsg.h"
#include "Messages/WindDataMsg.h"
#include "Messages/CompassDataMsg.h"
#include "Messages/GPSDataMsg.h"
#include "Messages/AISDataMsg.h"

#include "MessageBus/MessageTypes.h"
#include "MessageBus/MessageBus.h"
#include "MessageBus/ActiveNode.h"
#include "MessageBus/NodeIDs.h"

#include "Hardwares/CANWindsensorNode.h"
#include "Hardwares/CANArduinoNode.h"
#include "Hardwares/ActuatorNodeASPire.h"
#include "Hardwares/HMC6343Node.h"
#include "Hardwares/GPSDNode.h"
#include "Hardwares/CANAISNode.h"

#include <ncurses.h>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cctype>
#include <sstream>

#define BACKSPACE 8
#define ENTER 10
#define TAB 9

#define LONGEST_INPUT 20

#define DATA_OUT_OF_RANGE -2000
#define ON -3000
#define OFF -4000

typedef std::unordered_map<std::string, float> SensorData;

/*
	To add new sensors, the sensors should probably send out their data
	on the message bus. Then do the following:
		* Register for the new message
		* Process it and store its data
		* Add the new member fields to the print function.
*/

class SensorDataReceiver : public Node {
public:
	SensorDataReceiver(MessageBus& msgBus) :
	Node(NodeID::None, msgBus)
	{
		msgBus.registerNode(*this, MessageType::WindData);
		msgBus.registerNode(*this, MessageType::ASPireActuatorFeedback);
		msgBus.registerNode(*this, MessageType::CompassData);
		msgBus.registerNode(*this, MessageType::GPSData);
		msgBus.registerNode(*this, MessageType::AISData);

		m_SensorValues["Rudder Angle"] = DATA_OUT_OF_RANGE;
		m_SensorValues["Wingsail Angle"] = DATA_OUT_OF_RANGE;
		m_SensorValues["Wind Speed"] = DATA_OUT_OF_RANGE;
		m_SensorValues["Wind Direction"] = DATA_OUT_OF_RANGE;
		m_SensorValues["Wind Temperature"] = DATA_OUT_OF_RANGE;
		m_SensorValues["RC Mode"] = DATA_OUT_OF_RANGE;
		m_SensorValues["Heading"] = DATA_OUT_OF_RANGE;
    	m_SensorValues["Roll"] = DATA_OUT_OF_RANGE;
    	m_SensorValues["Pitch"] = DATA_OUT_OF_RANGE;
		m_SensorValues["GPS Longitude"] = DATA_OUT_OF_RANGE;
		m_SensorValues["GPS Latitude"] = DATA_OUT_OF_RANGE;
		m_SensorValues["GPS Online"] = DATA_OUT_OF_RANGE;
		m_SensorValues["AIS Longitude"] = DATA_OUT_OF_RANGE;
		m_SensorValues["AIS Latitude"] = DATA_OUT_OF_RANGE;

		m_Win = newwin(6+2*m_SensorValues.size(),60,1,2);

		box(m_Win,0,0);
		keypad(m_Win, FALSE);
		wrefresh(m_Win);
	}

	bool init() { return true; }


	void processMessage(const Message* message) {
		MessageType type = message->messageType();
		switch (type){

			case MessageType::ASPireActuatorFeedback:
				{
				const ASPireActuatorFeedbackMsg* actmsg = dynamic_cast<const ASPireActuatorFeedbackMsg*>(message);
				m_SensorValues["Rudder Angle"] = actmsg->rudderFeedback();
				m_SensorValues["Wingsail Angle"] = actmsg->wingsailFeedback();
				if (actmsg->radioControllerOn()){
					m_SensorValues["RC Mode"] = ON;
				} else {
					m_SensorValues["RC Mode"] = OFF;
				}
				}
				break;

			case MessageType::WindData:
				{
				const WindDataMsg* windmsg = dynamic_cast<const WindDataMsg*>(message);
				m_SensorValues["Wind Speed"] = windmsg->windSpeed();
				m_SensorValues["Wind Direction"] = windmsg->windDirection();
				m_SensorValues["Wind Temperature"] = windmsg->windTemp();
				}
				break;


			case MessageType::CompassData:
				{
				const CompassDataMsg* compassmsg = dynamic_cast<const CompassDataMsg*>(message);
		        m_SensorValues["Heading"] = compassmsg->heading();
		        m_SensorValues["Pitch"] = compassmsg->pitch();
		        m_SensorValues["Roll"] = compassmsg->roll();
				}
				break;

			case MessageType::GPSData:
				{
				const GPSDataMsg* gpsdata = dynamic_cast<const GPSDataMsg*>(message);
				m_SensorValues["GPS Latitude"] = gpsdata->latitude();
				m_SensorValues["GPS Longitude"]	= gpsdata->longitude();

				if (gpsdata->gpsOnline()){
					m_SensorValues["GPS Online"] = ON;
				}else{
					m_SensorValues["GPS Online"] = OFF;
				}

				}
				break;

			case MessageType::AISData:
				{
					const AISDataMsg* aisdata = dynamic_cast<const AISDataMsg*>(message);
					m_SensorValues["AIS Latitude"] = aisdata->posLat();
					m_SensorValues["AIS Longitude"] = aisdata->posLon();
				}
				break;

				default:
				break;

				}
		printSensorData();
	}

	void printSensorData() {

		wclear(m_Win);
		box(m_Win, 0, 0);

		wmove(m_Win, 2,20);
		wprintw(m_Win, "SENSOR READINGS");
		wmove(m_Win, 2, 10);
		int pos = 4;
		for(auto it : m_SensorValues) {
			wmove(m_Win, pos, 10);
			wprintw(m_Win, "%s : ", it.first.c_str());
			wmove(m_Win, pos, 35);
			if(it.second == -2000) {
				wprintw(m_Win, "%s", "Data not available.");
			} else if (it.second == ON) {
				wprintw(m_Win, "%s", "On");
			} else if (it.second == OFF) {
				wprintw(m_Win, "%s", "Off");
			} else {
				wprintw(m_Win, "%f", it.second);
			}
			pos+=1;
		}

		wrefresh(m_Win);
	}

	SensorData getValues() {
		return m_SensorValues;
	}

private:


	SensorData m_SensorValues;

	WINDOW* m_Win;

};


CANService canService;
MessageBus msgBus;

void messageLoop() {
	msgBus.run();
}

std::string FloatToString (float number) {
	std::ostringstream buff;
	buff<<number;
	return buff.str();
}


std::map<std::string, std::string> menuValues;
std::map<std::string, std::string> lastSentValues;
typedef std::map<std::string, std::string>::iterator menuIter;

void printInputMenu(WINDOW* win, menuIter highlightedItem) {
	wclear(win);
	box(win, 0,0);

	wmove(win, 2, 20);
	wprintw(win, "ACTUATOR COMMANDS");
	int pos = 4;
	for(auto it : menuValues) {
		if(it == *highlightedItem) {
			wattron(win, A_REVERSE);
			mvwprintw(win, pos, 5, "%s", it.first.c_str());
			wattroff(win, A_REVERSE);
			wprintw(win, "\t :");
		} else {
			mvwprintw(win, pos, 5, "%s", it.first.c_str());
			wprintw(win, "\t :");
		}
		mvwprintw(win, pos, 30, "%s", it.second.c_str());
		pos+=1;
	}

	mvwprintw(win, pos, 20, "PRESS ENTER TO SEND");
	wrefresh(win);

}

void sendActuatorCommands() {

	int rudderAngle16;
	int wingsailAngle16;
	//int windvaneAngle16; //For when the windvane is implemented

	for(auto& it : menuValues) {
		if(it.second.empty()) {
			it.second = lastSentValues[it.first];
		}
	}

	try {
		rudderAngle16 = stoi(menuValues["Rudder Angle"]);
		wingsailAngle16 = stoi(menuValues["Wingsail Angle"]);
		//windvaneAngle16 = stoi(menuValues["Windvane Angle"]); //For when the windvane is implemented
	} catch(std::invalid_argument ex) {
		std::cout << std::endl << "Actuator commands only works with integers." << std::endl << std::endl;
		return;
	}

	MessagePtr rudderMsg = std::make_unique<RudderCommandMsg>(rudderAngle16);
	msgBus.sendMessage(std::move(rudderMsg));
	MessagePtr wingSailMsg = std::make_unique<WingSailCommandMsg>(wingsailAngle16);
	msgBus.sendMessage(std::move(wingSailMsg));

	lastSentValues = menuValues;
}

int main() {

	// Initialize Ncurses
	initscr();
	Logger::init("integrationTest.log");

	// Database Path
	std::string db_path = "../asr.db";
	// Declare DBHandler and MessageBus
	DBHandler dbHandler(db_path);
	// Initialise DBHandler
	if(dbHandler.initialise())
	{
		Logger::info("Database Handler init\t\t[OK]");
	}
	else
	{
		Logger::error("Database Handler init\t\t[FAILED]");
		Logger::shutdown();
		exit(1);
	}

	// Comment out this line if not running on the pi
	// otherwise program will crash.
	auto future = canService.start();


	SensorDataReceiver sensorReceiver(msgBus);
	CANWindsensorNode windSensor(msgBus, dbHandler, canService);
	HMC6343Node compass(msgBus, dbHandler);
	compass.init ();



	CANArduinoNode arduino (msgBus, dbHandler, canService);
	ActuatorNodeASPire actuators (msgBus, canService);
	GPSDNode gps (msgBus, dbHandler);
	gps.init();
	CANAISNode ais (msgBus, dbHandler, canService);

	ais.start();
	gps.start();
	windSensor.start();
	arduino.start ();
	compass.start ();

	std::thread thr(messageLoop);
	thr.detach();
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	sensorReceiver.printSensorData();

	menuValues["Rudder Angle"] = "";
	menuValues["Wingsail Angle"] = "";
   // menuValues["Windvane Angle"] = "";

	lastSentValues = menuValues;

	lastSentValues["Rudder Angle"] = "0";
	lastSentValues["Wingsail Angle"] = "0";
	//lastSentValues["Windvane Angle"] = "0";

	menuIter highlighted = menuValues.begin();

	SensorData values = sensorReceiver.getValues();
	WINDOW* inputWin  = newwin(8+2*menuValues.size(),60, 2*values.size() + 10,2);
	keypad(inputWin, TRUE);
	cbreak();

	printInputMenu(inputWin, highlighted);
	while(1) {

		int c = wgetch(inputWin);

		if(isdigit(c)) {
			c -= 48;
			if(highlighted->second.size() <= LONGEST_INPUT) {
				highlighted->second += std::to_string(c);
			}
		} else if (c == '-'){
				if(highlighted->second.size() <= LONGEST_INPUT) {
					highlighted->second += c;
				}
		}else {
			switch(c) {
				case KEY_DOWN:
					if(highlighted != --menuValues.end()) {
						highlighted++;
					}
					break;
				case KEY_UP:
					if(highlighted != menuValues.begin()) {
						--highlighted;
					}
					break;
				case KEY_BACKSPACE:
					if(!highlighted->second.empty()) {
						highlighted->second.pop_back();
					}
					break;
				case ENTER:
					sendActuatorCommands();
					for(auto& it : menuValues) {
						it.second = "";
					}
					break;
				case TAB:
					if(highlighted == --menuValues.end()) {
						highlighted = menuValues.begin();
					} else {
						++highlighted;
					}
			}
		}
		printInputMenu(inputWin, highlighted);
	}
	endwin();
}
