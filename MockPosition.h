/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MockPositio.h
 * Author: sailbot
 *
 * Created on April 27, 2016, 11:49 AM
 */

#ifndef MOCKPOSITIO_H
#define MOCKPOSITIO_H

#include "Position.h"

class MockPosition : public Position  {
public:
    MockPosition();
    ~MockPosition();
    
    void setHeading(int heading);
    
    void setCourseToSteer(double cts);
    
    int getHeading();
    
    void updatePosition();
    
    PositionModel getModel();
    
private:
    PositionModel m_positionModel;
    int m_heading;
    double m_courseToSteer;
    
    void mockLatitude();
    void mockLongitude();
};
#endif /* MOCKPOSITIO_H */

