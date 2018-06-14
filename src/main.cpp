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
#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Plot.h"

#include "PowerTools++/Net.h"
#include "PowerTools++/IpoptProgram.h"
#include "PowerTools++/PTSolver.h"
#include "PowerTools++/Complex.h"
#include <cstdint>
#include <functional>
#include <chrono>
#include <thread>
#include <future>
#include <condition_variable>
#include <iostream>
#include <mutex>

class Ticker {
public:
    typedef std::chrono::duration<int64_t, std::nano> tick_interval_t;
    typedef std::function<void()> on_tick_t;
    
    Ticker (std::function<void()> onTick, std::chrono::duration<int64_t, std::nano> tickInterval)
    : _onTick (onTick)
    , _tickInterval (tickInterval)
    , _running (false) {}
    ~Ticker () {}
    
    void start () {
        if (_running) return;
        _running = true;
        std::thread run(&Ticker::timer_loop, this);
        run.detach();
    }
    
    void stop () { _running = false; }
    
    void setDuration(std::chrono::duration<int64_t, std::nano> tickInterval)
    {
        _tickIntervalMutex.lock();
        _tickInterval = tickInterval;
        _tickIntervalMutex.unlock();
    }
    
private:
    void timer_loop()
    {
        while (_running) {
            std::thread run(_onTick );
            run.detach();
            
            _tickIntervalMutex.lock();
            std::chrono::duration<int64_t, std::nano> tickInterval = _tickInterval;
            _tickIntervalMutex.unlock();
            std::this_thread::sleep_for( tickInterval );
        }
    }
    
    on_tick_t           _onTick;
    tick_interval_t     _tickInterval;
    volatile bool       _running;
    std::mutex          _tickIntervalMutex;
};

void tick()
{
    std::cout << "...\n";
}




using namespace std;


void printfcomma (int n) {
    int n2 = 0;
    int scale = 1;
    if (n < 0) {
        printf ("-");
        n = -n;
    }
    while (n >= 1000) {
        n2 = n2 + scale * (n % 1000);
        n /= 1000;
        scale *= 1000;
    }
    printf ("%d", n);
    while (scale != 1) {
        scale /= 1000;
        n = n2 / scale;
        n2 = n2  % scale;
        printf (",%03d", n);
    }
}


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
    

    SolverType st = ipopt;
    

    PowerModelType pmt = COPPER_PEAK;
    cout << "############################## GreenSim ##############################\n\n";
    Net net;


    int timesteps = 24;

//    string filename = "../../data/anu.m";
//    if (net.readFile(filename) == -1)
//        return -1;
//
//    string mapfile = "../../data/ID_mapping_building.csv";
//    if (net.readmap(mapfile, timesteps) == -1)
//        return -1;

    if (argc > 1) {
        net._nb_years = atoi(argv[1]);
    }
    if (argc > 2) {
        net._pv_cost = atof(argv[2]);
    }
    if (argc > 3) {
        net._demand_growth = atof(argv[3]);
    }
    if (argc > 4) {
        net._price_inflation = atof(argv[4]);
    }
    if (argc > 5) {
        net.PV_CAP = atof(argv[5]);
    }
    if (argc > 6) {
        net.PV_EFF = atof(argv[6]);
    }
    if (argc > 7) {
        net.BATT_CAP = atof(argv[7]);
    }
    if (argc > 8) {
        net.BATT_EFF = atof(argv[8]);
    }
    if (argc > 9) {
        net._nb_samples = atoi(argv[9]);
    }
    if (argc > 10) {
        net._uncert_perc = atof(argv[10]);
    }
    if (argc > 11) {
        net._demand_nb_years = atoi(argv[11]);
    }
    if (argc > 12) {
        net._irrad_nb_years = atoi(argv[12]);
    }

    /* This is for reading irradiance data with a minute granularity */
