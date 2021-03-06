/****************************************************************************************
*
* File:
* 		StateMessage.h
*
* Purpose:
*		A StateMessage contains contains the state of the vessel at a given time.
*
* Developer Notes:
*
*
***************************************************************************************/

#pragma once

#include "MessageBus/Message.h"


class StateMessage : public Message {
public:

    StateMessage (NodeID destinationID, NodeID sourceID, float compassHeading,
    double lat, double lon, double gpsSpeed, double gpsCourse)
    :Message(MessageType::StateMessage, sourceID, destinationID),
    m_VesselHeading(compassHeading), m_VesselLat(lat), m_VesselLon(lon), m_VesselSpeed(gpsSpeed), m_VesselCourse(gpsCourse)
    { }

    StateMessage(float compassHeading, double lat, double lon, double gpsSpeed, double gpsCourse)
    :Message(MessageType::StateMessage, NodeID::None, NodeID::None),
    m_VesselHeading(compassHeading), m_VesselLat(lat), m_VesselLon(lon), m_VesselSpeed(gpsSpeed), m_VesselCourse(gpsCourse)
    { }

    StateMessage(MessageDeserialiser deserialiser)
    :Message(deserialiser)
    {
        if( !deserialiser.readFloat(m_VesselHeading) ||
        !deserialiser.readDouble(m_VesselLat) ||
        !deserialiser.readDouble(m_VesselLon) ||
        !deserialiser.readDouble(m_VesselCourse) ||
        !deserialiser.readDouble(m_VesselSpeed))
        {
            m_valid = false;
        }
    }

    virtual ~StateMessage() { }


    float heading() const {  return m_VesselHeading; }
    double latitude() const {  return m_VesselLat; }
    double longitude() const {  return m_VesselLon; }
    double speed() const { return m_VesselSpeed; }
    double course() const {return m_VesselCourse; }


    ///----------------------------------------------------------------------------------
    /// Serialises the message into a MessageSerialiser
    ///----------------------------------------------------------------------------------
    virtual void Serialise(MessageSerialiser& serialiser) const
    {
        Message::Serialise(serialiser);

        serialiser.serialise(m_VesselHeading);
        serialiser.serialise(m_VesselLat);
        serialiser.serialise(m_VesselLon);
        serialiser.serialise(m_VesselCourse);
        serialiser.serialise(m_VesselSpeed);
    }

private:
    float 	m_VesselHeading;     // degree [0, 360[ in North-East reference frame (clockwise)
    double	m_VesselLat;         // degree
    double	m_VesselLon;         // degree
    double	m_VesselSpeed;       // m/s
    double  m_VesselCourse;      // degree [0, 360[ in North-East reference frame (clockwise)
};
