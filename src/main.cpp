//
//  main.cpp
//  MP2DAT
//
//  Created by Hassan Hijazi on 14/01/13.
//  Copyright (c) 2013 NICTA. All rights reserved.
//

#include <iostream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <fstream>
#include "PowerTools++/Net.h"
#include "PowerTools++/IpoptProgram.h"
#include "PowerTools++/Solver.h"
#include "PowerTools++/Complex.h"
#include "PowerTools++/PowerModel.h"

using namespace std;

int main (int argc, const char * argv[])
{
//    if (argc != 4) {
//        cerr << "Wrong number of arguments.\n";
//        exit(1);
//    }
    PowerModelType pmt = ACPOL;
//    PowerModelType pmt = SOCP_OTS;
//    PowerModelType pmt = ACRECT;
    SolverType st = ipopt;
//    SolverType st = gurobi;
    string filename = "../data/nesta_case3_lmbd.m";
    if (argc >=2) {
        string filename = argv[1];
    
        if(!strcmp(argv[2],"ACPOL")) pmt = ACPOL;
        else if(!strcmp(argv[2],"ACRECT")) pmt = ACRECT;
        //else if(!strcmp(argv[2],"QC")) pmt = QC;
        else if(!strcmp(argv[2],"OTS")) pmt = OTS;
        else if(!strcmp(argv[2],"SOCP")) pmt = SOCP;
        else if(!strcmp(argv[2],"DC")) pmt = DC;
        //else if(!strcmp(argv[2],"QC_OTS")) pmt = QC_OTS;
        else if(!strcmp(argv[2],"SOCP_OTS")) pmt = SOCP_OTS;
            else{
                    cerr << "Unknown model type.\n";
                    exit(1);
            }
        
        if(!strcmp(argv[3],"ipopt")) st = ipopt;
        
        else if(!strcmp(argv[3],"gurobi")) st = gurobi;
        else{
            cerr << "Unknown solver type.\n";
            exit(1);
        }

    }

    cout << "############################## POWERTOOLS ##############################\n\n";
    Net net;
    net.readFile(filename);
//    net.readFile("data/nesta/nesta_case2383wp_mp.m");
//    net.readFile("data/nesta/nesta_case300_ieee.m");
//    net.readFile("../../data/nesta/nesta_case9241_pegase.m");
//    net.readFile("../../data/nesta/nesta_case2383wp_mp.m");
//    net.readFile("../data/nesta/" + filename);
//    PowerModel power_model(pmt,&net);
    PowerModel power_model(pmt,&net,st);
    power_model.min_cost();
    power_model.solve();
    cout << "\nTo run PowerTools with a different power flow model, enter:\nPowerTools filename ACPOL/ACRECT/SOCP/DC/OTS/SOCP_OTS ipopt/gurobi\n";
    return 0;

}
