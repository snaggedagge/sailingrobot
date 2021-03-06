/****************************************************************************************
*
* File:
* 		CANArduinoNode.h
*
* Purpose:
*		 Sends data to the Actuator unit with the rudder and wingsail angles over the CAN bus. 
*
* Developer Notes:
*		 The CAN frame ID nubmer that this node subscribe to are:
*			700
*
***************************************************************************************/

#pragma once

#include "Math/Utility.h"
#include "MessageBus/MessageBus.h"
#include "MessageBus/MessageTypes.h"
#include "Messages/RudderCommandMsg.h"
#include "Messages/WingSailCommandMsg.h"
#include "Hardwares/CAN_Services/CANService.h"

class ActuatorNodeASPire : public Node {
public:
	ActuatorNodeASPire(MessageBus& msgBus, CANService& canService);
	~ActuatorNodeASPire();
	bool init();
	void processMessage(const Message* message);
	void sendCommandMessage();

private:
	CANService* m_CANService;
	const float MAX_RUDDER_ANGLE = 30;
	const float MAX_WINGSAIL_ANGLE = 13;
	const float INT16_SIZE = 65535;
	double m_rudderAngle;		// degree in vessel reference frame (counter clockwise from top view)
	double m_wingsailAngle;		// tail angle degree in wing sail reference frame (clockwise from top view)
	bool m_windvaneSelfSteeringOn;
	
};