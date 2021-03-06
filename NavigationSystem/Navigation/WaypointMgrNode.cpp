/****************************************************************************************
 *
 * File:
 * 		WaypointNode.cpp
 *
 * Purpose:
 *		The WaypointNode sends information about the waypoints to the sailing logic
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#include "WaypointMgrNode.h"


WaypointMgrNode::WaypointMgrNode(MessageBus& msgBus, DBHandler& db)
: Node(NodeID::Waypoint, msgBus), m_db(db),
    m_nextId(0),
    m_nextLongitude(0),
    m_nextLatitude(0),
    m_nextDeclination(0),
    m_nextRadius(0),

    m_prevId(0),
    m_prevLongitude(0),
    m_prevLatitude(0),
    m_prevDeclination(0),
    m_prevRadius(0),
    m_totalTime( 0 )
{
    msgBus.registerNode(*this, MessageType::StateMessage);
    msgBus.registerNode(*this, MessageType::ServerWaypointsReceived);
}

bool WaypointMgrNode::init()
{
    sendMessage();
    return true;
}

void WaypointMgrNode::processMessage(const Message* msg)
{
    MessageType type = msg->messageType();

	switch(type)
	{
        case MessageType::StateMessage:
            processVesselStateMessage((StateMessage*)msg);
            break;
        case MessageType::ServerWaypointsReceived:
            sendMessage();
            break;
        default:
            return;
	}

    if(waypointReached())
    {
        sendMessage();
    }
    sendNavigationInformation();
}

void WaypointMgrNode::processVesselStateMessage(StateMessage* msg)
{
    m_vesselLongitude = msg->longitude();
    m_vesselLatitude = msg->latitude();
    // std::cout << "processVesselStateMessage long: " << m_vesselLongitude << "lat: " << m_vesselLatitude << std::endl;
}

bool WaypointMgrNode::waypointReached()
{
    // double distanceAfterWaypoint = Utility::calculateWaypointsOrthogonalLine(m_nextLongitude, m_nextLatitude, m_prevLongitude,
    //             m_prevLatitude, m_vesselLongitude, m_vesselLatitude); //Checks if boat has passed the waypoint following the line, without entering waypoints radius
    if(harvestWaypoint())
    {
        if(not m_db.changeOneValue("current_Mission", std::to_string(m_nextId),"1","harvested"))
        {
            Logger::error("Failed to harvest waypoint");
        }
        Logger::info("Waypoint %d harvested", m_nextId);
        m_waypointTimer.stop();

        m_routeTime.stop();
        int seconds = m_routeTime.timePassed();
        m_totalTime += m_routeTime.timePassed();

        int minutes = seconds / 60;
        int hours = minutes / 60;
        minutes = minutes % 60;
        seconds = seconds % 60;
        Logger::info("\tTook %d:%d:%d to complete", hours, minutes, seconds);
        m_routeTime.reset();

        return true;
    }
    else
    {
        return false;
    }
}

void WaypointMgrNode::sendMessage()
{
    bool foundPrev = false;
    
    if(m_db.getWaypointValues(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius, m_nextStayTime,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius, foundPrev))
    {
        if( !foundPrev )
        {
            m_prevLatitude = m_vesselLatitude;
            m_prevLongitude = m_vesselLongitude;
        }

        MessagePtr msg = std::make_unique<WaypointDataMsg>(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius, m_nextStayTime,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius);
        m_MsgBus.sendMessage(std::move(msg));
        // std::cout << "send WaypointDataMsg. Next id:  " << m_nextId << std::endl;

        if( !m_routeTime.started() )
        {
            m_routeTime.start();
        }
    }
    else
    {
        Logger::warning("%s No waypoint found, boat is using old waypoint data. No message sent.", __func__);
        m_routeTime.stop();

        m_totalTime += m_routeTime.timePassed();

        int seconds = m_totalTime;
        int minutes = seconds / 60;
        int hours = minutes / 60;
        minutes = minutes % 60;
        seconds = seconds % 60;

        Logger::info("Completed route in %d:%d:%d", hours, minutes, seconds);
    }

    m_db.forceUnlock();
}

bool WaypointMgrNode::harvestWaypoint()
{
    double DistanceToWaypoint = CourseMath::calculateDTW(m_vesselLongitude, m_vesselLatitude, m_nextLongitude, m_nextLatitude); //Calculate distance to waypoint
    // std::cout << "DistanceToWaypoint: " << DistanceToWaypoint << std::endl;
    if(DistanceToWaypoint > m_nextRadius)
    {
        return false;
    }

    if(m_nextStayTime > 0) //if next waypoint has a time to stay inside its radius, start the timer
    {
        m_waypointTimer.start(); //NOTE : Marc : writeTime has never been initialized
        if(not writeTime)
        {
            Logger::info("Started waypoint timer. Stay at waypoint for: %d seconds", m_nextStayTime);
            writeTime = true;
        }

        if(m_waypointTimer.timeReached(m_nextStayTime)) //Check if boat has stayed the designated amount of time
        {
            Logger::info("Waypoint timer passed");
            writeTime = false;
            return true;
        }

        return false;
    }
    else //if no timer for waypoint, harvest it
    {
        return true;
    }
}

void WaypointMgrNode::sendNavigationInformation()
{
    double distanceToWaypoint = CourseMath::calculateDTW(m_vesselLongitude, m_vesselLatitude, m_nextLongitude, m_nextLatitude);
    int16_t bearingToWaypoint = CourseMath::calculateBTW(m_vesselLongitude, m_vesselLatitude, m_nextLongitude, m_nextLatitude);

    MessagePtr msg = std::make_unique<CourseDataMsg>(0, distanceToWaypoint, bearingToWaypoint);
    m_MsgBus.sendMessage(std::move(msg));
    // std::cout << "distanceToWaypoint: " << distanceToWaypoint << "  bearingToWaypoint: " << bearingToWaypoint << std::endl;
}
