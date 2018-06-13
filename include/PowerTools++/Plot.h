//
// Created by angela on 12/10/16.
//

#ifndef POWERTOOLS_PLOT_H
#define POWERTOOLS_PLOT_H
//#include "plc++demos.h"
//#include "plplot/plstream.h"
#include "PowerTools++/Net.h"

#ifdef PL_USE_NAMESPACE
using namespace std;
#endif

class plot {
public:
    //plot( int, const char ** , PowerModel& power_model);
    //plot_PV( int, const char ** , PowerModel& power_model);

    void plot_V( int, const char ** , PowerModel& power_model);
    //void plot_V_direct( int, const char **);
    void plot_PV( int, const char ** , PowerModel& power_model);
    void plot_flow( int, const char ** , PowerModel& power_model);
    void plot_soc( int argc, const char **argv , PowerModel& power_model);
//    void plfbox( PLFLT, double, int);

//    vector<plstream*>         pls;
    //plstream                    *pls;
private:
    // Class data



//    static PLFLT       pos[], red[], green[], blue[];
};


#endif //POWERTOOLS_PLOT_H
