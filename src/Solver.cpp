//
//  Solver.cpp
//  PowerTools++
//
//  Created by Hassan on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#include "PowerTools++/Solver.h"


Solver::Solver(Model* m):_model(m){

    
};

Solver::Solver(Model* m, SolverType stype):_model(m){
    _stype = stype;
    switch (stype) {
        case ipopt:
            break;
        case gurobi:
            try{
                prog.grbprog = new GurobiProgram();
            }catch(GRBException e) {
                cerr << "\nError code = " << e.getErrorCode() << endl;
                cerr << e.getMessage() << endl;
                exit(1);
            }
            break;
        default:
            break;
    }
    
};

Solver::~Solver(){
//    delete _model;
    if (_stype == gurobi) delete prog.grbprog;
}


int Solver::run(int output, bool relax){
    //GurobiProgram* grbprog;
    IpoptProgram* ipopt_prog = NULL;
    IpoptApplication iapp;
    // Initialize the IpoptApplication and process the options
    ApplicationReturnStatus status;

    switch (_stype) {
        case ipopt:
            ipopt_prog = new IpoptProgram(_model);
            status = iapp.Initialize();
            if (status != Solve_Succeeded) {
                std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
                delete ipopt_prog;
                return (int) status;
            }
            
            //            iapp.Options()->SetStringValue("hessian_constant", "yes");
            //            iapp->Options()->SetStringValue("derivative_test", "second-order");
            //            iapp->Options()->SetNumericValue("tol", 1e-6);
            //            iapp->Options()->SetNumericValue("tol", 1e-6);
            //            iapp->Options()->SetStringValue("derivative_test", "second-order");
            //            iapp.Options()->SetNumericValue("bound_relax_factor", 0);
            //            iapp.Options()->SetIntegerValue("print_level", 5);
            
            //            iapp.Options()->SetStringValue("derivative_test_print_all", "yes");
            status = iapp.OptimizeTNLP(ipopt_prog);
            
            if (status == Solve_Succeeded) {
                // Retrieve some statistics about the solve
                
                //                printf("\n\nSolution of the primal variables:\n");
                //                _model->print_solution();
                
            }
            delete ipopt_prog;
            break;
        case gurobi:
            try{
                //                prog.grbprog = new GurobiProgram();
                prog.grbprog->_output = output;
                prog.grbprog->set_model(_model);
                prog.grbprog->prepare_model();
                prog.grbprog->solve(relax);
            }catch(GRBException e) {
                cerr << "\nError code = " << e.getErrorCode() << endl;
                cerr << e.getMessage() << endl;
            }
            break;
        default:
            break;
    }
    return -1;
}