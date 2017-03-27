//
//  PTSolver.cpp
//  PowerTools++
//
//  Created by Hassan on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#include "PowerTools++/PTSolver.h"
#include <coin/BonBonminSetup.hpp>
#include <coin/BonCbc.hpp>




PTSolver::PTSolver(Model* model, SolverType stype){
    using namespace Ipopt;

    _stype = stype;
    switch (stype) {
        case ipopt:
            prog.ipopt_prog = new IpoptProgram(model);
            break;
        case gurobi:
            try{
                prog.grb_prog = new GurobiProgram(model);
            }catch(GRBException e) {
                cerr << "\nError code = " << e.getErrorCode() << endl;
                cerr << e.getMessage() << endl;
                exit(1);
            }
            break;
        case bonmin:
            prog.bonmin_prog = new BonminProgram(model);
            break;
        default:
            break;
    }
    
};

PTSolver::~PTSolver(){
    if (_stype == gurobi) delete prog.grb_prog;
    if (_stype == ipopt) delete prog.ipopt_prog;
}

void PTSolver::set_model(Model* m) {
    if (_stype == gurobi) prog.grb_prog->model = m;
    if (_stype == ipopt) prog.ipopt_prog->model = m; 
}


int PTSolver::run(int output, bool relax){
    //GurobiProgram* grbprog;
    // Initialize the IpoptApplication and process the options

    if (_stype==ipopt) {
            //IpoptApplication iapp;
            SmartPtr<IpoptApplication> iapp = IpoptApplicationFactory();
            ApplicationReturnStatus status = iapp->Initialize();
            if (status != Solve_Succeeded) {
                std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
                return (int) status;
            }

        SmartPtr<TNLP> tmp = new IpoptProgram(prog.ipopt_prog->model);
//        prog.ipopt_prog;
            //            iapp.Options()->SetStringValue("hessian_constant", "yes");
//                        iapp.Options()->SetStringValue("derivative_test", "second-order");
            //            iapp->Options()->SetNumericValue("tol", 1e-6);
                        iapp->Options()->SetNumericValue("tol", 1e-6);
            //            iapp->Options()->SetStringValue("derivative_test", "second-order");
            //            iapp.Options()->SetNumericValue("bound_relax_factor", 0);
            //            iapp->Options()->SetIntegerValue("print_level", 1);
            
            //            iapp.Options()->SetStringValue("derivative_test_print_all", "yes");
            status = iapp->OptimizeTNLP(tmp);
            
            if (status == Solve_Succeeded) {
                // Retrieve some statistics about the solve
                
                //                printf("\n\nSolution of the primal variables:\n");
                //                _model->print_solution();
//                return status;
                return 100;
            }
        if (status == Solved_To_Acceptable_Level) {
            return 150;
        }
    }
    else if(_stype==gurobi)
            try{
                //                prog.grbprog = new GurobiProgram();
                prog.grb_prog->_output = output;
                prog.grb_prog->reset_model();
                prog.grb_prog->prepare_model();
                prog.grb_prog->solve(relax);
            }catch(GRBException e) {
                cerr << "\nError code = " << e.getErrorCode() << endl;
                cerr << e.getMessage() << endl;
            }
    else if(_stype==bonmin) {
        BonminSetup bonmin;
        bonmin.initializeOptionsAndJournalist();
        bonmin.initialize(prog.bonmin_prog);
        try {
            Bab bb;
            bb(bonmin);
        }
        catch(TNLPSolver::UnsolvedError *E) {
            //There has been a failure to solve a problem with Ipopt.
            std::cerr<<"Ipopt has failed to solve a problem"<<std::endl;
        }
        catch(OsiTMINLPInterface::SimpleError &E) {
            std::cerr<<E.className()<<"::"<<E.methodName()
            <<std::endl
            <<E.message()<<std::endl;
        }
        catch(CoinError &E) {
            std::cerr<<E.className()<<"::"<<E.methodName()
            <<std::endl
            <<E.message()<<std::endl;
        }
    }
    return -1;
}