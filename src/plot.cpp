//
// Created by angela on 12/10/16.
//

#include "plot.h"
#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Bus.h"





plot::plot( int argc, const char **argv , PowerModel& power_model)
{
    int NSIZE = 20*power_model._timesteps;
    PLFLT x[NSIZE], y[NSIZE];
    PLFLT xmin = 0., xmax = 20*power_model._timesteps, ymin = 0., ymax = 100;

    // Prepare data to be plotted.
    // plot pv_rate against time
    int i = 0;
    for (int t = 0; t < power_model._timesteps; t++) {
        for (auto n:power_model._net->nodes) {
/*        x[i] = (PLFLT) ( i ) / (PLFLT) ( NSIZE - 1 ); In the example given
        y[i] = ymax * x[i] * x[i];*/
            if (n->in()) {
                x[i] = i++;
                y[i] = power_model._net->bMVA*1000*n->pv_t[t].get_value();
                cout << "p = (" <<  x[i] << ", " <<  y[i] << ")\n";
            }

        }
    }

    pls = new plstream();

    // Parse and process command line arguments
    pls->parseopts( &argc, argv, PL_PARSE_FULL );

    // Initialize plplot
    pls->init();

    // Create a labelled box to hold the plot.
    pls->env( xmin, xmax, ymin, ymax, 0, 0 );
    pls->lab( "time", "pv_t for each timestep", "pv_t versus time" );

    // Plot the data that was prepared above.
    pls->line(NSIZE, x, y );
    //pls->line( NSIZE, x, y ); found in the original code


    // In C++ we don't call plend() to close PLplot library
    // this is handled by the destructor
    delete pls;
}