//    if (net.read_agg_rad_all("/Users/hij001/Downloads/Solar_Data") == -1) //        return -1;

    string paramfile = "../../data/Params.csv";
    if (net.readparams(paramfile) == -1){
        cerr << "Unable to read Params.csv";
        return -1;
    }
    
    for (int i = 0; i<net._irrad_nb_years; i++) {
        string irradiancefile="../../data/Irradiance_"+to_string(i)+".csv";
        if (net.read_agg_rad(irradiancefile) == -1) {
            cerr << "Unable to read Irradiance Data\n";
            cerr << "Cannot find file: " << irradiancefile;
            return -1;
        }
        
    }
    for (int i = 0; i<net._demand_nb_years; i++) {
        string loadfile = "../../data/Demand_"+to_string(i)+".csv";
        if (net.read_agg_load(loadfile) == -1) {
            cerr << "Unable to read Demand Data\n";
            cerr << "Cannot find file: " << loadfile;
            return -1;
        }
    }
    string costfile = "../../data/Electricity_Rates.csv";
    if (net.readcost(costfile, timesteps) == -1){
        cerr << "Unable to read Electricity_Rates.csv";
        return -1;
    }
    
    
    
    pmt = COPPER_PEAK;
    PowerModel power_model(pmt, &net, st);
    double max_irr = *max_element(begin(power_model._net->_radiation), end(power_model._net->_radiation));
