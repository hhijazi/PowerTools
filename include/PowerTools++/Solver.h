//
//  Solver.h
//  PowerTools++
//
//  Created by Hassan on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____Solver__
#define __PowerTools____Solver__

#include <stdio.h>
#include <PowerTools++/Model.h>
#include <PowerTools++/IpoptProgram.h>
#include <PowerTools++/GurobiProgram.h>

class Solver {
    
protected:
    Model*                          _model;
    union {
        GurobiProgram* grbprog;
    } prog;

public:
    SolverType                      _stype;
    /** Constructor */
    //@{
    Solver();
    
    Solver(Model* m);

    Solver(Model* m, SolverType stype);
    //@}
    
    /* Destructor */
    ~Solver();
    
    int run(int output, bool relax);
};
#endif /* defined(__PowerTools____Solver__) */
