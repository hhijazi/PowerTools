//
//  main.cpp
//  MP2DAT
//
//  Created by Hassan Hijazi on 14/01/13.
//  Copyright (c) 2013 NICTA. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <fstream>
#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Plot.h"




using namespace std;

//  Windows
#ifdef _WIN32
#include <Windows.h>
double get_wall_time(){
    LARGE_INTEGER time,freq;
    if (!QueryPerformanceFrequency(&freq)){
        //  Handle error
        return 0;
    }
    if (!QueryPerformanceCounter(&time)){
        //  Handle error
        return 0;
    }
    return (double)time.QuadPart / freq.QuadPart;
}
double get_cpu_time(){
    FILETIME a,b,c,d;
    if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
        //  Returns total user time.
        //  Can be tweaked to include kernel times as well.
        return
        (double)(d.dwLowDateTime |
                 ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
    }else{
        //  Handle error
        return 0;
    }
}


//  Posix/Linux
#else
#include <time.h>
#include <sys/time.h>
double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time(){
    return (double)clock() / CLOCKS_PER_SEC;
}
#endif



int main (int argc, const char * argv[]) {
    
//    auto i = 3./10000;
//    cout << "i = " << i << endl;
//    return 0;
//    
//    if (argc != 4) {
//        cerr << "Wrong number of arguments.\n";
//        exit(1);
//    }
//    PowerModelType pmt = ACPOL;
//    PowerModelType pmt = SDP;
//    setenv("GRB_LICENSE_FILE", "/home/kbestuzheva/gurobi.research.lic", 1);

//    PowerModelType pmt = ACPF;
      PowerModelType pmt = ACPF_T;
//      PowerModelType pmt = SOCP;
//      PowerModelType pmt = SOCP_T;
//      PowerModelType pmt = ACPF_PV_T;
//    PowerModelType pmt = ACPF_BATT_T_NO_GEN;
//      PowerModelType pmt = SOCP_BATT_T_NO_GEN;
//    PowerModelType pmt = ACPF_BATT_T;
//    PowerModelType pmt = ACPF_PV_BATT_T;
//    PowerModelType pmt = QC_OTS_N;
//    PowerModelType pmt = GRB_TEST;
//   PowerModelType pmt = SOCP_PV_T;
//      PowerModelType pmt = SOCP_BATT_T;
//    PowerModelType pmt = SOCP_PV_BATT_T;

    //  Start Timers

//    PowerModelType pmt = SDP;
//    PowerModelType pmt = QC;
//    PowerModelType pmt = QC_SDP;
//    PowerModelType pmt = SOCP_OTS;
//    PowerModelType pmt = ACRECT;
    SolverType st = ipopt;
//    SolverType st = gurobi;

//    string filename = "/home/angela/DEV/PowerTools/data/anu.m";
//    string loadfile = "/home/angela/DEV/PowerTools/data/loadfile-24.csv";
//    string radiationfile = "/home/angela/DEV/PowerTools/data/radiationfile-24-june.csv";
//    string costfile = "/home/angela/DEV/PowerTools/data/gencost-24-recalculated.csv";
           string filename = "../data/anu.m";
           string loadfile = "../data/Jan_16_1hr_24h.csv";
//            string loadfile = "../data/July_16_1hr_24h.csv"; //*
//            string loadfile = "../data/February_16_1hr_24h.csv";  //*
//            string loadfile = "../data/Weekend_Feb_16.csv";  //*
//            string loadfile = "../data/July_weekend_16.csv";  //*
//               string loadfile = "../data/September_16_1hr_24h.csv";
//           string loadfile = "../data/June_16_1hr_24h.csv";
//           string radiationfile="../data/radiationfile-24-july.csv";
//        string radiationfile="../data/radiationfile-24-february.csv";
        string radiationfile="../data/radiationfile-24-january.csv";
//        string radiationfile="../data/radiationfile-24-june.csv";
//        string radiationfile="../data/radiationfile-24-september.csv";
           string costfile = "../data/gencost-24.csv";
#ifdef __APPLE__
    filename = "../" + filename;
    loadfile  = "../" + loadfile;
    radiationfile = "../" + radiationfile;
    costfile = "../" + costfile;
#endif

    ////        string pvfile = "../../data/pvmax.csv";


//    string filename = "/Users/hassan/Documents/Dropbox/Work/Dev/Private_PT/data/nesta_case30_ieee.m";
//    string filename = "../../data/nesta_case3_lmbd.m";
//    string filename = "../data/nesta_case118_ieee__api.m";
//    string filename = "../../data/nesta_case29_edin__sad.m";
//    string filename = "../../data/nesta_case24_ieee_rts.m";
//    string filename = "../../data/nesta_case24_ieee_rts__api.m";
//    string filename = "../../data/nesta_case30_ieee.m";
//    string filename = "../../data/nesta_case57_ieee.m";
//    string filename = "../../data/nesta_case30_as__sad.m";
//    string filename = "../../data/nesta_case24_ieee_rts__sad.m";
//    string filename = "../../data/nesta_case2224_edin.m";
//    string filename = "../../data/nesta_case14_ieee.m";
//    string filename = "../../data/nesta_case162_ieee_dtc.m";
//    string filename = "../../data/nesta_case5_pjm.m";
    if (argc >= 2) {
        filename = argv[1];

        if (!strcmp(argv[2], "ACPOL")) pmt = ACPOL;
        else if (!strcmp(argv[2], "ACRECT")) pmt = ACRECT;
        else if (!strcmp(argv[2], "QC")) pmt = QC;
        else if (!strcmp(argv[2], "QC_SDP")) pmt = QC_SDP;
        else if (!strcmp(argv[2], "OTS")) pmt = OTS;
        else if (!strcmp(argv[2], "SOCP")) pmt = SOCP;
        else if (!strcmp(argv[2], "SOCP_PV_T")) pmt = SOCP_PV_T;
        else if (!strcmp(argv[2], "ACPF_PV_T")) pmt = ACPF_PV_T;
        else if (!strcmp(argv[2], "SOCP_BATT_T")) pmt = SOCP_PV_BATT_T;
        else if (!strcmp(argv[2], "ACPF_PV_BATT_T")) pmt = ACPF_PV_BATT_T;
        else if (!strcmp(argv[2], "ACPF_BATT_T")) pmt = ACPF_BATT_T;
        else if (!strcmp(argv[2], "SOCP_BATT_T")) pmt = SOCP_BATT_T;
        else if (!strcmp(argv[2], "SOCP_PV_BATT_T")) pmt = SOCP_PV_BATT_T;
        else if (!strcmp(argv[2], "SOCP_T")) pmt = SOCP_T;
        else if (!strcmp(argv[2], "ACPF_T")) pmt = ACPF_T;
        else if (!strcmp(argv[2], "SDP")) pmt = SDP;
        else if (!strcmp(argv[2], "DC")) pmt = DC;
        else if (!strcmp(argv[2], "QC_OTS_O")) pmt = QC_OTS_O;
        else if (!strcmp(argv[2], "QC_OTS_N")) pmt = QC_OTS_N;
        else if (!strcmp(argv[2], "QC_OTS_L")) pmt = QC_OTS_L;
        else if (!strcmp(argv[2], "SOCP_OTS")) pmt = SOCP_OTS;
        else {
            cerr << "Unknown model type.\n";
            exit(1);
        }

        if (!strcmp(argv[3], "ipopt")) st = ipopt;

        else if (!strcmp(argv[3], "gurobi")) st = gurobi;
        else if (!strcmp(argv[3], "bonmin")) st = bonmin;
        else {
            cerr << "Unknown solver type.\n";
            exit(1);
        }

    }

    cout << "############################## POWERTOOLS ##############################\n\n";
    Net net;


    int timesteps = 24;

    if (net.readFile(filename) == -1)
        return -1;


    if (net.readload(loadfile, timesteps) == -1) {
        return -1;
    };


    if (net.readcost(costfile, timesteps) == -1)
        return -1;

//    if(net.readpvmax(pvfile)==-1)
//        return -1;
//

    if (net.readrad(radiationfile, timesteps) == -1)
        return -1;

//    if(net.choosetime()==-1)                 
//        return -1;
//  





//    net.readFile("data/nesta/nesta_case2383wp_mp.m");
//    net.readFile("data/nesta/nesta_case300_ieee.m");
//    net.readFile("../../data/nesta/nesta_case9241_pegase.m");
//    net.readFile("../../data/nesta/nesta_case2383wp_mp.m");
//    net.readFile("../data/nesta/" + filename);
//    PowerModel power_model(pmt,&net);

    cout <<
    "\nTo run PowerTools with a different input/power flow model, enter:\nPowerTools filename ACPOL/ACRECT/SOCP/QC/QC_SDP/SDPDC/OTS/SOCP_OTS ipopt/gurobi\n\n";
    PowerModel power_model(pmt, &net, st);
    //power_model.propagate_bounds();
    double wall0 = get_wall_time();
    double cpu0 = get_cpu_time();
    power_model.build(timesteps);
    int status = power_model.solve();

    //  Stop timers
    double wall1 = get_wall_time();
    double cpu1 = get_cpu_time();


    /*cout << "ALL_DATA, " << net._name << ", " << net.nodes.size() << ", " << net.arcs.size() << ", " <<
    power_model._model->_opt << ", " << status << ", " << wall1 - wall0 << ", -inf\n";*/

//    ofstream fw;
//    fw.open("text.txt");
//    std::streambuf *oldbuf = std::cout.rdbuf();
//    std::cout.rdbuf(fw.rdbuf());
//    ///
//    power_model._model->print_solution();
    cout << "OPTIMAL COST = " << power_model._model->_opt << endl;
//    power_model._model->_obj->print(true); //obj->print(true);
    
    ///
//    std::cout.rdbuf(oldbuf);
    cout << "\n";
    /*std::ofstream fileout("out.txt");
    fileout << power_model._model->print_solution();*/


/*
    float sum_power_loss = 0;

    for (int t = 0; t < power_model._timesteps; t++) {

        for (auto a:net.arcs) {

            cout << "Power loss" <<"_" <<a->_name <<"_"<<a->src->_name<< "_"<<a->dest->_name<< "(bMVA)"<< "="<<" " <<a->pi_t[t].get_value() + a->pj_t[t].get_value() << endl;


            sum_power_loss += a->pi_t[t].get_value() + a->pj_t[t].get_value();

        }

        cout << "Total power loss =" << sum_power_loss;

        return 0;

    }*/

/* Plotting functions */
    plot p;
    if(pmt==ACPF_PV_T || pmt==ACPF_PV_BATT_T || pmt==ACPF_BATT_T){
//        p.plot_V( argc, argv, power_model);
    }
    if(pmt==ACPF_PV_T || pmt==ACPF_PV_BATT_T){
//        p.plot_PV( argc, argv, power_model);
    }
    p.plot_flow( argc, argv, power_model);
    if(pmt==SOCP_PV_BATT_T || pmt==SOCP_BATT_T || pmt==ACPF_PV_BATT_T || pmt==ACPF_BATT_T){
        p.plot_soc(argc, argv, power_model);
    }
  //plot *x = new plot(argc, argv, power_model);


    cout << "Model: " << pmt << ", " << net._name << ", " << net.nodes.size() << ", " << net.arcs.size() << ", " <<
    power_model._model->_opt << ", " << status << ", " << wall1 - wall0 << ", -inf\n";
    cout << "OPTIMAL COST = " << power_model._model->_opt << endl;

    
    return 0;

}