//          cout << "Number of weather data points = " + to_string(power_model._net->_radiation.size()) << endl;
//          cout << "Number of load data points = " + to_string(power_model._net->_load_kW.size()) << endl;
    for (int i = 0; i < power_model._net->_radiation.size(); i++) {
        power_model._net->_radiation[i] /= max_irr; 
//        power_model._net->_radiation[i] *= 6.8 * 1e-3; /* Area corresponding to a 1kW PV = 6.8 square meters */
//        assert(power_model._net->_radiation[i]<=1);
    }
    power_model.add_COPPER_vars();    
    int diff_PV[net._nb_samples], bill[net._nb_samples], diff_Batt[net._nb_samples], diff_PV_Batt[net._nb_samples];
    int min_diff_PV = __INT_MAX__, min_diff_Batt = __INT_MAX__, min_diff_PV_Batt = __INT_MAX__;
    int max_diff_PV = 0, max_diff_Batt = 0, max_diff_PV_Batt = 0;
    double tot_PV = 0, tot_Batt = 0, tot_PV_Batt = 0;
    std::chrono::duration<int, std::milli> timer_duration(2500);
    Ticker ticker(std::function<void()>(tick), timer_duration);
    ticker.start();
    cout << "Running Simulation...\n";
    for (int i = 0; i<net._nb_samples; i++) {
        power_model.random_generator(net._uncert_perc);
        power_model.post_COPPER_static();
        double no_pv = power_model._optimal;
        bill[i] = no_pv/(12*net._nb_years);
        power_model._type = COPPER_PV_PEAK;
        power_model.post_COPPER_static(true,false); /*< ONLY PV */
        diff_PV[i] = (no_pv - power_model._optimal)/net._nb_years;
        min_diff_PV = min(min_diff_PV, diff_PV[i]);
        max_diff_PV = max(max_diff_PV, diff_PV[i]);
        tot_PV += diff_PV[i];
        power_model.post_COPPER_static(false,true); /*< ONLY Batt */
        diff_Batt[i] = (no_pv - power_model._optimal)/net._nb_years;
        min_diff_Batt = min(min_diff_Batt, diff_Batt[i]);
        max_diff_Batt = max(max_diff_Batt, diff_Batt[i]);
        tot_Batt += diff_Batt[i];
        power_model.post_COPPER_static(true,true); /*< Both PV and Batt */
        diff_PV_Batt[i] = (no_pv - power_model._optimal)/net._nb_years;
        min_diff_PV_Batt = min(min_diff_PV_Batt, diff_PV_Batt[i]);
        max_diff_PV_Batt = max(max_diff_PV_Batt, diff_PV_Batt[i]);
        tot_PV_Batt += diff_PV_Batt[i];
        power_model._random_load.clear();
        power_model._random_weather.clear();
        power_model._random_load_uncert.clear();
        power_model._random_PV_uncert.clear();
    }
    ticker.stop();
    printf("\nMonthly Bill Average For Each Sample: \n");
    for (int i = 0; i<net._nb_samples; i++) {
        cout << "$";
        printfcomma(bill[i]);
        cout << endl;
    }
    printf("\n ---------------- \n");
    printf("\n PV ONLY SCENARIO \n");
    printf("\n ---------------- \n");
    printf("\nAnnual Savings For Each Sample: \n");
    for (int i = 0; i<net._nb_samples; i++) {
        cout << "$";
        printfcomma(diff_PV[i]);
        cout << endl;
    }
    printf("\nMinimum Annual Savings = $");
    printfcomma(min_diff_PV);
    cout << endl;
    printf("Maximum Annual Savings = $");
    printfcomma(max_diff_PV);
    cout << endl;
    double mean = tot_PV/net._nb_samples;
    printf("Average Annual Savings = $");
    printfcomma(mean);
    cout << endl;
    double aver_dev = 0;
    for (int i = 0; i<net._nb_samples; i++) {
        aver_dev += abs(diff_PV[i] - mean);
    }
    aver_dev /= net._nb_samples;
    printf("Average Deviation = $%d\n", (int)aver_dev);
    printf("Total Average Savings Over %d years if we consider only PV = $", net._nb_years);
    printfcomma(mean*net._nb_years);
    cout << endl;
    cout << "Given annual demand growth of " << 100*(net._demand_growth-1) << "% ";
    cout << "and annual inflation of " << 100*(net._price_inflation-1) << "%" << endl;
    
    printf("\n --------------------- \n");
    printf("\n BATTERY ONLY SCENARIO \n");
    printf("\n --------------------- \n");
    printf("\nAnnual Savings For Each Sample: \n");
    for (int i = 0; i<net._nb_samples; i++) {
        cout << "$";
        printfcomma(diff_Batt[i]);
        cout << endl;
    }
    printf("\nMinimum Annual Savings = $");
    printfcomma(min_diff_Batt);
    cout << endl;
    printf("Maximum Annual Savings = $");
    printfcomma(max_diff_Batt);
    cout << endl;
    mean = tot_Batt/net._nb_samples;
    printf("Average Annual Savings = $");
    printfcomma(mean);
    cout << endl;
    aver_dev = 0;
    for (int i = 0; i<net._nb_samples; i++) {
        aver_dev += abs(diff_Batt[i] - mean);
    }
    aver_dev /= net._nb_samples;
    printf("Average Deviation = $%d\n", (int)aver_dev);
    printf("Total Average Savings Over %d years if we consider only Batteries  = $", net._nb_years);
    printfcomma(mean*net._nb_years);
    cout << endl;
    cout << "Given annual demand growth of " << 100*(net._demand_growth-1) << "% ";
    cout << "and annual inflation of " << 100*(net._price_inflation-1) << "%" << endl;
    
    printf("\n --------------------- \n");
    printf("\n PV + BATTERY SCENARIO \n");
    printf("\n --------------------- \n");
    printf("\nAnnual Savings For Each Sample: \n");
    for (int i = 0; i<net._nb_samples; i++) {
        cout << "$";
        printfcomma(diff_PV_Batt[i]);
        cout << endl;
    }
    printf("\nMinimum Annual Savings = $");
    printfcomma(min_diff_PV_Batt);
    cout << endl;
    printf("Maximum Annual Savings = $");
    printfcomma(max_diff_PV_Batt);
    cout << endl;
    mean = tot_PV_Batt/net._nb_samples;
    printf("Average Annual Savings = $");
    printfcomma(mean);
    cout << endl;
    aver_dev = 0;
    for (int i = 0; i<net._nb_samples; i++) {
        aver_dev += abs(diff_PV_Batt[i] - mean);
    }
    aver_dev /= net._nb_samples;
    printf("Average Deviation = $%d\n", (int)aver_dev);
    printf("Total Average Savings Over %d years if we consider PV + Batteries = $", net._nb_years);
    printfcomma(mean*net._nb_years);
    cout << endl;
    cout << "Given annual demand growth of " << 100*(net._demand_growth-1) << "% ";
    cout << "and annual inflation of " << 100*(net._price_inflation-1) << "%" << endl;
    cout << "Enter any key to exit\n";
    cin.get();
    return 0;
}
