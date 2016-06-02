//
//  Conductor.cpp
//  PowerTools++
//
//  Created by Hassan on 3/02/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#include "PowerTools++/Conductor.h"

Conductor::Conductor(Bus* b, double pl, double ql, double gs, double bs, int phase):bus(b), _ql(ql), _bs(bs), _gs(gs), _phase(phase){
    _pl.push_back(pl);
}

Conductor::~Conductor(){};