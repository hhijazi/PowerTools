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
#include <PowerTools++/Model.h>
#include <PowerTools++/IpoptProgram.h>
#ifdef USE_GUROBI
#include <PowerTools++/GurobiProgram.h>
#endif
#ifdef USE_BONMIN
#include <BonTMINLP.hpp>
#include <PowerTools++/BonminProgram.h>
#endif

class PTSolver {
    
protected:
    union {
#ifdef USE_GUROBI
        GurobiProgram* grb_prog;
#endif
        IpoptProgram* ipopt_prog;
#ifdef USE_BONMIN
        BonminProgram* bonmin_prog;
#endif
    } prog;

public:
    SolverType                      _stype;
    bool warm_start = false;
    /** Constructor */
    //@{
    PTSolver();

    PTSolver(Model* model, SolverType stype);
    //@}
    void set_model(Model* m);
    double conv_tol = 0;
    
    /* Destructor */
    ~PTSolver();
    
    int run(int output, bool relax);
};
#endif /* defined(__PowerTools____Solver__) */
