//
//  Conductor.h
//  PowerTools++
//
//  Created by Hassan on 3/02/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____Conductor__
#define __PowerTools____Conductor__

#include <stdio.h>

#include <vector>

class Bus;

class Conductor {
    
public:
    /** @brief Conductor active power load */
    std::vector<double> _pl;

    /** @brief Conductor pv limit */
    std::vector<double> _pvmax;
    
    /** @brief Conductor reactive power load */
    double _ql;
    
    /** @brief Conductor shunt b value */
    double _bs;
    /** @brief Conductor shunt g value */
    double _gs;
    
    /** @brief Conductor phase */
    int _phase;
    
    /** @brief Corresponding bus */
    Bus* bus;
    
    /**
     @brief Initialiser
     @note Designated initialiser
     */
    Conductor();
    
    
    /**
     @brief Initialiser with linked bus and conductor properties
     */
    Conductor(Bus* b, double pl, double ql, double gs, double bs, int phase);
    
    /** @brief Memory release */
    ~Conductor();
    
};
#endif /* defined(__PowerTools____Conductor__) */
