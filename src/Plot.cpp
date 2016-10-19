//
// Created by angela on 12/10/16.
//

#include "PowerTools++/Plot.h"
#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Bus.h"

void plot::plfbox( PLFLT x0, double y0, int i)
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


    pls[i]->fill( 4, x, y );
    pls[i]->col0( 1 );
    pls[i]->lsty( 1 );
    pls[i]->line( 4, x, y );


    delete[] x;
    delete[] y;
}



PLFLT       plot::pos[] = { 0.0, 0.25, 0.5, 0.75, 1.0 };
PLFLT       plot::red[] = { 1.0, 1.0, 1.0, 1.0, 1.0};
PLFLT       plot::green[] = {  0.0, 0.25, 0.5, 0.75, 1.0 };
PLFLT       plot::blue[] = { 0.0, 0.0, 0.0, 0.0, 0.0  };
//double      test[] = {0., 0., 10., 20., 50., 70., 85., 75., 50., 30., 0., 0.};

void plot::plot_PV( int argc, const char **argv , PowerModel& power_model) {

    //int NSIZE = 20*power_model._timesteps;
    //PLFLT x[NSIZE], y[NSIZE];
    //PLFLT xmin = 0., xmax = 20*power_model._timesteps, ymin = 0., ymax = 100;

    // Prepare data to be plotted.
    // plot pv_rate against time
/*    int i = 0;

    for (auto n:power_model._net->nodes) {
*//*        x[i] = (PLFLT) ( i ) / (PLFLT) ( NSIZE - 1 ); In the example given
    y[i] = ymax * x[i] * x[i];*//*
        if (n->in()) {
            for (int t = 0; t < power_model._timesteps; t++) {
                x[i] = i;
                y[i] = power_model._net->bMVA*1000*n->pv_t[t].get_value();
                cout << "p = (" <<  x[i] << ", " <<  y[i] << ")\n";
                i++;
            }
        }

    }*/

    //pls = new plstream();

    // Parse and process command line arguments
    //pls->parseopts( &argc, argv, PL_PARSE_FULL );

    //pls->spal0( "cmap0_black_on_white.pal" );
    //pls->spal1( "cmap1_gray.pal", true );
    // Initialize plplot
    //pls->init();

    // Create a labelled box to hold the plot.
    //pls->env( xmin, xmax, ymin, ymax, 0, 0 );
    //pls->lab( "time", "pv_t for each timestep", "pv_t versus time" );

    // Plot the data that was prepared above.
    //pls->line(NSIZE, x, y );
    //pls->line( NSIZE, x, y ); found in the original code


    // In C++ we don't call plend() to close PLplot library
    // this is handled by the destructor
    //delete pls;
    double y0[power_model._timesteps];
    int i = 1;
    int b = 1;
    char string[20];

    // Parse and process command line arguments.
    pls.reserve(20);
    for (auto n:power_model._net->nodes) {
        if (n->in()) {
            pls[i-1] = new plstream();
#ifdef __APPLE__
            //                pls->sfnam("out.pdf");
//        pls->sdev("pdf");
#elif __linux__
            std::string name("out"+to_string(i)+".psc");
            pls[i-1]->sfnam(name.c_str());
            pls[i-1]->sdev("psc");
#endif
            pls[i-1]->spal0("cmap0_black_on_white.pal");
            pls[i-1]->spal1("cmap1_gray.pal", true);

            // Initialize plplot.

            pls[i-1]->init();


            pls[i-1]->adv(0);
            pls[i-1]->vsta();
            pls[i-1]->wind(1, power_model._timesteps + 1, 0.0, 150.0);
            pls[i-1]->box("bc", 1.0, 0, "bcnv", power_model._timesteps + 1, 0);
            pls[i-1]->col0(2);
            pls[i-1]->lab("#frTime Step", "#frkWh", "#frPV Generation");

            pls[i-1]->scmap1l(true, 5, pos, red, green, blue, NULL);
            n->print();
            cout << "Panel size = " << n->pv_rate.get_value() << endl;
            for (int t = 0; t < power_model._timesteps; t++) {
                y0[t] = power_model._net->bMVA * 1000 * n->pv_t[t].get_value();
                //y0[t] = power_model._net->bMVA * 1000 * n->w_t[t].get_value();
//                y0[i] = test[i-1];
                cout << y0[t] << ", ";
                pls[i - 1]->col1((y0[t]) / 150.);
                pls[i - 1]->psty(0);
                plfbox(t, y0[t], i-1);
                if (y0[t] >= 0.1) {
                    if (y0[t] >= 1) {
                        sprintf(string, "%.00f", y0[t]);
                    }
                    else {
                        sprintf(string, "%.01f", y0[t]);
                    }
                    pls[i - 1]->ptex((t + .5), (y0[t] + 5.), 1.0, 0.0, .5, string);
                }

                sprintf(string, "%d", t);
                pls[i-1]->mtex("b", 1.0, (t / (1. * power_model._timesteps) - .5 / power_model._timesteps), 0.5,
                               string);
            }
            delete pls[i-1];
            i++;
        }


    }

#ifdef _WIN32
#elif __APPLE__
    system("open out.pdf");
#elif __linux__

    i = 1;
    for (auto n:power_model._net->nodes) {
        if (n->in()) {
            std::string name("gv out" + to_string(i) + ".psc");
            system(name.c_str());
            i++;
        };
    };

#else
#   error "Unknown compiler"
#endif
//




}

