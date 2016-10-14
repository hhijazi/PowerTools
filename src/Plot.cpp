//
// Created by angela on 12/10/16.
//

#include "PowerTools++/Plot.h"
#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Bus.h"

void plot::plfbox( PLFLT x0, double y0 )
{
    PLFLT *x = new PLFLT[4];
    double *y = new double[4];
    
    x[0] = x0;
    y[0] = 0.;
    x[1] = x0;
    y[1] = y0;
    x[2] = x0 + 1.;
    y[2] = y0;
    x[3] = x0 + 1.;
    y[3] = 0.;
    pls->fill( 4, x, y );
    pls->col0( 1 );
    pls->lsty( 1 );
    pls->line( 4, x, y );
    
    delete[] x;
    delete[] y;
}



PLFLT       plot::pos[] = { 0.0, 0.25, 0.5, 0.75, 1.0 };
PLFLT       plot::red[] = { 1.0, 1.0, 1.0, 1.0, 1.0};
PLFLT       plot::green[] = {  0.0, 0.25, 0.5, 0.75, 1.0 };
PLFLT       plot::blue[] = { 0.0, 0.0, 0.0, 0.0, 0.0  };
//double      test[] = {0., 0., 10., 20., 50., 70., 85., 75., 50., 30., 0., 0.};

plot::plot( int argc, const char **argv , PowerModel& power_model)
{
//    int NSIZE = 20*power_model._timesteps;
//    PLFLT x[NSIZE], y[NSIZE];
//    PLFLT xmin = 0., xmax = 20*power_model._timesteps, ymin = 0., ymax = 100;
//
//    // Prepare data to be plotted.
//    // plot pv_rate against time
//    int i = 0;
//    
//    for (auto n:power_model._net->nodes) {
///*        x[i] = (PLFLT) ( i ) / (PLFLT) ( NSIZE - 1 ); In the example given
//    y[i] = ymax * x[i] * x[i];*/
//        if (n->in()) {
//            for (int t = 0; t < power_model._timesteps; t++) {
//                x[i] = i;
//                y[i] = power_model._net->bMVA*1000*n->pv_t[t].get_value();
//                cout << "p = (" <<  x[i] << ", " <<  y[i] << ")\n";
//                i++;
//            }
//        }
//
//    }
//
//    pls = new plstream();
//
//    // Parse and process command line arguments
//    pls->parseopts( &argc, argv, PL_PARSE_FULL );
//
//    pls->spal0( "cmap0_black_on_white.pal" );
//    pls->spal1( "cmap1_gray.pal", true );
//    // Initialize plplot
//    pls->init();
//
//    // Create a labelled box to hold the plot.
//    pls->env( xmin, xmax, ymin, ymax, 0, 0 );
//    pls->lab( "time", "pv_t for each timestep", "pv_t versus time" );
//
//    // Plot the data that was prepared above.
//    pls->line(NSIZE, x, y );
//    //pls->line( NSIZE, x, y ); found in the original code
//
//
//    // In C++ we don't call plend() to close PLplot library
//    // this is handled by the destructor
//    delete pls;
    double y0[power_model._timesteps];
    int  i = 1;
    char string[20];
    
    pls = new plstream();
    
    // Parse and process command line arguments.
#ifdef __APPLE__
    pls->sfnam("out.pdf");
    pls->sdev("pdf");
#elif __linux__
    pls->sfnam("out.psc");
    pls->sdev("psc");
#endif
    pls->spal0( "cmap0_black_on_white.pal" );
    pls->spal1( "cmap1_gray.pal", true );

    // Initialize plplot.
    
    pls->init();
    
    
    
    pls->adv( 0 );
    pls->vsta();
    pls->wind( 1, power_model._timesteps+1, 0.0, 150.0 );
    pls->box( "bc", 1.0, 0, "bcnv", power_model._timesteps+1, 0 );
    pls->col0( 2 );
    pls->lab( "#frTime Step", "#frkWh", "#frPV Generation" );
    
    pls->scmap1l( true, 5, pos, red, green, blue, NULL );
    for (auto n:power_model._net->nodes) {
        if (n->in()) {
            n->print();
            cout << "Panel size = " << n->pv_rate.get_value() << endl;
            for (int t = 0; t < power_model._timesteps; t++) {
                y0[i] = power_model._net->bMVA*1000*n->pv_t[t].get_value();
//                y0[i] = test[i-1];
                cout << y0[i] << ", ";
                pls->col1( (y0[i]) / 150. );
                pls->psty( 0 );
                plfbox( ( i ), y0[i] );
                if(y0[i] >= 0.1) {
                    if(y0[i] >= 1) {
                        sprintf( string, "%.00f", y0[i] );
                    }
                    else {
                        sprintf( string, "%.01f", y0[i] );
                    }
                    pls->ptex( ( i + .5 ), ( y0[i] + 5. ), 1.0, 0.0, .5, string );
                }
                
                sprintf( string, "%d", i );
                pls->mtex( "b", 1.0, ( i/(1.*power_model._timesteps)  - .5/power_model._timesteps ), 0.5, string );
                i++;
            }
            //break;
        }

    }
    

    delete pls;
}


