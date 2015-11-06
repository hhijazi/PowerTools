//
//  PTSolver.h
//  PowerTools++
//
//  Created by Hassan on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____Solver__
#define __PowerTools____Solver__

#include <stdio.h>
#include <BonTMINLP.hpp>
#include <PowerTools++/Model.h>
#include <PowerTools++/IpoptProgram.h>
#include <PowerTools++/GurobiProgram.h>
#include <PowerTools++/BonminProgram.h>

class PTSolver {
    
protected:
    union {
        GurobiProgram* grb_prog;
        IpoptProgram* ipopt_prog;
        BonminProgram* bonmin_prog;
    } prog;

public:
    SolverType                      _stype;
    /** Constructor */
    //@{
    PTSolver();

    PTSolver(Model* model, SolverType stype);
    //@}
    void set_model(Model* m);
    
    /* Destructor */
    ~PTSolver();
    
    int run(int output, bool relax);
};
#endif /* defined(__PowerTools____Solver__) */
