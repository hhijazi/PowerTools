//
// Created by angela on 12/10/16.
//

#ifndef POWERTOOLS_PLOT_H
#define POWERTOOLS_PLOT_H
//#include "plc++demos.h"
#include "plplot/plstream.h"
#include "PowerTools++/Net.h"

#ifdef PL_USE_NAMESPACE
using namespace std;
#endif

class plot {
public:
    plot( int, const char ** , PowerModel& power_model);
    void plfbox( PLFLT, double );
private:
    // Class data
    plstream         *pls;
    
    static PLFLT       pos[], red[], green[], blue[];
};


#endif //POWERTOOLS_PLOT_H
