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
#ifdef ENABLE_GUROBI
#include <PowerTools++/GurobiProgram.h>
#endif

class Solver {
    
protected:
    union {
#ifdef ENABLE_GUROBI
        GurobiProgram* grb_prog;
#endif
        IpoptProgram* ipopt_prog;
    } prog;

public:
    SolverType                      _stype;
    /** Constructor */
    //@{
    Solver();

    Solver(Model* model, SolverType stype);
    //@}
    void set_model(Model* m);
    
    /* Destructor */
    ~Solver();
    
    int run(int output, bool relax);
};
#endif /* defined(__PowerTools____Solver__) */