void plot::plot_V( int argc, const char **argv , PowerModel& power_model) {

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
    int i = 1;
    int b = 1;
    char string[20];

    // Parse and process command line arguments.
    pls.reserve(20);
    for (auto n:power_model._net->nodes) {
        if (n->in()) {
            pls[i-1] = new plstream();
#ifdef __APPLE__
            //                pls->sfnam("out.pdf");
//        pls->sdev("pdf");
#elif __linux__

            std::string name("out"+to_string(i)+".psc");
            pls[i-1]->sfnam(name.c_str());
            pls[i-1]->sdev("psc");
#endif
            pls[i-1]->spal0("cmap0_black_on_white.pal");
            pls[i-1]->spal1("cmap1_gray.pal", true);

            // Initialize plplot.

            pls[i-1]->init();


            pls[i-1]->adv(0);
            pls[i-1]->vsta();
            pls[i-1]->wind(1, power_model._timesteps + 1, 0.0, 150.0);
            pls[i-1]->box("bc", 1.0, 0, "bcnv", power_model._timesteps + 1, 0);
            pls[i-1]->col0(2);
            pls[i-1]->lab("#frTime Step", "#frkV", "#fr voltage square magnitude");

            pls[i-1]->scmap1l(true, 5, pos, red, green, blue, NULL);
            n->print();
            cout << "Panel size = " << n->pv_rate.get_value() << endl;
            for (int t = 0; t < power_model._timesteps; t++) {


                y0[t] = n->_kvb * 1000. * (pow(n->vr_t[t].get_value(),2) + pow(n->vi_t[t].get_value(),2));
                //y0[t] = power_model._net->bMVA * 1000 * n->w_t[t].get_value();
//                y0[i] = test[i-1];
                cout << y0[t] << ", ";
                pls[i - 1]->col1((y0[t]) / 150.);
                pls[i - 1]->psty(0);
                plfbox(t, y0[t], i-1);
                if (y0[t] >= 0.1) {
                    if (y0[t] >= 1) {
                        sprintf(string, "%.00f", y0[t]);
                    }
                    else {
                        sprintf(string, "%.01f", y0[t]);
                    }
                    pls[i - 1]->ptex((t + .5), (y0[t] + 5.), 1.0, 0.0, .5, string);
                }

                sprintf(string, "%d", t);
                pls[i-1]->mtex("b", 1.0, (t / (1. * power_model._timesteps) - .5 / power_model._timesteps), 0.5,
                               string);
            }
            delete pls[i-1];
            i++;
        }


    }
#ifdef _WIN32
#elif __APPLE__
    system("open out.pdf");
#elif __linux__
    i = 1;
    for (auto n:power_model._net->nodes) {
        if (n->in()) {
            std::string name("gv out" + to_string(i) + ".psc");
            system(name.c_str());
            i++;
        };
    };

#else
#   error "Unknown compiler"
#endif
//


}

void plot::plot_flow( int argc, const char **argv , PowerModel& power_model) {

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
    int i = 1;
    int b = 1;
    char string[400];

    // Parse and process command line arguments.
    pls.reserve(400);
    int pmax = 0;
    for (auto a:power_model._net->arcs) {
        if(a->status==false){
            continue;
        }
        pmax=0;

        pls[i - 1] = new plstream();
#ifdef __APPLE__
        //                pls->sfnam("out.pdf");
//        pls->sdev("pdf");
#elif __linux__
        std::string name("out" + to_string(i) + ".psc");
        pls[i - 1]->sfnam(name.c_str());
        pls[i - 1]->sdev("psc");
#endif
        pls[i - 1]->spal0("cmap0_black_on_white.pal");
        pls[i - 1]->spal1("cmap1_gray.pal", true);

        // Initialize plplot.

        pls[i - 1]->init();


        pls[i - 1]->adv(0);
        pls[i - 1]->vsta();



        for (int t = 0; t < power_model._timesteps; t++) {
            y0[t] = (a->pi_t[t].get_value()) * 1000. * power_model._net->bMVA;
            //y0[t] = power_model._net->bMVA * 1000 * n->w_t[t].get_value();
//                y0[i] = test[i-1];
            cout << y0[t] << ", ";
            if(pmax < fabs(y0[t])){
                pmax = fabs(y0[t]);
            }
        }
        pls[i - 1]->wind(1, power_model._timesteps + 1, 0.0, pmax*1.1 + 10);
        pls[i - 1]->box("bc", 1.0, 0, "bcnv", power_model._timesteps + 1, 0);
        pls[i - 1]->col0(2);
        pls[i - 1]->lab("#frTime Step", "#frkWh", "#fr power flow");

        pls[i - 1]->scmap1l(true, 5, pos, red, green, blue, NULL);
        a->print();
        for (int t = 0; t < power_model._timesteps; t++) {

            if (y0[t] < 0) {
                pls[i - 1]->col1(0);
                y0[t] *= -1;
            }
            else {
                pls[i - 1]->col1(1);
            }

            pls[i - 1]->psty(0);
            plfbox(t+1, y0[t], i-1 );
            if (y0[t] >= 0.1) {
                if (y0[t] >= 1) {
                    sprintf(string, "%.00f", y0[t]);
                }
                else {
                    sprintf(string, "%.01f", y0[t]);
                }
                pls[i - 1]->ptex((t + 1.5), (y0[t]+ 1.+ pmax*0.05), 1.0, 0.0, 0.5, string);
            }

            sprintf(string, "%d", t);
            pls[i - 1]->mtex("b", 1.0, ((t+1) / (1. * power_model._timesteps) - .5 / power_model._timesteps), 0.5,
                             string);
        }

        delete pls[i - 1];
        i++;
    }
#ifdef _WIN32
#elif __APPLE__
    system("open out.pdf");
#elif __linux__
    i = 1;
    for (auto a:power_model._net->arcs) {
        if (a->status=1) {
            std::string name("gv out" + to_string(i) + ".psc");
            system(name.c_str());
            i++;
        };
    };

#else
#   error "Unknown compiler"
#endif
//

}




