//
//  PowerModel.cpp
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights resized.
//

#include "PowerTools++/PowerModel.h"
#include "PowerTools++/Bus.h"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <stdlib.h>
#include <iomanip>


PowerModel::PowerModel():PowerModel(ACPOL, new Net(), ipopt){};

PowerModel::PowerModel(PowerModelType type, Net* net, SolverType stype):_type(type), _net(net), _stype(stype){
    _model = NULL;
    time ( &_rawtime );
    _start_date = localtime ( &_rawtime );
    _start_date->tm_year = 2018 - 1900;
    _start_date->tm_mon = 0;
    _start_date->tm_mday = 1;
    _start_date->tm_hour = 1;
    _start_date->tm_min = 0;
    _start_date->tm_sec = 0;
//    _timeinfo->tm_isdst = 0;
    mktime ( _start_date );
    _eng = mt19937(_rd()); // seed the generator
    _model = new Model();
    _solver = new PTSolver(_model, _stype);    
};



PowerModel::~PowerModel(){
    delete _model;
    delete _solver;
    delete _demand_start_date;
    delete _irrad_start_date;
    //    delete _net;
};


void PowerModel::build(int time_steps){
    double max_irr = *max_element(begin(_net->_radiation), end(_net->_radiation));
    cout << "Max irradiance = " + to_string(max_irr) << endl;
    for (int i = 0; i < _net->_radiation.size(); i++) {
        _net->_radiation[i] /= max_irr;
        assert(_net->_radiation[i]<=1);
    }
    //setenv("TZ", "/usr/share/zoneinfo/America/New_York", 1); // POSIX-specific
    
//    cout << "Nb days in current month = " << get_nb_days_in_month() << endl;
    
    _timesteps = time_steps;
    switch (_type) {

            
        case COMPACT_FIXED_PV:  //no pv no battery
            add_AC_Rect_vars_Time_fixed_compact();
            post_AC_PF_Time_Fixed_Compact();
            min_cost_pv_fixed();
            break;
            
        case FIXED_PV:  //no pv no battery
            add_AC_Rect_vars_Time_fixed();
            post_AC_PF_Time_Fixed();
            min_cost_pv_fixed();
            break;
            
        case ACPF_T:  //no pv no battery
            add_AC_Rect_vars_Time();
            post_AC_PF_Time();
            min_cost_time();
            break;
            
        case NF:  //no pv no battery
            add_NF_vars();
            post_NF();
            min_cost_time();
            break;
            
        case NF_PEAK:  //no pv no battery
            add_NF_Peak_vars(false);
            post_NF_Peak(false);
            min_cost_peak();
            break;
            
        case NF_PV_PEAK:  //no pv no battery, modelling peak demand cost
            add_NF_Peak_vars(true);
            post_NF_Peak(true);
            min_cost_peak();
            break;
        case ACPF_PV_PEAK:  //no pv no battery, modelling peak demand cost
            add_AC_Rect_PV_vars_Peak();
            post_AC_PF_PV_Peak();
            min_cost_peak();
            break;
            
        case COPPER_PV_PEAK:  //no pv no battery, modelling peak demand cost
            add_COPPER_vars(true);
            post_COPPER_static(true);
            break;
            
        case COPPER_PEAK:  //no pv no battery, modelling peak demand cost
            add_COPPER_vars();
            post_COPPER_static();
            break;

            
        case ACPF_PEAK:  //no pv no battery, modelling peak demand cost
            add_AC_Rect_vars_Peak();
            post_AC_PF_Peak();
            min_cost_peak();
            break;

        case ACPF_PV_T: //pv only
            add_AC_Rect_PV_vars_Time();
            post_AC_PF_PV_Time();
            min_cost_pv();
            break;

        case ACPF_BATT_T_NO_GEN:
            add_AC_Rect_PV_Batt_vars_Time();
            post_AC_PF_Batt_Time_No_Gen();
            min_cost_pv_batt();
            break;

        case ACPF_PV_BATT_T: //pv and battery
            add_AC_Rect_PV_Batt_vars_Time();
            post_AC_PF_PV_Batt_Time();
            min_cost_pv_batt();
            break;

        case ACPF_BATT_T:
            add_AC_Rect_Batt_vars_Time();
            post_AC_PF_Batt_Time();
            min_cost_batt();
            break;

        case SOCP_T:
            add_AC_SOCP_vars_Time();
            post_AC_SOCP_Time();
            min_cost_time();
            break;

        case SOCP_PV_T:
            add_SOCP_Rect_PV_vars_Time();
            post_SOCP_PF_PV_Time();
            min_cost_pv();
            break;

        case SOCP_BATT_T_NO_GEN:
            add_SOCP_Rect_PV_Batt_vars_Time();
            post_SOCP_PF_Batt_Time_No_Gen();
            min_cost_pv_batt();
            break;

        case SOCP_BATT_T:
            add_SOCP_Rect_Batt_vars_Time();
            post_SOCP_PF_Batt_Time();
            min_cost_batt();
            break;

        case SOCP_PV_BATT_T:
            add_SOCP_Rect_PV_Batt_vars_Time();
            post_SOCP_PF_PV_Batt_Time();
            min_cost_pv_batt();
            break;

        case ACPF_PV:
            add_AC_Rect_PV_vars();
            post_AC_PF_PV();
            min_cost();
            break;
        case ACPF:
            add_AC_Rect_vars();
//            post_AC_PF();
            min_cost();
            break;
        case ACPOL:
            add_AC_Pol_vars();
            post_AC_Polar();
            min_cost();
            break;
        case ACRECT:
            add_AC_Rect_vars();
            post_AC_Rect();
            min_cost();
            break;
        case QC:
            add_QC_vars();
            post_QC();
            min_cost();
            break;
        case QC_SDP:
            add_QC_vars();
            post_QC();
            add_SDP_cuts(3);
            min_cost();
            break;
        case OTS:
            add_AC_OTS_vars();
            post_AC_OTS();
            min_cost();
            break;
        case SOCP:
            add_AC_SOCP_vars();
            post_AC_SOCP();
            min_cost();
            break;
        case SDP:
            add_AC_SOCP_vars();
            post_AC_SOCP();
            add_SDP_cuts(3);
            min_cost();
            break;
        case DC:
            add_DC_vars();
            post_DC();
            min_cost();
            break;
        case QC_OTS_L:
            add_QC_OTS_vars();
            post_QC_OTS(true,false);
            min_cost();
            break;
        case QC_OTS_N:
            add_QC_OTS_vars();
            post_QC_OTS(true,true);
            min_cost();
            break;
        case QC_OTS_O:
            add_QC_OTS_vars();
            post_QC_OTS(false,true);
            min_cost();
            break;
        case SOCP_OTS:
            add_SOCP_OTS_vars();
            post_SOCP_OTS();
            min_cost();
            break;
        case GRB_TEST:
            //run_grb_lin_test();
            run_grb_quad_test();
            break;
        default:
            break;
    }
}

void PowerModel::post_AC_PF_Batt_Time(){

    double tot_pl = 0;
/*
    for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t]==0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/



    for (auto n:_net->nodes) {
        add_AC_KCL_Batt_Time(n);
        if (n->candidate()) {
            add_AC_link_Batt_Time(n);
        }
        else if (n->inst()){
            add_link_PV_fixed_Rate(n);       //with curtailment
        }
        add_AC_Voltage_Bounds_Time(n);




        tot_pl += n->pl();
    }
    cout << "Total pl = " << tot_pl << endl;


    for (auto a:_net->arcs) {
        add_AC_Power_Flow_Time(a);     //done
        add_AC_thermal_Time(a);        //done
        add_AC_Angle_Bounds_Time(a);
    }

}

void PowerModel::post_AC_PF_PV_Batt_Time(){
    
    double tot_pl = 0;  // total load power is 0?????
/*
    for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t]==0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {
                
                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/


    
    for (auto n:_net->nodes) {  //for each node, add constraits
        add_AC_KCL_PV_Batt_Time(n);  //KCL constraints for PV and batt, time dependent
        if (n->candidate()) {  //if this node is candidate
            add_AC_link_Batt_Time(n);  //battery constraints??????
            add_link_PV_Rate(n);       //with curtailment, PV constraints, capacity is a variable
//            add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
        }
        else if (n->inst()){  //if PV is installed in the node
            add_link_PV_fixed_Rate(n);       //with curtailment, PV constraints, fixed capacity
        }
        add_AC_Voltage_Bounds_Time(n);  //voltage constraints
        
        

        


     

        tot_pl += n->pl(); //Returns the active power load at this bus
    }
    cout << "Total pl = " << tot_pl << endl;  // printout the active power load at this bus





    for (auto a:_net->arcs) {  //for each line, add constraits
        add_AC_Power_Flow_Time(a);     //done, power flow constraints
        add_AC_thermal_Time(a);        //done, line thermal limits
        add_AC_Angle_Bounds_Time(a);  //phase angle differences constraints

    }



}

void PowerModel::post_SOCP_PF_Batt_Time_No_Gen(){
    Node *src = NULL;
    Node *dest = NULL;

    double tot_pl = 0;

/*    for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t] == 0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/


    for (int t = 0; t < _timesteps; t++) {

        Constraint real_gen_zero("real_gen_to_zero" + to_string(t));
        for (auto g: _net->gens) {

            real_gen_zero+= g->pg_t[t];
            real_gen_zero = 0;
        }
        _model->addConstraint(real_gen_zero);
    }

    for (int t = 0; t < _timesteps; t++) {

        Constraint im_gen_zero("im_gen_to_zero" + to_string(t));
        for (auto g: _net->gens) {

            im_gen_zero+= g->qg_t[t];
            im_gen_zero = 0;
        }
        _model->addConstraint(im_gen_zero);
    }


    for (auto n:_net->nodes) {
        add_SOCP_KCL_Batt_Time(n);         //sdone

        add_AC_link_Batt_Time(n);      //no curtailment

//        add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
        add_link_PV_Rate(n);       //with curtailment

   /*     Constraint Sum_PV_bound("Sum_PV_bound" + n->_name);
        Sum_PV_bound += n->pv_rate;
        Sum_PV_bound <= (3.*1000.) * (_net->bMVA);
        _model->addConstraint(Sum_PV_bound);*/



        tot_pl += n->pl();
    }
    cout << "Total pl = " << tot_pl << endl;


    for (auto a:_net->arcs) {

        Arc *ap = nullptr;
        if (a->status == 1) {
            for (int t = 0; t < _timesteps; t++) {


                src = a->src;
                dest = a->dest;
                /** subject to Flow_P_From {(l,i,j) in arcs_from}:
                 p[l,i,j] = g[l]*(v[i]/tr[l])^2
                 + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_P_From("Flow_P_From" + a->pi._name);
                Flow_P_From += a->pi_t[t];
                //        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
                //            cout << "ok";
                //            cout << endl;
                //        }

                Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w_t[t];
                ap = _net->get_arc(a->src, a->dest);
                Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_From = 0;
                _model->addConstraint(Flow_P_From);

                /** subject to Flow_P_To {(l,i,j) in arcs_to}:
                 p[l,i,j] = g[l]*v[i]^2
                 + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_P_To("Flow_P_To" + a->pj._name);
                Flow_P_To += a->pj_t[t];
                Flow_P_To -= a->g * dest->w_t[t];
                Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_To = 0;
                _model->addConstraint(Flow_P_To);

                /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
                 q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
                 +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_Q_From("Flow_Q_From" + a->qi._name);
                Flow_Q_From += a->qi_t[t];
                Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w_t[t];
                Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_From = 0;
                _model->addConstraint(Flow_Q_From);

                /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
                 q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
                 +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
                Flow_Q_To += a->qj_t[t];
                Flow_Q_To += (a->ch / 2 + a->b) * dest->w_t[t];
                Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_To = 0;
                _model->addConstraint(Flow_Q_To);

                /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
                 */
                Constraint SOCP("SOCP");
                SOCP += a->src->w_t[t] * a->dest->w_t[t];
                SOCP -= ((ap->wr_t[t]) ^ 2);
                SOCP -= ((ap->wi_t[t]) ^ 2);
                SOCP >= 0;
                _model->addConstraint(SOCP);


            }
            add_AC_thermal_Time(a);        //done


        }

    }
}

void PowerModel::post_AC_PF_Batt_Time_No_Gen(){

    double tot_pl = 0;

    /*for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t]==0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/

    /*for (int t = 0; t < _timesteps; t++) {

            Constraint real_gen_zero("set_gen_to_zero" + to_string(t));
            for (auto g: _net->gens) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                real_gen_zero+= g->pg_t[t];
                real_gen_zero = 0;
            }
            _model->addConstraint(real_gen_zero);
        }*/

    /*for (int t = 0; t < _timesteps; t++) {

        Constraint im_gen_zero("set_gen_to_zero" + to_string(t));
        for (auto g: _net->gens) {

            //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
            im_gen_zero+= g->qg_t[t];
            im_gen_zero = 0;
        }
        _model->addConstraint(im_gen_zero);
    }*/




    for (auto n:_net->nodes) {
        add_AC_KCL_Batt_Time(n);         //sdon
        if(n->_gen.size()==0) {

            add_AC_Voltage_Bounds_Time(n);
        }
        else {
            cout << "number of gens in bus " << n->_name << " = " << n->_gen.size();
            cout << "YEAH"<< endl;
        }
        add_AC_link_Batt_Time(n);      //no curtailment

//        add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
        add_link_PV_Rate(n);       //with curtailment

        /*Constraint Sum_PV_bound("Sum_PV_bound" + n->_name);
        Sum_PV_bound += n->pv_rate;
        Sum_PV_bound <= (3.*1000.)*(_net->bMVA);
        _model->addConstraint(Sum_PV_bound);*/


        tot_pl += n->pl();
    }
    cout << "Total pl = " << tot_pl << endl;





    for (auto a:_net->arcs) {
        add_AC_Power_Flow_Time(a);     //done
        add_AC_thermal_Time(a);        //done


    }



}


void PowerModel::post_AC_PF_Time(){
  /*  double tot_pl =0;*/
    for (auto n:_net->nodes) {
        add_AC_KCL_Time(n);         //sdone
        add_AC_Voltage_Bounds_Time(n);
        if (n->inst()) {
            add_link_PV_fixed_Rate(n);
        }

        /*tot_pl += n->pl();*/
    }
  /*  cout << "Total pl = " << tot_pl << endl;*/

    for (auto a:_net->arcs) {
        add_AC_Power_Flow_Time(a);     //done
//        add_AC_thermal_Time(a);        //done
//        add_AC_Angle_Bounds_Time(a);
    }
    
};

void PowerModel::post_NF(bool PV){
    /*  double tot_pl =0;*/
    for (auto n:_net->nodes) {
        add_AC_KCL_NF(n,PV);         //sdone
        if (n->inst()|| (PV && n->candidate())) {
            add_link_PV_fixed_Rate(n);
        }
        
        /*tot_pl += n->pl();*/
    }
    /*  cout << "Total pl = " << tot_pl << endl;*/
};

void PowerModel::post_AC_PF_Peak() {
    
    post_AC_PF_Time();
    for (int y = 0; y < _net->_nb_years; y++) {
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (int g = 0; g < _net->gens.size(); g++) {
                Constraint kVa_MAX("kVa_MAX_"+to_string(g)+"_"+to_string(t));
                kVa_MAX += _net->max_kVas_year[y];
                if (t<24) {
                    kVa_MAX -= (1/_summer_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                else {
                    kVa_MAX -= (1/_winter_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                kVa_MAX >= 0;
                _model->addConstraint(kVa_MAX);
                kVa_MAX.print();
            }
        }
    }
    
}


void PowerModel::post_AC_PF_PV_Peak() {
    
    post_AC_PF_PV_Time();
    for (int y = 0; y < _net->_nb_years; y++) {
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (int g = 0; g < _net->gens.size(); g++) {
                Constraint kVa_MAX("kVa_MAX_"+to_string(g)+"_"+to_string(t));
                kVa_MAX += _net->max_kVas_year[y];
                if (t<24) {
                    kVa_MAX -= (1/_summer_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                else {
                    kVa_MAX -= (1/_winter_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                kVa_MAX >= 0;
                _model->addConstraint(kVa_MAX);
                kVa_MAX.print();
            }
        }
    }
    
}

void PowerModel::post_NF_Peak(bool PV) {
    
    post_NF(PV);
    for (int y = 0; y < _net->_nb_years; y++) {
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (int g = 0; g < _net->gens.size(); g++) {
                Constraint kVa_MAX("kVa_MAX_"+to_string(g)+"_"+to_string(t));
                kVa_MAX += _net->max_kVas_year[y];
                if (t<24) {
                    kVa_MAX -= (1/_summer_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                else {
                    kVa_MAX -= (1/_winter_power_factor)*pow(_net->_demand_growth, y)*_net->gens[g]->pg_t[t];
                }
                kVa_MAX >= 0;
                _model->addConstraint(kVa_MAX);
                kVa_MAX.print();
            }
        }
    }
    
}



//void PowerModel::post_COPPER_PV_Peak() {
//    
////    post_CO
//    for (int y = 0; y < _nb_years; y++) {
//        for (int t = 0; t < _timesteps*_nb_days; t++) {
//            for (int g = 0; g < _net->gens.size(); g++) {
//                Constraint kVa_MAX("kVa_MAX_"+to_string(g)+"_"+to_string(t));
//                kVa_MAX += _net->max_kVas_year[y];
//                if (t<24) {
//                    kVa_MAX -= (1/_summer_power_factor)*pow(_demand_growth, y)*_net->gens[g]->pg_t[t];
//                }
//                else {
//                    kVa_MAX -= (1/_winter_power_factor)*pow(_demand_growth, y)*_net->gens[g]->pg_t[t];
//                }
//                kVa_MAX >= 0;
//                _model->addConstraint(kVa_MAX);
//                kVa_MAX.print();
//            }
//        }
//    }
//    
//}


void PowerModel::post_AC_PF_PV_Time() {

    double tot_pl = 0;


    for (auto n:_net->nodes) {
        //cout<<"n is "<< n<<" ;";
        add_AC_KCL_PV_Time(n);         //sdone
        add_AC_Voltage_Bounds_Time(n);

//        add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
        if (n->candidate()) {
            add_link_PV_Rate(n);       //with curtailment
        }
        else if(n->inst()){
            add_link_PV_fixed_Rate(n);
        }


/*        Constraint Sum_PV_bound("Sum_PV_bound" + n->_name);
        Sum_PV_bound += n->pv_rate;
        Sum_PV_bound <= (3. * 1000.) * (_net->bMVA);
        _model->addConstraint(Sum_PV_bound);*/


        tot_pl += n->pl();
    }


    cout << "Total pl = " << tot_pl << endl;
    
    for (auto a:_net->arcs) {

        add_AC_Power_Flow_Time(a);     //done
        add_AC_thermal_Time(a);        //done


    }
}

void PowerModel::post_AC_PF_Time_Fixed() {
    
    for (auto n:_net->nodes) {
        add_AC_KCL_PV_Time(n);         //sdone
        add_AC_Voltage_Bounds_Time(n);
         if(n->inst() || n->candidate()){
            add_link_PV_fixed_Rate(n);
        }
    }
    
    for (auto a:_net->arcs) {
        
        add_AC_Power_Flow_Time(a);     //done
        add_AC_thermal_Time(a);        //done
        add_AC_Angle_Bounds_Time(a);
        
    }
}


void PowerModel::post_AC_PF_Time_Fixed_Compact() {
    
    for (auto n:_net->nodes) {
        add_AC_KCL_PV_Time_Compact(n);         //sdone
        add_AC_Voltage_Bounds_Time(n);
    }
    
//    for (auto a:_net->arcs) {
//        add_AC_thermal_Time_Compact(a);        //done
//    }
}





void PowerModel::post_AC_PF_PV(){
    
    for (auto n:_net->nodes) {
        add_AC_KCL_PV(n, false);
    }
    for (auto a:_net->arcs) {
        add_AC_Power_Flow(a, false);
        add_AC_thermal(a, false);
    }
    for (auto g: _net->gens){
        add_fixed_V(g);
        //        add_fixed_Pg(g);
    }
    
}

void PowerModel::post_SOCP_PF_Batt_Time() {

    Node *src = NULL;
    Node *dest = NULL;

    double tot_pl = 0;

    /*  for (int t = 0; t < _timesteps; t++) {
          if (_net->_radiation[t] == 0) {
              Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
              for (auto n: _net->nodes) {

                  //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                  All_zero += n->pv_t[t];
                  All_zero = 0;
                  _model->addConstraint(All_zero);
              }
          }
      }*/



    for (auto n:_net->nodes) {


        add_SOCP_KCL_Batt_Time(n);         //sdone

        if(n->candidate()) {
            add_AC_link_Batt_Time(n);      //no curtailment
        }


        tot_pl += n->pl();
    }
    cout << "Total pl = " << tot_pl << endl;


    for (auto a:_net->arcs) {

        Arc *ap = nullptr;
        if (a->status == 1) {
            for (int t = 0; t < _timesteps; t++) {


                src = a->src;
                dest = a->dest;
                /** subject to Flow_P_From {(l,i,j) in arcs_from}:
                 p[l,i,j] = g[l]*(v[i]/tr[l])^2
                 + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_P_From("Flow_P_From" + a->pi._name);
                Flow_P_From += a->pi_t[t];
                //        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
                //            cout << "ok";
                //            cout << endl;
                //        }

                Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w_t[t];
                ap = _net->get_arc(a->src, a->dest);
                Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_From = 0;
                _model->addConstraint(Flow_P_From);

                /** subject to Flow_P_To {(l,i,j) in arcs_to}:
                 p[l,i,j] = g[l]*v[i]^2
                 + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_P_To("Flow_P_To" + a->pj._name);
                Flow_P_To += a->pj_t[t];
                Flow_P_To -= a->g * dest->w_t[t];
                Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_To = 0;
                _model->addConstraint(Flow_P_To);

                /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
                 q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
                 +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_Q_From("Flow_Q_From" + a->qi._name);
                Flow_Q_From += a->qi_t[t];
                Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w_t[t];
                Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_From = 0;
                _model->addConstraint(Flow_Q_From);

                /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
                 q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
                 +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
                Flow_Q_To += a->qj_t[t];
                Flow_Q_To += (a->ch / 2 + a->b) * dest->w_t[t];
                Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_To = 0;
                _model->addConstraint(Flow_Q_To);

                /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
                 */
                Constraint SOCP("SOCP");
                SOCP += a->src->w_t[t] * a->dest->w_t[t];
                SOCP -= ((ap->wr_t[t]) ^ 2);
                SOCP -= ((ap->wi_t[t]) ^ 2);
                SOCP >= 0;
                _model->addConstraint(SOCP);


            }
            add_AC_thermal_Time(a);        //done


        }
        //add_Wr_Wi_time(a);

    }
}

void PowerModel::post_SOCP_PF_PV_Batt_Time() {

    Node *src = NULL;
    Node *dest = NULL;

    double tot_pl = 0;

  /*  for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t] == 0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/



    for (auto n:_net->nodes) {
        add_SOCP_KCL_PV_Batt_Time(n);         //sdone

        if(n->candidate()) {
            add_AC_link_Batt_Time(n);      //no curtailment

//        add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
            add_link_PV_Rate(n);       //with curtailment
        }
        else if (n->inst()) {
            add_link_PV_fixed_Rate(n);       //with curtailment
        }
/*        Constraint Sum_PV_bound("Sum_PV_bound" + n->_name);
        Sum_PV_bound += n->pv_rate;
        Sum_PV_bound <= (3.*1000.) * (_net->bMVA);
        _model->addConstraint(Sum_PV_bound);*/


        tot_pl += n->pl();
    }
    cout << "Total pl = " << tot_pl << endl;


    for (auto a:_net->arcs) {

        Arc *ap = nullptr;
        if (a->status == 1) {
            for (int t = 0; t < _timesteps; t++) {


                src = a->src;
                dest = a->dest;
                /** subject to Flow_P_From {(l,i,j) in arcs_from}:
                 p[l,i,j] = g[l]*(v[i]/tr[l])^2
                 + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_P_From("Flow_P_From" + a->pi._name);
                Flow_P_From += a->pi_t[t];
                //        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
                //            cout << "ok";
                //            cout << endl;
                //        }

                Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w_t[t];
                ap = _net->get_arc(a->src, a->dest);
                Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_From = 0;
                _model->addConstraint(Flow_P_From);

                /** subject to Flow_P_To {(l,i,j) in arcs_to}:
                 p[l,i,j] = g[l]*v[i]^2
                 + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_P_To("Flow_P_To" + a->pj._name);
                Flow_P_To += a->pj_t[t];
                Flow_P_To -= a->g * dest->w_t[t];
                Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_To = 0;
                _model->addConstraint(Flow_P_To);

                /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
                 q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
                 +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_Q_From("Flow_Q_From" + a->qi._name);
                Flow_Q_From += a->qi_t[t];
                Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w_t[t];
                Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_From = 0;
                _model->addConstraint(Flow_Q_From);

                /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
                 q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
                 +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
                Flow_Q_To += a->qj_t[t];
                Flow_Q_To += (a->ch / 2 + a->b) * dest->w_t[t];
                Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_To = 0;
                _model->addConstraint(Flow_Q_To);

                /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
                 */
                Constraint SOCP("SOCP");
                SOCP += a->src->w_t[t] * a->dest->w_t[t];
                SOCP -= ((ap->wr_t[t]) ^ 2);
                SOCP -= ((ap->wi_t[t]) ^ 2);
                SOCP >= 0;
                _model->addConstraint(SOCP);


            }
            add_AC_thermal_Time(a);        //done
            add_SOCP_Angle_Bounds_Time(a);
        }

    }
}

void PowerModel::post_SOCP_PF_PV_Time() {
    Node *src = NULL;
    Node *dest = NULL;
    double tot_pl = 0;
/*    for (int t = 0; t < _timesteps; t++) {
        if (_net->_radiation[t] == 0) {
            Constraint All_zero("All_zero_pv_if_no_radiation_t" + to_string(t));
            for (auto n: _net->nodes) {

                //        Link_PV_Rate += n->pv_t[t]-(_net->_radiation[t])*(n->pv_rate);
                All_zero += n->pv_t[t];
                All_zero = 0;
                _model->addConstraint(All_zero);
            }
        }
    }*/
    for (auto n:_net->nodes) {
        add_SOCP_KCL_PV_Time(n);         //sdone
//        add_link_PV_Rate_NoCurt_Time(n);      //no curtailment
        if (n->candidate()) {
            add_link_PV_Rate(n);      //with curtailment
        }
        else if (n->inst()) {
            add_link_PV_fixed_Rate(n);      //with curtailment
        }
        
/*        Constraint Sum_PV_bound("Sum_PV_bound" + n->_name);
        Sum_PV_bound += n->pv_rate;
        Sum_PV_bound <= (3./1000.)*(_net->bMVA); // 3MW limit on the total PV
        _model->addConstraint(Sum_PV_bound);*/

        tot_pl += n->pl();
    }

    cout << "Total pl = " << tot_pl << endl;


    for (auto a:_net->arcs) {
        Arc *ap = nullptr;
        if (a->status == 1) {
            for (int t = 0; t < _timesteps; t++) {
                src = a->src;
                dest = a->dest;
                /** subject to Flow_P_From {(l,i,j) in arcs_from}:
                 p[l,i,j] = g[l]*(v[i]/tr[l])^2
                 + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_P_From("Flow_P_From" + a->pi._name);
                Flow_P_From += a->pi_t[t];
                //        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
                //            cout << "ok";
                //            cout << endl;
                //        }

                Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w_t[t];
                ap = _net->get_arc(a->src, a->dest);
                Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_From = 0;
                _model->addConstraint(Flow_P_From);

                /** subject to Flow_P_To {(l,i,j) in arcs_to}:
                 p[l,i,j] = g[l]*v[i]^2
                 + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_P_To("Flow_P_To" + a->pj._name);
                Flow_P_To += a->pj_t[t];
                Flow_P_To -= a->g * dest->w_t[t];
                Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_To = 0;
                _model->addConstraint(Flow_P_To);

                /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
                 q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
                 +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_Q_From("Flow_Q_From" + a->qi._name);
                Flow_Q_From += a->qi_t[t];
                Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w_t[t];
                Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_From = 0;
                _model->addConstraint(Flow_Q_From);

                /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
                 q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
                 +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                 + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
                 */
                Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
                Flow_Q_To += a->qj_t[t];
                Flow_Q_To += (a->ch / 2 + a->b) * dest->w_t[t];
                Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_To = 0;
                _model->addConstraint(Flow_Q_To);

                /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
                 */
                Constraint SOCP("SOCP");
                SOCP += a->src->w_t[t] * a->dest->w_t[t];
                SOCP -= ((ap->wr_t[t]) ^ 2);
                SOCP -= ((ap->wi_t[t]) ^ 2);
                SOCP >= 0;
                _model->addConstraint(SOCP);


            }

            add_AC_thermal_Time(a);
            //add_Wr_Wi_time(a);

        }
    }

}




void PowerModel::add_fixed_V(Gen* g){
    Constraint Fixed_v("Fixed_v"+g->_name);
    Fixed_v += g->_bus->_V_.square_magnitude();
    Fixed_v = pow(g->_bus->vs,2);
    _model->addConstraint(Fixed_v);
}

void PowerModel::add_fixed_Pg(Gen *g){
    Constraint Fixed_Pg("Fixed_Pg"+g->_name);
    Fixed_Pg += g->pg;
    Fixed_Pg = g->ps;
    _model->addConstraint(Fixed_Pg);
}


void PowerModel::reset(){
    delete _solver;
    delete _model;
    _solver = NULL;
    _model = NULL;
};

void PowerModel::run_grb_lin_test(){
    var<bool> x, y, z;
    x.init("x",0.0,1.0);
    y.init("y",0.0,1.0);
    z.init("z",0.0,1.0);
    
    _model->addVar(x);
    _model->addVar(y);
    _model->addVar(z);
    
    Constraint C1("C1");
    C1 += x + 2*y + 3*z;
    C1 <= 4;
    _model->addConstraint(C1);
    
    Constraint C2("C2");
    C2 += x + y;
    C2 >= 1;
    _model->addConstraint(C2);
    
    Function* obj = new Function();
    *obj += x + y + 2*z;
    _model->setObjective(obj);
    solve();
}

void PowerModel::run_grb_quad_test(){
    var<double> x, y, z;
    x.init("x",0.0,1.0);
    y.init("y",0.0,1.0);
    z.init("z",0.0,1.0);
    
    _model->addVar(x);
    _model->addVar(y);
    _model->addVar(z);
    
    Constraint C1("C1");
    C1 += x + 2*y + 3*z;
    C1 >= 4;
    _model->addConstraint(C1);
    
    Constraint C2("C2");
    C2 += x + y;
    C2 >= 1;
    _model->addConstraint(C2);
    
    Function* obj = new Function();
    *obj += x*x + x*y + y*y + y*z + z*z + 2*x;
    _model->setObjective(obj);
    solve();
}


/** Accessors */
Function* PowerModel::objective(){
    return _model->_obj;
}



/** Objective Functions */
void PowerModel::min_cost(){
    _objective = MinCost;
    Function* obj = new Function();
    for (auto g:_net->gens) {
        if (!g->_active)
            continue;
        *obj += _net->bMVA*g->_cost->c1*(g->pg) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg^2) + g->_cost->c0;
    }
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
//        obj->print(true);
    //    _solver->run();
}

void PowerModel::min_cost_time() {
    _objective = MinCostTime;
    Function* obj = new Function();
    for (int y = 0; y < _net->_nb_years; y++) {
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (auto g:_net->gens) {
                if ((t>=0)&&(t<24)){ //summer Feb
                    *obj += 20*6*_net->bMVA*(_net->weekday_cost[t])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
                if ((t>=24)&&(t<48)){ //spring or autumn Sep
                    *obj += 20*6*_net->bMVA*(_net->weekday_cost[t-24])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
                if ((t>=48)&&(t<72)){ //weekends
                    *obj += 2.6*4*12*_net->bMVA*(_net->weekend_cost[t-48])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
            }
            
        }
    }
    *obj /=1000000;    //millions
    _model->setObjective(obj);
    _model->setObjectiveType(minimize);
}

void PowerModel::min_cost_peak() {
    _objective = MinCostTime;
    Function* obj = new Function();
    for (int y = 0; y < _net->_nb_years; y++) {
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (auto g:_net->gens) {
                if ((t>=0)&&(t<24)){ //summer Feb
                    *obj += 20*6*_net->bMVA*(_net->weekday_cost[t])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
                if ((t>=24)&&(t<48)){ //spring or autumn Sep
                    *obj += 20*6*_net->bMVA*(_net->weekday_cost[t-24])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
                if ((t>=48)&&(t<72)){ //weekends
                    *obj += 2.6*4*12*_net->bMVA*(_net->weekend_cost[t-48])*(pow(_net->_demand_growth,y)*g->pg_t[t])*1000*pow(_net->_price_inflation,y);
                }
            }
            
        }
        *obj += 12*_net->bMVA*1000*30*1.8*0.16952*_net->max_kVas_year[y];
    }
    
    *obj /=1000000;
    obj->print(true);
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
}


/** Objective Functions */
void PowerModel::min_cost_pv(){
    _objective = MinCostPv;
    Function* obj = new Function();
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        for (auto g:_net->gens) {
            //            *obj += _net->bMVA*0.050060*1000*(g->pg_t[t])/6;          //$0.05006/kwh*(1/6hour)*pg=cost
            //            *obj += _net->bMVA*_net->weekday_cost[t]*1000*(g->pg_t[t])/6;          //$c1/kwh*(1/6hour)*pg=cost
            if ((t>=0)&&(t<24)){ //summer Feb
                *obj += 20*6*_net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;
            }
            if ((t>=24)&&(t<48)){ //spring or autumn Sep
                *obj += 20*6*_net->bMVA*(_net->weekday_cost[t-24])*(g->pg_t[t])*1000*1.3;
            }
            
            //*obj += _net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            if ((t>=48)&&(t<72)){ //weekend
                *obj += 2.6*4*12*_net->bMVA*(_net->weekend_cost[t-48])*(g->pg_t[t])*1000*1.3;
            }
            //*obj += _net->bMVA*(_net->weekend_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            
            
            //            *obj += _net->bMVA*0.169520*(g->pg_t[t] + g->qg)*1000;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            //        *obj += _net->bMVA*g->_cost->weekday_cost*(g->pg_t[t]) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg_t[t]^2) + g->_cost->c0;
        }
        
    }
    
    *obj *= _net->_nb_years;   // 10 years
//    for (auto n:_net->nodes){
//        if (n->candidate()) {
//            //                *obj += 0.0000001*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
//            //                *obj += n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
//            *obj += 1.04*_pv_cost*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
////            *obj += (350.*_net->bMVA*1000)*n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
//            
//        }
//    }
    *obj /=1000000;    //120//    *obj = (*obj/_timesteps);
//    *obj = *obj/24;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
//    obj->print(true);
    //    _solver->run();
}

/** Objective Functions */
void PowerModel::min_cost_pv_fixed(){
    _objective = MinCostPv;
    Function* obj = new Function();
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        for (auto g:_net->gens) {
            //            *obj += _net->bMVA*0.050060*1000*(g->pg_t[t])/6;          //$0.05006/kwh*(1/6hour)*pg=cost
            //            *obj += _net->bMVA*_net->weekday_cost[t]*1000*(g->pg_t[t])/6;          //$c1/kwh*(1/6hour)*pg=cost
            if ((t>=0)&&(t<24)){ //summer Feb
                *obj += 20*6*_net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;
            }
            if ((t>=24)&&(t<48)){ //spring or autumn Sep
                *obj += 20*6*_net->bMVA*(_net->weekday_cost[t-24])*(g->pg_t[t])*1000*1.3;
            }
            
            //*obj += _net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            if ((t>=48)&&(t<72)){ //weekend
                *obj += 2.6*4*12*_net->bMVA*(_net->weekend_cost[t-48])*(g->pg_t[t])*1000*1.3;
            }
            //*obj += _net->bMVA*(_net->weekend_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            
            
            //            *obj += _net->bMVA*0.169520*(g->pg_t[t] + g->qg)*1000;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            //        *obj += _net->bMVA*g->_cost->weekday_cost*(g->pg_t[t]) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg_t[t]^2) + g->_cost->c0;
        }
        
    }
    
    *obj *= _net->_nb_years;   // 10 years
//    for (auto n:_net->nodes){
//        if (n->candidate()) {
//            //                *obj += 0.0000001*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
//            //                *obj += n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
//            *obj += 1.04*_pv_cost*n->max_pv_size; // 2 dollars per W = 2000 dollars per square meter divided by ten years
//            //            *obj += (350.*_net->bMVA*1000)*n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
//            
//        }
//    }
    *obj /=1000000;    //120//    *obj = (*obj/_timesteps);
    //    *obj = *obj/24;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
    obj->print(true);
    //    _solver->run();
}


void PowerModel::min_cost_batt(){
    _objective = MinCostBatt_t;
    Function* obj = new Function();
    for (int t = 0; t < _timesteps; t++) {
        for (auto g:_net->gens) {
//            *obj += _net->bMVA*0.050060*1000*(g->pg_t[t])/6;          //$0.05006/kwh*(1/6hour)*pg=cost
//            *obj += _net->bMVA*_net->weekday_cost[t]*1000*(g->pg_t[t])/6;          //$c1/kwh*(1/6hour)*pg=cost
            *obj += _net->bMVA*_net->weekday_cost[t]*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
//            *obj += _net->bMVA*0.169520*(g->pg_t[t] + g->qg)*1000;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
            //        *obj += _net->bMVA*g->_cost->weekday_cost*(g->pg_t[t]) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg_t[t]^2) + g->_cost->c0;
        }
//        for (auto n:_net->nodes) {
//            if (n->in()) {
//            *obj += 2.5*1000000*0.01*n->pv_rate*_net->bMVA/(365*24*6) + 2.5*1000000*n->pv_rate*_net->bMVA/(10*365*24*6); // 1% of investment cost of 2.5$/W, divided by the number of days in a year.(10min simulation)
//            *obj += 1.5*1000000*0.01*n->pv_rate*_net->bMVA/(365*24) + 1.5*1000000*n->pv_rate*_net->bMVA/(30*365*24); // 1% of investment cost of 2.5$/W, divided by the number of days in a year.(1 hour simulation) keep using for 20 years
//                *obj += 1.15*n->pv_t[t]*1000*_net->bMVA; // $0.15/kWh -> $150/MWh paid over 1 year
                //            *obj += _net->bMVA*(n->batt_cap)*1000000/6/(10*365*24*6); // $1000000/MWh battery investment for 10 years.(10min simulation)
//            *obj += _net->bMVA*(n->batt_cap)*1000000/(30*365*24); // $1000000/MWh battery investment for 30 years.(1 hour simulation)
                //*obj += _net->bMVA*(n->batt_cap)*1000*0.20; //$0.20/kwh battery investment = $200/MWh paid over one year
                //*obj += _net->bMVA*(n->pdis_t[t])*1000*0.2;
//            }
//        }

    }
    for (auto n:_net->nodes){
        if (n->candidate()) {
            //                *obj += 0.0000001*n->pv_rate; /* 2 dollars per W = 2000 dollars per square meter divided by ten years */
            //                *obj += n->batt_cap; /* 3500 dollars per 10kW battery divided by ten years */
//            *obj += _pv_cost/(10*365.)*n->pv_rate; /* 2 dollars per W = 2000 dollars per square meter divided by ten years */
            *obj += (350.*_net->bMVA*1000)/(10*365)*n->batt_cap; /* 3500 dollars per 10kW battery divided by ten years */
            
        }
    }//    *obj = *obj/_timesteps;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
//    obj->print(true);
    //    _solver->run();
}

/** Objective Functions */
/*void PowerModel::min_cost_pv_batt(){
    _objective = MinCostPvBatt;
    Function* obj = new Function();
    //for (int month = 1; month < 5; month++){
        for (int t = 0; t < _timesteps; t++) {
            for (auto g:_net->gens) {
                //            *obj += _net->bMVA*0.050060*1000*(g->pg_t[t])/6;          //$0.05006/kwh*(1/6hour)*pg=cost
                //            *obj += _net->bMVA*_net->weekday_cost[t]*1000*(g->pg_t[t])/6;          //$c1/kwh*(1/6hour)*pg=cost
                if (_net->tag[t]==1){
 
                    *obj += _net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                }else{
 
                    *obj += _net->bMVA*(_net->weekend_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                }
                
                //            *obj += _net->bMVA*0.169520*(g->pg_t[t] + g->qg)*1000;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                //        *obj += _net->bMVA*g->_cost->weekday_cost*(g->pg_t[t]) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg_t[t]^2) + g->_cost->c0;
            }
            
        }
   // }
   // *obj *= _nb_years;   // 10 years
    for (auto n:_net->nodes){
        if (n->candidate()) {
            *obj += _pv_cost*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
            *obj += (350.*_net->bMVA*1000)*n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
            
        }
    }
    // *obj /=1000000;    //120
    // *obj = *obj/_timesteps;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
    obj->print(true);
    //    _solver->run();
}*/


/** Objective Functions */
void PowerModel::min_cost_pv_batt(){
    _objective = MinCostPvBatt;
    Function* obj = new Function();
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            for (auto g:_net->gens) {
                //            *obj += _net->bMVA*0.050060*1000*(g->pg_t[t])/6;          //$0.05006/kwh*(1/6hour)*pg=cost
                //            *obj += _net->bMVA*_net->weekday_cost[t]*1000*(g->pg_t[t])/6;          //$c1/kwh*(1/6hour)*pg=cost
                    if ((t>=0)&&(t<24)){ //summer Feb
                        *obj += 20*6*_net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;
                    }
                    if ((t>=24)&&(t<48)){ //spring or autumn Sep
                        *obj += 20*6*_net->bMVA*(_net->weekday_cost[t-24])*(g->pg_t[t])*1000*1.3;
                    }
                
                    //*obj += _net->bMVA*(_net->weekday_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                    if ((t>=48)&&(t<72)){ //weekend
                        *obj += 2.6*4*12*_net->bMVA*(_net->weekend_cost[t-48])*(g->pg_t[t])*1000*1.3;
                    }
                    //*obj += _net->bMVA*(_net->weekend_cost[t])*(g->pg_t[t])*1000*1.3;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                
                
                //            *obj += _net->bMVA*0.169520*(g->pg_t[t] + g->qg)*1000;          //$c1/kwh*(1 hour)*pg(MWh)*1000=cost //currently using
                //        *obj += _net->bMVA*g->_cost->weekday_cost*(g->pg_t[t]) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg_t[t]^2) + g->_cost->c0;
            }
            
        }
    
    *obj *= _net->_nb_years;   // 10 years
        for (auto n:_net->nodes){
            if (n->candidate()) {
//                *obj += 0.0000001*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
//                *obj += n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
                *obj += _net->_pv_cost*n->pv_rate; // 2 dollars per W = 2000 dollars per square meter divided by ten years
                *obj += (350.*_net->bMVA*1000)*n->batt_cap; // 3500 dollars per 10kW battery divided by ten years
                
            }
        }
    *obj /=1000000;    //120
    //*obj = *obj/_timesteps;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
    //obj->print(true);
    //    _solver->run();
}



void PowerModel::min_var(var<>& v){
    _objective = MinDelta;
    Function* obj = new Function();
    *obj += v;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize);
}

void PowerModel::max_var(var<>& v){
    _objective = MaxDelta;
    Function* obj = new Function();
    *obj += v;
    _model->setObjective(obj);
    _model->setObjectiveType(maximize);
}



int PowerModel::solve(int output, bool relax) {


/*
    for (int t = 0; t < _timesteps; t++) {
        for (auto a:_net->arcs) {

            cout << "Power loss =" << a->pi_t[t].get_value() + a->pj_t[t].get_value() << endl;


            sum_power_loss += a->pi_t[t].get_value() + a->pj_t[t].get_value();

        }

        cout << "Total power loss =" << sum_power_loss;*/
        return _solver->run(output, relax);


}

void PowerModel::add_AC_gen_vars(){
    for (auto g:_net->gens) {
        if (!g->_active)
            continue;
//        g->pg.init("pg"+g->_name+":"+g->_bus->_name, g->pbound.min, g->pbound.max);
        g->pg.init("pg"+g->_name+":"+g->_bus->_name, 0, g->pbound.max);
//        g->pg = g->ps;
//        g->pg = g->pbound.max;
        _model->addVar(g->pg);
        g->qg.init("qg"+g->_name+":"+g->_bus->_name, g->qbound.min, g->qbound.max);
//        g->qg = g->qs;
        _model->addVar(g->qg);
        g->init_complex();
    }
}

void PowerModel::add_AC_Pol_vars(){
    for (auto n:_net->nodes) {
        n->theta.init("t"+n->_name);
        n->v.init("v"+n->_name, n->vbound.min, n->vbound.max);
        n->v = n->vs;
//                n->v = 1;
        _model->addVar(n->v);
        _model->addVar(n->theta);
        n->init_complex(true);
        
    }
    add_AC_gen_vars();
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pi);
        _model->addVar(a->pj);
        _model->addVar(a->qi);
        _model->addVar(a->qj);
        a->init_complex();
    }
}

void PowerModel::compute_losses(){
    vector<double> losses;
    double loss_t = 0;
    double gen = 0, ratio = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        loss_t = 0;
        gen = (*_net->gens.begin())->pg_t[t].get_value();
        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            loss_t += a->pi_t[t].get_value() + a->pj_t[t].get_value();
        }
        losses.push_back(loss_t);
        cout << "Losses at time step " << t << " = " << loss_t*1000*_net->bMVA << endl;
        ratio = (gen - loss_t)/gen;
        cout << "Relative losses at time step " << t << " = " << ratio*100 << "%" << endl;
    }
}

void PowerModel::add_SOCP_Rect_Batt_vars_Time(){
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps);
        a->pj_t.resize(_timesteps);
        a->qi_t.resize(_timesteps);
        a->qj_t.resize(_timesteps);
        a->wr_t.resize(_timesteps);
        a->wi_t.resize(_timesteps);
    }

    for (auto n:_net->nodes) {

        n-> w_t.resize(_timesteps);
    }

    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps);
        g->qg_t.resize(_timesteps);
    }


    for (int t = 0; t < _timesteps; t++) {
        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->qj_t[t]);
            a->wr_t[t].init("wr("+a->_name, a->src->vbound.min*a->dest->vbound.min, a->src->vbound.max*a->dest->vbound.max);
            a->wr_t[t] = 1;
            _model->addVar(a->wr_t[t]);
            a->wi_t[t].init("wi("+a->_name, -a->src->vbound.max*a->dest->vbound.max, a->src->vbound.max*a->dest->vbound.max);
            _model->addVar(a->wi_t[t]);
            a->init_complex();
        }

        for (auto n:_net->nodes) {

            n->w_t[t].init("w" + n->_name + "_" + to_string(t), pow(n->vbound.min, 2), pow(n->vbound.max, 2));
            n->w_t[t] = pow(n->vs, 2);
            _model->addVar(n->w_t[t]);

            n->init_lifted_complex();
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
        }
    }
    add_Battery_vars();
}

void PowerModel::add_SOCP_Rect_PV_Batt_vars_Time(){
    add_SOCP_Rect_PV_vars_Time();
    add_Battery_vars();
}


void PowerModel::add_AC_Rect_Batt_vars_Time(){
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps);
        a->pj_t.resize(_timesteps);
        a->qi_t.resize(_timesteps);
        a->qj_t.resize(_timesteps);

    }

    for (auto n:_net->nodes) {
        n->vr_t.resize(_timesteps);
        n->vi_t.resize(_timesteps);
        if (n->inst()) {
            n->pv_t.resize(_timesteps);
        }
    }

    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps);
        g->qg_t.resize(_timesteps);
    }
    add_Battery_vars();

    for (int t = 0; t < _timesteps; t++) {

        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->qj_t[t]);

        }

        for (auto n:_net->nodes) {
            n->vr_t[t].init("vr"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            n->vr_t[t] = n->vs;
            //                n->vr = 1;
            _model->addVar(n->vr_t[t]);
            n->vi_t[t].init("vi"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            _model->addVar(n->vi_t[t]);
            if(n->inst()){
                n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
            }

        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
        }
    }
    
}


void PowerModel::add_Battery_vars(){
    for (auto n:_net->nodes) {  //for all nodes
        if(n->candidate()) {  //if this node is a candidate for PV installation
            n->pch_t.resize(_timesteps*_nb_days);  //active power charging value, reserve mem to variables
            n->pdis_t.resize(_timesteps*_nb_days);  //active power discharging value
            n->soc_t.resize(_timesteps*_nb_days);  //state of charge
            n->batt_cap.init("battery capacity"+n->_name, 0, 500./(_net->bMVA*1000.));  //battery capacity, give name to variable
            _model->addVar(n->batt_cap);   //add battery capacity to model
        }
    }
    for (int t = 0; t < _timesteps*_nb_days; t++) {  //for each time step, initialize and add 3 variables dependent on time
        for (auto n:_net->nodes) {
            if(n->candidate()) {  //if this node is a candidate for PV installation
                n->pch_t[t].init("pcharge"+n->_name+"_" + to_string(t), 0, 500./(_net->bMVA*1000.));
                _model->addVar(n->pch_t[t]);
                n->pdis_t[t].init("pdischarge"+n->_name+"_" + to_string(t), 0, 500./(_net->bMVA*1000.));
                _model->addVar(n->pdis_t[t]);
                n->soc_t[t].init("state_of_charge"+n->_name+"_" + to_string(t), 0, 500./(_net->bMVA*1000.));
                _model->addVar(n->soc_t[t]);
            }
            
        }
    }

}
void PowerModel::add_AC_Rect_PV_Batt_vars_Time(){
    add_AC_Rect_PV_vars_Time();  //add PV variables to model along with time
    add_Battery_vars();  //add battery variables
}



void PowerModel::add_AC_Rect_vars_Time() {
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps*_nb_days);
        a->pj_t.resize(_timesteps*_nb_days);
        a->qi_t.resize(_timesteps*_nb_days);
        a->qj_t.resize(_timesteps*_nb_days);

    }

    for (auto n:_net->nodes) {
        n->vr_t.resize(_timesteps*_nb_days);
        n->vi_t.resize(_timesteps*_nb_days);
        if (n->inst()) {
            n->pv_t.resize(_timesteps*_nb_days);
        }

    }

    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps*_nb_days);
        g->qg_t.resize(_timesteps*_nb_days);
    }


    for (int t = 0; t < _timesteps*_nb_days; t++) {

        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->qj_t[t]);

            a->init_complex();

        }

        for (auto n:_net->nodes) {
            n->vr_t[t].init("vr"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            n->vr_t[t] = n->vs;
            //                n->vr = 1;
            _model->addVar(n->vr_t[t]);
            n->vi_t[t].init("vi"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            _model->addVar(n->vi_t[t]);
            if(n->inst()){
                n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
            }



        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
            g->pg_t[t]._print = true;
            g->qg_t[t]._print = true;

        }

    }
}

void PowerModel::add_NF_vars(bool PV) {
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps*_nb_days);
        a->qi_t.resize(_timesteps*_nb_days);
    }
    
    for (auto n:_net->nodes) {
        if (n->inst()|| (PV && n->candidate())) {
            n->pv_t.resize(_timesteps*_nb_days);
        }
    }
    
    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps*_nb_days);
        g->qg_t.resize(_timesteps*_nb_days);
    }
    
    
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        
        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            
            a->init_complex();
            
        }
        
        for (auto n:_net->nodes) {
            if(n->inst() || (PV && n->candidate())){
                n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
            }
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
            g->pg_t[t]._print = true;
            g->qg_t[t]._print = true;
            
        }
        
    }
}

void PowerModel::add_AC_Rect_vars_Peak() {
    add_AC_Rect_vars_Time();
    _net->max_kVas_year.resize(_net->_nb_years);
    for (int y = 0; y<_net->_nb_years; y++) {
        _net->max_kVas_year[y].init("max_kVA_"+to_string(y), 0, 1);
        _model->addVar(_net->max_kVas_year[y]);
        _net->max_kVas_year[y]._print = true;
    }
}



void PowerModel::add_AC_Rect_PV_vars_Peak() {
    add_AC_Rect_PV_vars_Time();
    _net->max_kVas_year.resize(_net->_nb_years);
    for (int y = 0; y<_net->_nb_years; y++) {
        _net->max_kVas_year[y].init("max_kVA_"+to_string(y), 0, 1);
        _model->addVar(_net->max_kVas_year[y]);
        _net->max_kVas_year[y]._print = true;
    }
    
}

int PowerModel::get_nb_days_in_month(const tm& timeinfo) const{
    int numberOfDays;
    if (timeinfo.tm_mon == 3 || timeinfo.tm_mon == 5 || timeinfo.tm_mon == 8 || timeinfo.tm_mon == 10)
        numberOfDays = 30;
    else if (timeinfo.tm_mon == 1)
    { bool isLeapYear = ((timeinfo.tm_year+1900) % 4 == 0 && (timeinfo.tm_year+1900) % 100 != 0) || ((timeinfo.tm_year+1900) % 400 == 0);
        if (isLeapYear)
            numberOfDays = 29;
        else
            numberOfDays = 28;
    }  
    else  
        numberOfDays = 31;
    return numberOfDays;
}



void PowerModel::add_COPPER_vars(bool PV) {
    
    _irrad_start_date = new tm();
    _irrad_start_date->tm_year = get<0>(_net->_date[0]) - 1900;
    _irrad_start_date->tm_mon = get<1>(_net->_date[0]);
    _irrad_start_date->tm_mday = get<2>(_net->_date[0]);
    _irrad_start_date->tm_hour = get<3>(_net->_date[0]);
    
    mktime ( _irrad_start_date );
    
    _demand_start_date = new tm();
    _demand_start_date->tm_year = get<0>(_net->_date[0]) - 1900;
    _demand_start_date->tm_mon = get<1>(_net->_date[0]);
    _demand_start_date->tm_mday = get<2>(_net->_date[0]);
    _demand_start_date->tm_hour = get<3>(_net->_date[0]);
    mktime ( _demand_start_date );
    cout << "Historical demand data start time is: " << asctime(_demand_start_date);
    cout << "Simulation start date is: " << asctime (_start_date);
    int nb_days = 0;
    int idx = 0;
    auto copy_time = *_start_date;
    _net->max_kVas_month.resize(_net->_nb_years*12);// 1 per month
    _net->max_kVas_year.resize(_net->_nb_years*12);
    cout << "Building Models...\n";
    for (int y = 0; y<_net->_nb_years; y++) {
        for (int m = 0; m<12; m++) {
            _net->max_kVas_year[y*12+m].init("max_kVA_year_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year), 0, 10e10);
//            _model->addVar(_net->max_kVas_year[y*12+m]);
            _net->max_kVas_year[y]._print = true;
            _net->max_kVas_year[y*12+m] = 0;
            _net->max_kVas_month[y*12+m].init("max_kVA_month_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year), 0, 10e10);
//            _model->addVar(_net->max_kVas_month[y*12+m]);
            _net->max_kVas_month[y*12+m]._print = true;
            _net->max_kVas_month[y*12+m] = 0;
            nb_days = get_nb_days_in_month(copy_time);
            _net->gen.resize(_net->gen.size()+nb_days*24);
            _net->pv_gen.resize(_net->pv_gen.size()+nb_days*24);
            _net->batt.resize(_net->batt.size()+nb_days*24);
            
            for (int d = 0; d<nb_days; d++) {
                for (int h = 0; h<24; h++) {
//                    if (PV) {
                        _net->pv_gen[idx].init("pv_gen_"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year)+" : "+to_string(copy_time.tm_hour)+ "h", 0, _net->PV_CAP);
                        (_net->pv_gen[idx])._print = true;
                        _net->pv_gen[idx] = 0;
                    _net->batt[idx].init("batt"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year)+" : "+to_string(copy_time.tm_hour)+ "h", 0, _net->BATT_CAP);
                    (_net->batt[idx])._print = true;
                    _net->batt[idx] = 0;
//                        _model->addVar(_net->pv_gen[idx]);
//                    }
                    _net->gen[idx].init("gen_"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon+1)+"_"+to_string(1900+copy_time.tm_year)+" : "+to_string(copy_time.tm_hour)+ "h", 0, 50000);// Max set to 50 MW
                    _net->gen[idx]._print = true;
                    _net->gen[idx] = 0;
//                    _model->addVar(_net->gen[idx]);
                    copy_time.tm_hour += 1;
                    idx++;
                }
                copy_time.tm_mday = (copy_time.tm_mday + 1)%nb_days;
            }
            mktime(&copy_time);

        }
    }
//    cout << endl;
//    cout << "idx = " << idx << endl;
    
}



void PowerModel::post_COPPER(bool PV){
    _objective = MinCostTime;
    Function* obj = new Function();
    int nb_days = 0;
    auto copy_time = *_start_date;
    int idx = 0;
    _net->max_kVas_month.resize(_net->_nb_years*12);
    _net->max_kVas_year.resize(_net->_nb_years*12);
    for (int y = 0; y<_net->_nb_years; y++) {
        for (int m = 0; m<12; m++) {
            if (y==0 || (y==1 && m==1)) {
                Constraint Max_kVA_year("Max_kVA_year_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year));
                Max_kVA_year += _net->max_kVas_year[12*y+m];
                Max_kVA_year >= 17738.130;
                _model->addConstraint(Max_kVA_year);
            }
            else {
                for (int i = 1; i<=13; i++) {
                    Constraint Max_kVA_year("Max_kVA_year_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year)+"_"+to_string(i));
                    Max_kVA_year += _net->max_kVas_year[12*y+m];
                    Max_kVA_year -= _net->max_kVas_month[12*y+m-i];
                    Max_kVA_year >= 0;
                    _model->addConstraint(Max_kVA_year);
                }
                
            }
            *obj += 0.16952*pow(_net->_price_inflation,y)*(_net->max_kVas_year[12*y+m] + _net->max_kVas_month[12*y+m]);
            nb_days = get_nb_days_in_month(copy_time);
            for (int d = 0; d<nb_days; d++) {
                Constraint Max_kVA_month("Max_kVA_month_"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year));
                Max_kVA_month += _net->max_kVas_month[12*y+m];
                for (int h = 0; h<24; h++) {
                    if (copy_time.tm_wday==0||copy_time.tm_wday==6) {//Weekend
                        *obj += _net->weekend_cost[copy_time.tm_hour]*pow(_net->_price_inflation,y)*(_net->gen[idx]);
                    }
                    else{
                        *obj += _net->weekday_cost[copy_time.tm_hour]*pow(_net->_price_inflation,y)*(_net->gen[idx]);
                    }
                    Constraint Demand("Demand_"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year)+" : "+to_string(copy_time.tm_hour)+ "h");
                    Demand += _net->gen[idx];
                    Max_kVA_month -= (1/_net->_power_factor[copy_time.tm_hour])*_net->gen[idx];
                    if (PV) {
                        Demand += _net->pv_gen[idx];
                    }
                    Demand -= pow(_net->_demand_growth, y)*_net->_load_kW[idx];
                    Demand >= 0;
                    _model->addConstraint(Demand);
                    if (PV) {
                        Constraint PV_gen("PV_gen_"+to_string(copy_time.tm_mday)+"_"+to_string(copy_time.tm_mon)+"_"+to_string(1900+copy_time.tm_year)+" : "+to_string(copy_time.tm_hour)+ "h");
                        PV_gen += _net->pv_gen[idx];
                        PV_gen -= pow(_net->_demand_growth, y)*_net->_load_kW[idx%_net->_load_kW.size()];
                        PV_gen >= 0;
                        _model->addConstraint(PV_gen);
                    }
                    copy_time.tm_hour += 1;
                    mktime(&copy_time);
                    idx++;
                }
                Max_kVA_month *= nb_days;
                Max_kVA_month >= 0;
                _model->addConstraint(Max_kVA_month);
            }
            
        }
    }
//    *obj /=1000000;
    obj->print(true);
    _model->setObjective(obj);
    _model->setObjectiveType(minimize);    
}

int PowerModel::get_week_day_of_1st(tm time_c){
    auto copy_time = time_c;
    int year = copy_time.tm_year +1900;
    int month = copy_time.tm_mon;
    int wday = 1;// The first day of 2001 was a monday
    int curr_year = 2001;
    for (int i = 0; i<year-2001; i++) {
        wday = (wday + 1)%7;
        if(((curr_year%4) == 0) && (((curr_year%100)!=0) || ((curr_year%400) == 0))){   //leap year
            wday = (wday + 1)%7;
        }
        curr_year++;
    }
    copy_time.tm_mon = 0;
    for (int m = 0; m<month; m++) {
        wday = (wday + (get_nb_days_in_month(copy_time)%7))%7;
        copy_time.tm_mon += 1;
    }
    return wday;
}


/* Randomly pick a day that matches the weekday, week number and month stored in time_c in the years between start_year and end_year */
int PowerModel::random_load(tm time_c, int start_year, int end_year) {
    auto copy_time = time_c;
    int year = time_c.tm_year+1900;
    if ( year < start_year || year > end_year) { // No uncertainty for historical data
        year = _distr_year_load(_eng);
        copy_time.tm_year = year - 1900;// tm_year starts counting at 1900
        int week_nb = floor(time_c.tm_mday/7.);
        copy_time.tm_yday -= (copy_time.tm_mday - 1);// back to first day in month
        copy_time.tm_wday = get_week_day_of_1st(copy_time);
//        copy_time.tm_wday = time_c.tm_wday;
        copy_time.tm_mday = 1;
        int i = 0;
        while (i<week_nb) {
            if (copy_time.tm_wday==time_c.tm_wday || copy_time.tm_yday==364) {
                i++;
            }
            if (i<week_nb) {
                if (copy_time.tm_yday<364) {
                    copy_time.tm_yday++;
                    copy_time.tm_mday++;
                    copy_time.tm_wday = (copy_time.tm_wday + 1)%7;
                }
            }
        }
        while (copy_time.tm_wday!=time_c.tm_wday) {
            copy_time.tm_yday++;
            copy_time.tm_mday++;
            copy_time.tm_wday++;
            copy_time.tm_wday = copy_time.tm_wday%7;
        }
        //    copy_time.tm_isdst=0;
        
        
    }
//        mktime(&copy_time);
//        cout << "Load random:" << endl;
//        cout << "input time = " << asctime(&time_c) << endl;
//        cout << "output time = " << asctime(&copy_time) << endl;
//        cout << "yday = " << copy_time.tm_yday << endl;
    
    int new_idx = 0;
    int curr_year = start_year;
    for (int i=0; i<year - start_year; i++) {
        if(((curr_year%4) == 0) && (((curr_year%100)!=0) || ((curr_year%400) == 0))){   //leap year
            new_idx += 366*_timesteps;
        }
        else{
            new_idx += 365*_timesteps;
        }
        curr_year += 1;
    }
    new_idx += copy_time.tm_yday*_timesteps;
    return new_idx;
}

/* Randomly pick a day that matches the month stored in time_c in the years between start_year and end_year */
int PowerModel::random_weather(tm time_c, int start_year, int end_year) {
    auto copy_time = time_c;
//    if (copy_time.tm_isdst!=0) {
//        throw invalid_argument("is dst");
//        exit(-1);
//    }
    int year = time_c.tm_year+1900;
    if ( year < start_year || year > end_year) { // No uncertainty for past events
        year = _distr_year_PV(_eng);
        copy_time.tm_year = year - 1900;// tm_year starts counting at 1900
        int nb_days = get_nb_days_in_month(copy_time);
        int day = _distr_day(_eng)%nb_days;
        copy_time.tm_yday = copy_time.tm_yday - (copy_time.tm_mday - 1) + day;
        copy_time.tm_mday = day+1;
    }
//        mktime(&copy_time);
//        cout << "Weather random:" << endl;
//        cout << "input time = " << asctime(&time_c) << endl;
//        cout << "output time = " << asctime(&copy_time) << endl;
    int new_idx = 0;
    int curr_year = start_year;
    for (int i=0; i<year - start_year; i++) {
        if(((curr_year%4) == 0) && (((curr_year%100)!=0) || ((curr_year%400) == 0))){   //leap year
            new_idx += 366*_timesteps;
        }
        else{
            new_idx += 365*_timesteps;
        }
        curr_year += 1;
    }
    new_idx += copy_time.tm_yday*_timesteps;
    return new_idx;
}


void PowerModel::random_generator(double uncert_perc){
    int nb_days;
    auto copy_time = *_start_date;
    mktime(&copy_time);
    _distr_uncert = uniform_int_distribution<>(-uncert_perc, uncert_perc); // randomly pick a percentage
    _distr_year_PV = uniform_int_distribution<>(_irrad_start_date->tm_year+1900, _irrad_start_date->tm_year+1900 + _net->_irrad_nb_years-1); // randomly pick a year
    _distr_year_load = uniform_int_distribution<>(_demand_start_date->tm_year+1900, _demand_start_date->tm_year+1900 + _net->_demand_nb_years-1); // randomly pick a year
    _distr_day = uniform_int_distribution<>(0, 31); // randomly pick a day
//    copy_time.tm_isdst = 0;
    int idx = 0, new_idx;
    for (int y = 0; y<_net->_nb_years; y++) {
        copy_time.tm_mon = 0;
        copy_time.tm_yday = 0;
        for (int m = 0; m<12; m++) {
            copy_time.tm_mday = 1;
             nb_days = get_nb_days_in_month(copy_time);
            for (int d = 0; d<nb_days; d++) {
                    new_idx = random_load(copy_time, _demand_start_date->tm_year+1900, _demand_start_date->tm_year+1900 + _net->_demand_nb_years-1);
                    assert(new_idx>=0);
                    _random_load.push_back(new_idx);
                    _random_load_uncert.push_back((1 + _distr_uncert(_eng)/100.));
//                    _random_load_uncert.push_back(1);
                    new_idx = random_weather(copy_time, _irrad_start_date->tm_year+1900, _irrad_start_date->tm_year+1900 + _net->_irrad_nb_years-1);
                    assert(new_idx>=0);
                    _random_weather.push_back(new_idx);
                    _random_PV_uncert.push_back((1 + _distr_uncert(_eng)/100.));
//                    _random_PV_uncert.push_back(1);
                copy_time.tm_wday = (copy_time.tm_wday + 1)%7;
                copy_time.tm_mday++;
                copy_time.tm_yday++;
                idx++;
            }
            copy_time.tm_mon++;
        }
        copy_time.tm_year++;
    }
}

void PowerModel::post_COPPER_static(bool PV, bool Batt){
    double tot_off_peak = 0, tot_peak = 0, tot_ev = 0;
    double objective = 0;
    double tot_kw = 0, tot_PV = 0, tot_Batt_Ch = 0, tot_Batt_Dis = 0;
    double monthly_bill = 0;
    int nb_days = 0;
    int d_idx = 0;
    int r_load = 0;
    int r_weather = 0;
    auto copy_time = *_start_date;
    int idx = 0;
    _net->max_kVas_month.resize(_net->_nb_years*12);
    _net->max_kVas_year.resize(_net->_nb_years*12);
    vector<double> c1 = _net->weekday_cost;
    vector<double> c3 = _net->weekend_cost;
    double peak_tariff = _net->_peak_rate;
    for (int y = 0; y<_net->_nb_years; y++) {
//        if (y > 1 && y%3==0) {
//            if (y == 3) {
//            for (int h = 0; h<24; h++) {
//                c1[h] *= 1.2;
//                c3[h] *= 1.2;
//            }
//            peak_tariff *= 1.05;
//        }

        tot_PV = 0;
        for (int m = 0; m<12; m++) {
            tot_kw = 0;
            monthly_bill = 0;
            nb_days = get_nb_days_in_month(copy_time);
            _net->max_kVas_month[12*y+m].set_val(0);
            _net->max_kVas_year[12*y+m].set_val(0);
//            if (y==2 && m==0) {
//                cout << "here";
//            }
            tot_off_peak = 0;
            tot_ev = 0;
            tot_peak = 0;
            for (int d = 0; d<nb_days; d++) {
                r_load = _random_load[d_idx];
                r_weather = _random_weather[d_idx];


                for (int h = 0; h<24; h++) {
                    if (PV && Batt) {
                        auto pv_gen = _net->_radiation[r_weather+h] * _net->PV_CAP * _net->PV_EFF*pow(0.99, y) *_random_PV_uncert[d_idx];
                        _net->pv_gen[idx].set_val(pv_gen);
                        tot_PV += pv_gen;
                        double charge_amount = 0, discharge_amount = 0;
                        if (copy_time.tm_wday==0||copy_time.tm_wday==6) {//Weekend
                            if (c3[h]==_net->min_cost_weekend) {
                                //charge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc!=_net->BATT_CAP) {
                                    charge_amount = _net->BATT_CAP/5;
                                    if (prev_soc + charge_amount > _net->BATT_CAP) {
                                        charge_amount = _net->BATT_CAP - prev_soc;
                                    }
                                    _net->batt[idx].set_val(prev_soc + charge_amount);
                                    tot_Batt_Ch += charge_amount;
                                    //Import charge amount from PV then Grid if not sufficient pv
                                    if (pv_gen >= charge_amount) {
                                        pv_gen -= charge_amount;
                                        charge_amount = 0;
                                    }
                                    else {
                                        charge_amount -= pv_gen;
                                        pv_gen = 0;
                                        
                                    }
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                            else {
                                //discharge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc>_net->BATT_CAP*0.1) {
                                    discharge_amount = _net->BATT_CAP/5;
                                    if (prev_soc - discharge_amount < _net->BATT_CAP*0.1) {
                                        discharge_amount = prev_soc - _net->BATT_CAP*0.1;
                                    }
                                    _net->batt[idx].set_val(prev_soc - discharge_amount);
                                    tot_Batt_Dis += discharge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                        }
                        else{
                            if (c1[h]==_net->min_cost_week) {
                                //charge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc!=_net->BATT_CAP) {
                                    charge_amount = _net->BATT_CAP/5;
                                    if (prev_soc + charge_amount > _net->BATT_CAP) {
                                        charge_amount = _net->BATT_CAP - prev_soc;
                                    }
                                    _net->batt[idx].set_val(prev_soc + charge_amount);
                                    tot_Batt_Ch += charge_amount;
                                    //Import charge amount from PV then Grid if not sufficient pv
                                    if (pv_gen >= charge_amount) {
                                        pv_gen -= charge_amount;
                                        charge_amount = 0;
                                    }
                                    else {
                                        charge_amount -= pv_gen;
                                        pv_gen = 0;
                                        
                                    }
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                            else {
                                //discharge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc>_net->BATT_CAP*0.1) {
                                    discharge_amount = _net->BATT_CAP/5;
                                    if (prev_soc - discharge_amount < _net->BATT_CAP*0.1) {
                                        discharge_amount = prev_soc - _net->BATT_CAP*0.1;
                                    }
                                    _net->batt[idx].set_val(prev_soc - discharge_amount);
                                    tot_Batt_Dis += discharge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                        }
                        _net->gen[idx].set_val(1.*(pow(_net->_demand_growth, y)*_net->_load_kW[r_load+h]*_random_load_uncert[d_idx] - (pv_gen - 1/(_net->BATT_EFF*pow(0.9999, y))*charge_amount + _net->BATT_EFF*pow(0.9999, y)*discharge_amount)));
                    }
                    else if (PV && !Batt) {
                        _net->pv_gen[idx].set_val(_net->_radiation[r_weather+h] * _net->PV_CAP * _net->PV_EFF*pow(0.99, y) *_random_PV_uncert[d_idx]);
                        _net->gen[idx].set_val(1.*(pow(_net->_demand_growth, y)*_net->_load_kW[r_load+h]*_random_load_uncert[d_idx] - (_net->pv_gen[idx].get_value())));
                        tot_PV += _net->pv_gen[idx].get_value();
                    }
                    else if (!PV && Batt) {
                        double charge_amount = 0, discharge_amount = 0;
                        if (copy_time.tm_wday==0||copy_time.tm_wday==6) {//Weekend
                            if (c3[h]==_net->min_cost_weekend) {
                                //charge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc!=_net->BATT_CAP) {
                                    charge_amount = _net->BATT_CAP/5;
                                    if (prev_soc + charge_amount > _net->BATT_CAP) {
                                        charge_amount = _net->BATT_CAP - prev_soc;
                                    }
                                    _net->batt[idx].set_val(prev_soc + charge_amount);
                                    tot_Batt_Ch += charge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                            else {
                                //discharge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc>_net->BATT_CAP*0.1) {
                                    discharge_amount = _net->BATT_CAP/5;
                                    if (prev_soc - discharge_amount < _net->BATT_CAP*0.1) {
                                        discharge_amount = prev_soc - _net->BATT_CAP*0.1;
                                    }
                                    _net->batt[idx].set_val(prev_soc - discharge_amount);
                                    tot_Batt_Dis += discharge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                        }
                        else{
                            if (c1[h]==_net->min_cost_week) {
                                //charge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc!=_net->BATT_CAP) {
                                    charge_amount = _net->BATT_CAP/5;
                                    if (prev_soc + charge_amount > _net->BATT_CAP) {
                                        charge_amount = _net->BATT_CAP - prev_soc;
                                    }
                                    _net->batt[idx].set_val(prev_soc + charge_amount);
                                    tot_Batt_Ch += charge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                            else {
                                //discharge
                                double prev_soc = 0;
                                if (idx==0) {
                                    prev_soc=_net->BATT_CAP*0.1;
                                }
                                else {
                                    prev_soc =_net->batt[idx-1].get_value();
                                }
                                if (prev_soc>_net->BATT_CAP*0.1) {
                                    discharge_amount = _net->BATT_CAP/5;
                                    if (prev_soc - discharge_amount < _net->BATT_CAP*0.1) {
                                        discharge_amount = prev_soc - _net->BATT_CAP*0.1;
                                    }
                                    _net->batt[idx].set_val(prev_soc - discharge_amount);
                                    tot_Batt_Dis += discharge_amount;
                                }
                                else{
                                    _net->batt[idx].set_val(prev_soc);
                                }
                            }
                        }
//                        _net->gen[idx].set_val(1.*(pow(_net->_demand_growth, y)*_net->_load_kW[r_load+h]*_random_load_uncert[d_idx] + charge_amount - discharge_amount));
                        _net->gen[idx].set_val(1.*(pow(_net->_demand_growth, y)*_net->_load_kW[r_load+h]*_random_load_uncert[d_idx] + 1/(_net->BATT_EFF*pow(0.9999, y))*charge_amount - _net->BATT_EFF*pow(0.9999, y)*discharge_amount));
                        
                    }
                    else{ // No PV and no Batt
                        double val = _net->_load_kW[r_load+h]*_random_load_uncert[d_idx];
                        _net->gen[idx].set_val(1.*(pow(_net->_demand_growth, y)*val));
                    }
                    tot_kw += _net->gen[idx].get_value();
                    assert(tot_kw>=0);
                    _net->max_kVas_month[12*y+m].set_val(max(_net->max_kVas_month[12*y+m].get_value(), _winter_power_factor*_net->gen[idx].get_value()));
//                    _net->max_kVas_month[12*y+m].set_val(0.97*_net->gen[idx].get_value());
//                    if(y>0){
                        if (copy_time.tm_wday==0||copy_time.tm_wday==6) {//Weekend
//                            cout << "tariff = " << c3[copy_time.tm_hour] << endl;
                            objective += c3[h]*pow(_net->_price_inflation,y)*(_net->gen[idx].get_value());
                            monthly_bill +=c3[h]*pow(_net->_price_inflation,y)*(_net->gen[idx].get_value());;
                            tot_off_peak += _net->gen[idx].get_value();
                        }
                        else{
//                            cout << "h = " << h << " | tariff = " << c1[h] << endl;
                            objective += c1[h]*pow(_net->_price_inflation,y)*(_net->gen[idx].get_value());
                            monthly_bill += c1[h]*pow(_net->_price_inflation,y)*(_net->gen[idx].get_value());
                            if (h<7) {
                                tot_off_peak += _net->gen[idx].get_value();
                            }
                            if (h>=7 && h<17) {
                                tot_peak += _net->gen[idx].get_value();
                            }
                            if (h>=17 && h<22) {
                                tot_ev += _net->gen[idx].get_value();
                            }
                            if(h >= 22){
                                tot_off_peak += _net->gen[idx].get_value();
                            }
                        }
//                    }
                    copy_time.tm_hour = (copy_time.tm_hour + 1)%24;
                    idx++;
                }
                copy_time.tm_wday = (copy_time.tm_wday + 1)%7;
                d_idx++;
            }
//            if(y>0){
//            printf("Monthly bill before max charges for %d/%d = %f\n",m+1,1900+copy_time.tm_year,monthly_bill);
                _net->max_kVas_month[12*y+m].set_val(_net->max_kVas_month[12*y+m].get_value()*nb_days);
//            printf("Monthly max charges for %d/%d = %f\n",m+1,1900+copy_time.tm_year,peak_tariff*_net->max_kVas_month[12*y+m].get_value());
                objective += peak_tariff*pow(_net->_price_inflation,y)*(_net->max_kVas_month[12*y+m].get_value());
                monthly_bill += peak_tariff*pow(_net->_price_inflation,y)*(_net->max_kVas_month[12*y+m].get_value());

//                printf("Monthly total demand = %f\n",tot_kw);
//                printf("Monthly peak demand = %f\n",_net->max_kVas_month[12*y+m].get_value());
//                printf("Monthly peak kWh = %f\n",tot_peak);
//                printf("Monthly shoulder kWh = %f\n",tot_ev);
//                printf("Monthly off_peak kWh = %f\n",tot_off_peak);
//            }
            if (y==0 || (y==1 && m<=1)) {
                _net->max_kVas_year[12*y+m].set_val(600000);
//                _net->max_kVas_year[12*y+m].set_val(504112);
            }
            else {
                for (int i = 0; i<13; i++) {
                    _net->max_kVas_year[12*y+m].set_val(max(_net->max_kVas_year[12*y+m].get_value(), _net->max_kVas_month[12*y+m-i].get_value()));
                }
                
            }
//            if(y>0){
//            printf("Monthly yearly max charges for %d/%d = %f\n",m+1,1900+copy_time.tm_year,peak_tariff*_net->max_kVas_year[12*y+m].get_value());
                objective += peak_tariff*pow(_net->_price_inflation,y)*(_net->max_kVas_year[12*y+m].get_value());
            monthly_bill +=peak_tariff*pow(_net->_price_inflation,y)*(_net->max_kVas_year[12*y+m].get_value());
            monthly_bill += _net->_metering_charges * nb_days * _net->_nb_meters; // Metering charges
            monthly_bill += _net->_supply_charges * nb_days * _net->_nb_connect_points; // Supply charge
            
//                printf("Yearly peak demand = %f\n",_net->max_kVas_year[12*y+m].get_value());
//            }
//            printf("Monthly bill for %d/%d = %f\n",m+1,1900+copy_time.tm_year,monthly_bill);
            copy_time.tm_mon = (copy_time.tm_mon + 1)%12;
        }
//        if (tot_PV>0) {
//            printf("Yearly kWh generation for %d = %f\n",1900+copy_time.tm_year,tot_PV);
//        }
        copy_time.tm_year+=1;
//        cout << "Total Energy Charged in the Batteries = " << tot_Batt_Ch << endl;
//        cout << "Total Energy Discharged from the Batteries = " << tot_Batt_Dis << endl;
    }
//    cout << "idx =" << idx << endl;
//    printf("Optimal cost = %f\n",objective);
    _optimal = objective;
    
}

void PowerModel::add_NF_Peak_vars(bool PV) {
    add_NF_vars(PV);
    _net->max_kVas_year.resize(_net->_nb_years);
    for (int y = 0; y<_net->_nb_years; y++) {
        _net->max_kVas_year[y].init("max_kVA_"+to_string(y), 0, 1);
        _model->addVar(_net->max_kVas_year[y]);
        _net->max_kVas_year[y]._print = true;
    }
    
}



void PowerModel::add_AC_Rect_vars_Time_fixed_compact() {
    
    for (auto n:_net->nodes) {
        n->vr_t.resize(_timesteps*_nb_days);
        n->vi_t.resize(_timesteps*_nb_days);
    }
    
    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps*_nb_days);
        g->qg_t.resize(_timesteps*_nb_days);
    }
    
    
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        
        
        for (auto n:_net->nodes) {
            n->vr_t[t].init("vr"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            n->vr_t[t] = n->vs;
            //                n->vr = 1;
            _model->addVar(n->vr_t[t]);
            n->vi_t[t].init("vi"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            _model->addVar(n->vi_t[t]);
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
            g->pg_t[t]._print = true;
            g->qg_t[t]._print = true;
            
        }
        
    }
}


void PowerModel::add_AC_Rect_vars_Time_fixed() {
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps*_nb_days);
        a->pj_t.resize(_timesteps*_nb_days);
        a->qi_t.resize(_timesteps*_nb_days);
        a->qj_t.resize(_timesteps*_nb_days);
        
    }
    
    for (auto n:_net->nodes) {
        n->vr_t.resize(_timesteps*_nb_days);
        n->vi_t.resize(_timesteps*_nb_days);
        if (n->candidate() || n->inst()) {
            n->pv_t.resize(_timesteps*_nb_days);
        }
        
    }
    
    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps*_nb_days);
        g->qg_t.resize(_timesteps*_nb_days);
    }
    
    
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        
        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->qj_t[t]);
            
            a->init_complex();
            
        }
        
        for (auto n:_net->nodes) {
            n->vr_t[t].init("vr"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            n->vr_t[t] = n->vs;
            //                n->vr = 1;
            _model->addVar(n->vr_t[t]);
            n->vi_t[t].init("vi"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            _model->addVar(n->vi_t[t]);
            if(n->candidate() || n->inst()){
                n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
                n->pv_t[t]._print = true;
            }
            
            
            
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
            g->pg_t[t]._print = true;
            g->qg_t[t]._print = true;
            
        }
        
    }
}

void PowerModel::add_AC_Rect_PV_vars_Time(){
    for (auto a:_net->arcs) {  //arc is the edge of the network
        if (a->status==0) {  //if disconnect
            continue;
        }
        a->pi_t.resize(_timesteps*_nb_days);  //active power src, resize: reserve mem for variable
        a->pj_t.resize(_timesteps*_nb_days);  //active power dest
        a->qi_t.resize(_timesteps*_nb_days);  //reactive power src
        a->qj_t.resize(_timesteps*_nb_days);  //reactive power dest

    }

    for (auto n:_net->nodes) {  //nodes in network
        n->vr_t.resize(_timesteps*_nb_days);  //Re(voltage)
        n->vi_t.resize(_timesteps*_nb_days);  //Im(voltage)
        if (n->candidate() || n->inst()) {  //
            n->pv_t.resize(_timesteps*_nb_days);  //Active power from PV pannels (time dependent)
            if (n->candidate()) {  //if this node is a candidate for PV installation
                n->pv_rate.init("pv_rate_node_"+n->_name, 0, n->max_pv_size);  //initiate pv_rate: Size for PV pannels (sq.m). give name to it, give lower bound and upper bound to the function.
                _model->addVar(n->pv_rate);  //add Variable to model
                n->pv_rate._print = true;
            }
        }
    }
    
    for (auto g:_net->gens) {  //generators
        g->pg_t.resize(_timesteps*_nb_days);  //Active power generation variable (time dependent)
        g->qg_t.resize(_timesteps*_nb_days);  //Reactive power generation variable (time dependent)
    }

    for (int t = 0; t < _timesteps*_nb_days; t++) {   //for each time step, initialise variables and add them to model

        for (auto a:_net->arcs) {
            if (a->status==0) {
                continue;
            }
            a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
            _model->addVar(a->qj_t[t]);

        }
    
        for (auto n:_net->nodes) {
            n->vr_t[t].init("vr"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            n->vr_t[t] = n->vs;
            //                n->vr = 1;
            _model->addVar(n->vr_t[t]);
            n->vi_t[t].init("vi"+n->_name+"_" + to_string(t), -n->vbound.max, n->vbound.max);
            _model->addVar(n->vi_t[t]);
            if (n->candidate() || n->inst()) {
                n->pv_t[t].init("pv"+n->_name+"_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
            }
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
            g->pg_t[t]._print = true;
            g->qg_t[t]._print = true;

        }
    }
}


//void PowerModel::add_COPPER_PV_vars_Time(){
//
//    for (auto n:_net->nodes) {  //nodes in network
//        if (n->candidate() || n->inst()) {  //
//            n->pv_t.resize(_timesteps*_nb_days*_nb_years);  //Active power from PV pannels (time dependent)
//        }
//    }
//    
//    for (auto g:_net->gens) {  //generators
//        g->pg_t.resize(_timesteps*_nb_days*_nb_years);  //Active power generation variable (time dependent)
//        g->qg_t.resize(_timesteps*_nb_days*_nb_years);  //Reactive power generation variable (time dependent)
//    }
//    string name;
//    int idx = 0;
//    for (int y = 0; y<_nb_years; y++) {   //for each year
//        for (int d=0; d<_nb_days; d++) {   //for each day
//            for (int t=0; t<_timesteps; t++) {   //for each time step
//                name =  "Y"+to_string(y) +"_D" + to_string(d) + "_T" + to_string(t);
//                idx= y*(_nb_days*_timesteps)+d*(_timesteps)+t;
//                for (auto n:_net->nodes) {
//                    if (n->candidate() || n->inst()) {
//                        n->pv_t[idx].init("pv"+n->_name+"_"+name, 0, 1);
//                        _model->addVar(n->pv_t[idx]);
//                    }
//                }
//                for (auto g:_net->gens) {
//                    g->pg_t[idx].init("pg"+g->_name+":"+g->_bus->_name+"_"+name, 0, g->pbound.max);
//                    g->qg_t[idx].init("qg"+g->_name+":"+g->_bus->_name+"_"+name, g->qbound.min, g->qbound.max);
//                    _model->addVar(g->pg_t[idx]);
//                    _model->addVar(g->qg_t[idx]);
//                    g->pg_t[idx]._print = true;
//                    g->qg_t[idx]._print = true;
//                    
//                }
//            }
//        }
//    }
//}


void PowerModel::add_SOCP_Rect_PV_vars_Time(){
        for (auto a:_net->arcs) {
            if (a->status==0) {
            continue;
            }
            a->pi_t.resize(_timesteps);
            a->pj_t.resize(_timesteps);
            a->qi_t.resize(_timesteps);
            a->qj_t.resize(_timesteps);
            a->wr_t.resize(_timesteps);
            a->wi_t.resize(_timesteps);
        }

        for (auto n:_net->nodes) {
            if(n->candidate() || n->inst()){
                n->pv_t.resize(_timesteps);
                if(n->candidate()){
                    n->pv_rate.init("pv_rate"+n->_name, 0, n->max_pv_size);
                    _model->addVar(n->pv_rate);
                }
            }
            n-> w_t.resize(_timesteps);
        }

        for (auto g:_net->gens) {
            g->pg_t.resize(_timesteps);
            g->qg_t.resize(_timesteps);
        }


        for (int t = 0; t < _timesteps; t++) {
            for (auto a:_net->arcs) {
                if (a->status==0) {
                    continue;
                }
                a->pi_t[t].init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
                _model->addVar(a->pi_t[t]);
                a->pj_t[t].init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
                _model->addVar(a->pj_t[t]);
                a->qi_t[t].init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")_" + to_string(t));
                _model->addVar(a->qi_t[t]);
                a->qj_t[t].init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")_" + to_string(t));
                _model->addVar(a->qj_t[t]);
                a->wr_t[t].init("wr("+a->_name, a->src->vbound.min*a->dest->vbound.min, a->src->vbound.max*a->dest->vbound.max);
                a->wr_t[t] = 1;
                _model->addVar(a->wr_t[t]);
                a->wi_t[t].init("wi("+a->_name, -a->src->vbound.max*a->dest->vbound.max, a->src->vbound.max*a->dest->vbound.max);
                _model->addVar(a->wi_t[t]);
                a->init_complex();
                }

            for (auto n:_net->nodes) {

                if(n->candidate() || n->inst()){
                    n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                    _model->addVar(n->pv_t[t]);
                }

                n->w_t[t].init("w" + n->_name + "_" + to_string(t), pow(n->vbound.min, 2), pow(n->vbound.max, 2));
                n->w_t[t] = pow(n->vs, 2);
                _model->addVar(n->w_t[t]);

                n->init_lifted_complex();
            }
            for (auto g:_net->gens) {
                g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
                g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
                _model->addVar(g->pg_t[t]);
                _model->addVar(g->qg_t[t]);
            }
        }
}

void PowerModel::add_AC_Rect_PV_vars(){
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->pi);
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pj);
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->qi);
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->qj);
        a->init_complex();
    }
    for (auto n:_net->nodes) {
        n->vr.init("vr"+n->_name, -n->vbound.max, n->vbound.max);
        n->vr = n->vs;
//                n->vr = 1;
        _model->addVar(n->vr);
        n->vi.init("vi"+n->_name, -n->vbound.max, n->vbound.max);
        _model->addVar(n->vi);
        n->init_complex(false);
        
        n->pv.init("pv"+n->_name, 0, 1000000);
        _model->addVar(n->pv);
        
    }
    add_AC_gen_vars();
}

void PowerModel::add_AC_Rect_vars(){
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->pi);
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pj);
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->qi);
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->qj);
        a->init_complex();
    }
    for (auto n:_net->nodes) {
        n->vr.init("vr"+n->_name, -n->vbound.max, n->vbound.max);
        n->vr = n->vs;
        //                n->vr = 1;
        _model->addVar(n->vr);
        n->vi.init("vi"+n->_name, -n->vbound.max, n->vbound.max);
        _model->addVar(n->vi);
        n->init_complex(false);
        
//        n->pv.init("pv"+n->_name, n->pvbound.min, n->pvbound.max);
//        n->pv.set_lb(0);
//        n->pv.set_ub(100);
//        _model->addVar(n->pv);
        
    }
    add_AC_gen_vars();
}

void PowerModel::add_QC_vars(){
    double l,u;
    for (auto n:_net->nodes) {
        n->theta.init("t"+n->_name);
        n->v.init("v"+n->_name, n->vbound.min, n->vbound.max);
        n->v = n->vs;
        n->w.init("w"+n->_name,pow(n->vbound.min,2), pow(n->vbound.max,2));
        n->w = n->vs*n->vs;
        //        n->v = 1;
        _model->addVar(n->v);
        _model->addVar(n->theta);
        _model->addVar(n->w);
        n->init_lifted_complex();
    }
    add_AC_gen_vars();
    
    for (auto a:_net->arcs) { // vivjcos, vivjsin, vivj, cos, sin, c
        if (a->status==0) {
//            assert(false);
            continue;
        }
        l = a->tbound.min - a->as;
        u = a->tbound.max - a->as;
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pi);
        _model->addVar(a->pj);
        _model->addVar(a->qi);
        _model->addVar(a->qj);
        if (l < 0 && u > 0)
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(l), cos(u)), 1.);
        else
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(l), cos(u)), max(cos(l),cos(u)));
        a->vv.init("vv("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->src->vbound.min*a->dest->vbound.min,a->src->vbound.max*a->dest->vbound.max);
        a->vcs.init("vcs("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_lb()*a->cs.get_lb(), a->vv.get_ub()*a->cs.get_ub());
        if(l < 0 && u > 0)
            a->vsn.init("vsn("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_ub()*sin(l), a->vv.get_ub()*sin(u));
        if (l >= 0)
            a->vsn.init("vsn("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_lb()*sin(l), a->vv.get_ub()*sin(u));
        if (u <= 0)
            a->vsn.init("vsn("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_ub()*sin(l), a->vv.get_lb()*sin(u));
        a->sn.init("sn("+a->_name+","+a->src->_name+","+a->dest->_name+")",sin(l), sin(u));
        a->ci.init("ci("+a->_name+","+a->src->_name+","+a->dest->_name+")", 0.0, INFINITY);
        a->delta.init("delta("+a->_name+","+a->src->_name+","+a->dest->_name+")",l,u);
        _model->addVar(a->vcs);
        a->vcs = 1;
        _model->addVar(a->vsn);
        _model->addVar(a->vv);
        _model->addVar(a->cs);
        _model->addVar(a->sn);
        _model->addVar(a->ci);
        _model->addVar(a->delta);
        a->init_complex();
    }
    
}

void PowerModel::add_AC_OTS_vars() {
    add_AC_gen_vars();
    for (auto n:_net->nodes) { //
        n->theta.init("t"+n->_name);
        n->v.init("v"+n->_name, n->vbound.min, n->vbound.max);
        //n->v = n->vs;
        n->v = 1;
        _model->addVar(n->v);
        _model->addVar(n->theta);
        n->init_complex(true);
    }
    for (auto a:_net->arcs) {
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->on.init("on("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pi);
        _model->addVar(a->pj);
        _model->addVar(a->qi);
        _model->addVar(a->qj);
        _model->addVar(a->on);
        a->on=1;
        a->init_complex();
    }
}

void PowerModel::add_AC_SOCP_vars_Time() {

    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi_t.resize(_timesteps);
        a->pj_t.resize(_timesteps);
        a->qi_t.resize(_timesteps);
        a->qj_t.resize(_timesteps);
        a->wr_t.resize(_timesteps);
        a->wi_t.resize(_timesteps);
    }

    for (auto n:_net->nodes) {
        if(n->inst()){
            n->pv_t.resize(_timesteps);
        }
        n-> w_t.resize(_timesteps);
    }

    for (auto g:_net->gens) {
        g->pg_t.resize(_timesteps);
        g->qg_t.resize(_timesteps);
    }
    for (int t = 0; t < _timesteps; t++) {
        for (auto a:_net->arcs) {
            if (a->status == 0) {
                continue;
            }
            a->pi_t[t].init("p(" + a->_name + "," + a->src->_name + "," + a->dest->_name + ")");
            _model->addVar(a->pi_t[t]);
            a->pj_t[t].init("p(" + a->_name + "," + a->dest->_name + "," + a->src->_name + ")");
            _model->addVar(a->pj_t[t]);
            a->qi_t[t].init("q(" + a->_name + "," + a->src->_name + "," + a->dest->_name + ")");
            _model->addVar(a->qi_t[t]);
            a->qj_t[t].init("q(" + a->_name + "," + a->dest->_name + "," + a->src->_name + ")");
            _model->addVar(a->qj_t[t]);
            a->wr_t[t].init("wr" + a->_name, a->src->vbound.min * a->dest->vbound.min,
                       a->src->vbound.max * a->dest->vbound.max);
            a->wr_t[t] = 1;
            _model->addVar(a->wr_t[t]);
            a->wi_t[t].init("wi" + a->_name, -a->src->vbound.max * a->dest->vbound.max,
                       a->src->vbound.max * a->dest->vbound.max);

            _model->addVar(a->wi_t[t]);
            a->init_complex();

        }
        for (auto n:_net->nodes) {
            n->w_t[t].init("w" + n->_name, pow(n->vbound.min, 2), pow(n->vbound.max, 2));
            n->w_t[t] = 1;
            _model->addVar(n->w_t[t]);
            if(n->inst()){
                n->pv_t[t].init("pv" + n->_name + "_" + to_string(t), 0, 1);
                _model->addVar(n->pv_t[t]);
            }
            n->init_lifted_complex();
        }
        for (auto g:_net->gens) {
            g->pg_t[t].init("pg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), 0, g->pbound.max);
            g->qg_t[t].init("qg"+g->_name+":"+g->_bus->_name+")_" + to_string(t), g->qbound.min, g->qbound.max);
            _model->addVar(g->pg_t[t]);
            _model->addVar(g->qg_t[t]);
        }
    }
}

void PowerModel::add_AC_SOCP_vars(){
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->pi);
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pj);
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->qi);
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->qj);
        a->wr.init("wr"+a->_name, a->src->vbound.min*a->dest->vbound.min*cos(a->tbound.min), a->src->vbound.max*a->dest->vbound.max);
        a->wr = 1;
        _model->addVar(a->wr);
        a->wi.init("wi"+a->_name, -a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max), a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max));
        _model->addVar(a->wi);
        a->init_complex();

    }
    for (auto n:_net->nodes) {
        n->w.init("w"+n->_name, pow(n->vbound.min,2), pow(n->vbound.max,2));
        n->w = pow(n->vs,2);
        _model->addVar(n->w);
        n->init_lifted_complex();
    }
    add_AC_gen_vars();
}

void PowerModel::add_DC_vars(){
    for (auto n:_net->nodes) {
        n->theta.init("t"+n->_name);
        _model->addVar(n->theta);

    }
    for (auto g:_net->gens) {
        if (!g->_active)
            continue;
        g->pg.init("pg"+g->_bus->_name, g->pbound.min, g->pbound.max);
        _model->addVar(g->pg);
    }
    for (auto a:_net->arcs) {
        if (a->status==0)
            continue;
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pi);
        _model->addVar(a->pj);
    }
}

void PowerModel::add_QC_OTS_vars() {
    double m_theta_ub = 0, m_theta_lb = 0, l, u;
    for (auto a:_net->arcs) {
        m_theta_ub += a->tbound.max - a->as;
        m_theta_lb += a->tbound.min - a->as;
    }
    _net->m_theta_ub = m_theta_ub;
    _net->m_theta_lb = m_theta_lb;
    add_AC_gen_vars();
    for (auto n:_net->nodes) {
        n->v.init("v"+n->_name, n->vbound.min, n->vbound.max);
        n->v = 1;
        n->theta.init("t"+n->_name);
        n->w.init("w"+n->_name, pow(n->vbound.min,2), pow(n->vbound.max,2));
        _model->addVar(n->v);
        //n->w = pow(n->vs,2);
        n->w = 1.001;
        _model->addVar(n->w);
        n->theta = 0;
        _model->addVar(n->theta);
        n->init_lifted_complex();
    }

    for (auto a:_net->arcs) {
        if (a->status == 0) continue;
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->on.init("on("+a->_name+","+a->dest->_name+","+a->src->_name+")");

        l = a->tbound.min - a->as;
        u = a->tbound.max - a->as;

        a->vv.init("vv("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->src->vbound.min*a->dest->vbound.min,a->src->vbound.max*a->dest->vbound.max,0,0);
        if (l < 0 && u > 0)
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(l),cos(u)),1,0,0);
        else
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(l),cos(u)),max(cos(l),cos(u)),0,0);
        a->wr.init("wr"+a->_name, a->vv.get_lb()*a->cs.get_lb(), a->vv.get_ub()*a->cs.get_ub(),0,0);
        if(l < 0 && u > 0)
            a->wi.init("wi"+a->_name, a->vv.get_ub()*sin(l), a->vv.get_ub()*sin(u),0,0);
        if (l >= 0)
            a->wi.init("wi"+a->_name, a->vv.get_lb()*sin(l), a->vv.get_ub()*sin(u),0,0);
        if (u <= 0)
            a->wi.init("wi"+a->_name, a->vv.get_ub()*sin(l), a->vv.get_lb()*sin(u),0,0);
        a->w_line_ij.init("w_line("+a->_name+","+a->src->_name+","+a->dest->_name+")", pow(a->src->vbound.min,2), pow(a->src->vbound.max,2),0,0);
        a->w_line_ji.init("w_line("+a->_name+","+a->dest->_name+","+a->src->_name+")", pow(a->dest->vbound.min,2), pow(a->dest->vbound.max,2),0,0);

        a->sn.init("sn("+a->_name+","+a->src->_name+","+a->dest->_name+")",sin(l), sin(u),0,0);
        a->ci.init("ci("+a->_name+","+a->src->_name+","+a->dest->_name+")", 0, INFINITY);
        a->delta.init("delta("+a->_name+","+a->src->_name+","+a->dest->_name+")", l, u, _net->m_theta_lb, _net->m_theta_ub);
        _model->addVar(a->pi);
        _model->addVar(a->pj);
        _model->addVar(a->qi);
        _model->addVar(a->qj);
        a->on = 1;
        _model->addVar(a->on);
        a->wr = 1;
        _model->addVar(a->wr);
        a->wi = 0;
        _model->addVar(a->wi);
        _model->addVar(a->w_line_ij);
        _model->addVar(a->w_line_ji);
        _model->addVar(a->vv);
        _model->addVar(a->cs);
        _model->addVar(a->sn);
        _model->addVar(a->ci);
        _model->addVar(a->delta);
        a->init_complex();
    }
}


void PowerModel::add_SOCP_OTS_vars() {
    for (auto a:_net->arcs) {
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->pi);
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pj);
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        _model->addVar(a->qi);
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->qj);
        a->wr.init("wr"+a->_name);
        _model->addVar(a->wr);
        a->wi.init("wi"+a->_name);
        _model->addVar(a->wi);
        a->on.init("on("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->on = true;
        _model->addVar(a->on);
        a->w_line_ij.init("w_line("+a->_name+","+a->src->_name+","+a->dest->_name+")", 0.0, INFINITY);
        _model->addVar(a->w_line_ij);
        a->w_line_ji.init("w_line("+a->_name+","+a->dest->_name+","+a->src->_name+")", 0.0, INFINITY);
        _model->addVar(a->w_line_ji);
        a->init_complex();
    }
    for (auto n:_net->nodes) {
        n->w.init("w"+n->_name, pow(n->vbound.min,2), pow(n->vbound.max,2));
        n->w = pow(n->vs,2);
        _model->addVar(n->w);
        n->init_lifted_complex();
    }
    add_AC_gen_vars();
}


//#subject to Theta_Delta_UB {(l,i,j) in arcs_from}: wi[l] <= tan(ad_max[l])*wr[l];
//#subject to Theta_Delta_LB {(l,i,j) in arcs_from}: wi[l] >= tan(ad_min[l])*wr[l];
void PowerModel::add_Wr_Wi(Arc *a){
    if (a->status==1 && !a->parallel) {
        Constraint Wr_Wi_l("Theta_Delta_UB");
        Wr_Wi_l += a->wi;
        Wr_Wi_l -= tan(a->tbound.max)*a->wr;
        Wr_Wi_l <= 0;
        _model->addConstraint(Wr_Wi_l);
        Constraint Wr_Wi_u("Theta_Delta_UB");
        Wr_Wi_u += a->wi;
        Wr_Wi_u -= tan(a->tbound.min)*a->wr;
        Wr_Wi_u >= 0;
        _model->addConstraint(Wr_Wi_u);        
    }
}

void PowerModel::add_SOCP_Angle_Bounds_Time(Arc *a){
    for (int t=0; t<_timesteps; t++){
        if (a->status == 1 && !a->parallel) {
       /*     Constraint Wr_Wi_l("Theta_Delta_LB");
            Wr_Wi_l += a->wi_t[t];
            Wr_Wi_l -= tan(a->tbound.max) * a->wr_t[t];
            Wr_Wi_l <= 0;
            _model->addConstraint(Wr_Wi_l);
            Constraint Wr_Wi_u("Theta_Delta_UB");
            Wr_Wi_u += a->wi_t[t];
            Wr_Wi_u -= tan(a->tbound.min) * a->wr_t[t];
            Wr_Wi_u >= 0;
            _model->addConstraint(Wr_Wi_u);*/

            Constraint Wr_Wi_l("Theta_Delta_LB");
            Wr_Wi_l += a->wi_t[t];
            Wr_Wi_l -= tan(-a->tbound.max) * a->wr_t[t];
            Wr_Wi_l >= 0;
            _model->addConstraint(Wr_Wi_l);
            Constraint Wr_Wi_u("Theta_Delta_UB");
            Wr_Wi_u += a->wi_t[t];
            Wr_Wi_u -= tan(a->tbound.max) * a->wr_t[t];
            Wr_Wi_u <= 0;
            _model->addConstraint(Wr_Wi_u);   //original of both versions found above, changed according to (40) on paper.
        }
    }
}

void PowerModel::add_AC_Angle_Bounds_Time(Arc *a){
    for (int t=0; t<_timesteps*_nb_days; t++){
        if (a->status == 1 && !a->parallel) {
            /*     Constraint Wr_Wi_l("Theta_Delta_LB");
             Wr_Wi_l += a->wi_t[t];
             Wr_Wi_l -= tan(a->tbound.max) * a->wr_t[t];
             Wr_Wi_l <= 0;
             _model->addConstraint(Wr_Wi_l);
             Constraint Wr_Wi_u("Theta_Delta_UB");
             Wr_Wi_u += a->wi_t[t];
             Wr_Wi_u -= tan(a->tbound.min) * a->wr_t[t];
             Wr_Wi_u >= 0;
             _model->addConstraint(Wr_Wi_u);*/
            
            Constraint Th_lb("Theta_Delta_LB");  //k = Vi*(Vj*)
            Th_lb += a->src->vi_t[t]*a->dest->vr_t[t] - a->src->vr_t[t]*a->dest->vi_t[t];// Im(k)
            Th_lb -= tan(-a->tbound.max) * (a->src->vr_t[t]*a->dest->vr_t[t] + a->src->vi_t[t]*a->dest->vi_t[t]);// tan(-PAD)*maxRe(k)
            Th_lb >= 0;  //why?? compared with upper bound
            _model->addConstraint(Th_lb);
            Constraint Th_ub("Theta_Delta_UB");
            Th_ub += a->src->vi_t[t]*a->dest->vr_t[t] - a->src->vr_t[t]*a->dest->vi_t[t];
            Th_ub -= tan(a->tbound.max) * (a->src->vr_t[t]*a->dest->vr_t[t] + a->src->vi_t[t]*a->dest->vi_t[t]);
            Th_ub <= 0;
            _model->addConstraint(Th_ub);   //original of both versions found above, changed according to (40) on paper.
        }
    }
}



void PowerModel::add_AC_thermal_Time(Arc* a){
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to Thermal_Limit {(l,i,j) in arcs}: p[l,i,j]^2 + q[l,i,j]^2 <= s[l]^2;*/
        
        if (a->status==1 && !a->dest->_has_gen && !a->src->_has_gen) {
            Constraint Thermal_Limit_from("Thermal_Limit_from_T_" + to_string(t));
            Thermal_Limit_from += ((a->pi_t[t])^2) + ((a->qi_t[t])^2);
            if(pow(a->limit,2.)>0.004){
                Thermal_Limit_from <= pow(a->limit,2.);
//                Thermal_Limit_from.print();
            }
            else{
                Thermal_Limit_from <= 0.03;
            }
            _model->addConstraint(Thermal_Limit_from);
            
            Constraint Thermal_Limit_to("Thermal_Limit_to_T_" + to_string(t));
            Thermal_Limit_to += ((a->pj_t[t])^2) + ((a->qj_t[t])^2);
            if(pow(a->limit,2.)>0.004){
                Thermal_Limit_to <= pow(a->limit,2.);
            }
            else{
                Thermal_Limit_to <= 0.03;
            }            
            _model->addConstraint(Thermal_Limit_to);
        }
    }
}


void PowerModel::add_AC_thermal(Arc* a, bool switch_lines){
/** subject to Thermal_Limit {(l,i,j) in arcs}: p[l,i,j]^2 + q[l,i,j]^2 <= s[l]^2;*/
    if (a->status==1 || switch_lines) {
        Constraint Thermal_Limit_from("Thermal_Limit_from");
        Thermal_Limit_from += a->_Si_.square_magnitude();
        if (!switch_lines){
            if(pow(a->limit,2.)>0.004){
                Thermal_Limit_from <= pow(a->limit,2.);
            }
            else{
                Thermal_Limit_from <= 0.03;
            }
        }
        else{
            Thermal_Limit_from -= pow(a->limit,2.)*a->on + (1-a->on)*a->smax;
            Thermal_Limit_from <= 0;
        }
        _model->addConstraint(Thermal_Limit_from);
        Constraint Thermal_Limit_to("Thermal_Limit_to");
        Thermal_Limit_to += a->_Sj_.square_magnitude();
        if (!switch_lines){
            if(pow(a->limit,2.)>0.004){
                Thermal_Limit_to <= pow(a->limit,2.);
            }
            else{
                Thermal_Limit_to <= 0.03;
            }

        }
        else{
            Thermal_Limit_to -= pow(a->limit,2.)*a->on + (1-a->on)*a->smax;
            Thermal_Limit_to <= 0;
        }
        _model->addConstraint(Thermal_Limit_to);
    }
}


void PowerModel::add_AC_Angle_Bounds(Arc* a, bool switch_lines){
/** Adding phase angle difference bound constraint
 subject to Theta_Delta_LB{(l,i,j) in arcs_from}: t[i]-t[j] >= -theta_bound;
 subject to Theta_Delta_UB{(l,i,j) in arcs_from}: t[i]-t[j] <= theta_bound;
 */
    if (a->status==1 || switch_lines) {
        Node* src = a->src;
        Node* dest = a->dest;
        Constraint Theta_Delta_UB("Theta_Delta_UB");
        Theta_Delta_UB += (src->theta - dest->theta);
        if (!switch_lines){
            Theta_Delta_UB <= a->tbound.max;
        }
        else{
            Theta_Delta_UB -= a->tbound.max*a->on + (1-a->on)*_net->m_theta_ub;
            Theta_Delta_UB <= 0;
        }
        _model->addConstraint(Theta_Delta_UB);
        Constraint Theta_Delta_LB("Theta_Delta_LB");
        Theta_Delta_LB += src->theta - dest->theta;
        if (!switch_lines){
            Theta_Delta_LB >= a->tbound.min;
        }
        else{
            Theta_Delta_LB -= a->tbound.min*a->on + (1-a->on)*_net->m_theta_lb;
            Theta_Delta_LB >= 0;
        }
        _model->addConstraint(Theta_Delta_LB);
    }
}


void PowerModel::add_AC_Power_Flow_Time(Arc *a){
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        if (a->status==1) { //if nodes are connected
            /** subject to Flow_P_From {(l,i,j) in arcs_from}:
             p[l,i,j] = g[l]*(v[i]/tr[l])^2
             + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
             + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
             */
            Node* src = a->src;
            Node* dest = a->dest;
            Constraint Flow_P_From("Flow_P_From"+a->pi._name + "_" + to_string(t));
            Flow_P_From += a->pi_t[t];
            Flow_P_From -= a->g*((((src->vi_t[t])^2) + ((src->vr_t[t])^2)))/pow(a->tr,2.);
            Flow_P_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vr_t[t]*dest->vr_t[t] + src->vi_t[t]*dest->vi_t[t]);
            Flow_P_From -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi_t[t]*dest->vr_t[t] - src->vr_t[t]*dest->vi_t[t]);
            Flow_P_From = 0;
            _model->addConstraint(Flow_P_From);
            /** subject to Flow_P_To {(l,i,j) in arcs_to}:
             p[l,i,j] = g[l]*v[i]^2
             + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
             + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
             */
            Constraint Flow_P_To("Flow_P_To"+a->pj._name + "_" + to_string(t));
            Flow_P_To += a->pj_t[t];
            Flow_P_To -= a->g*((((dest->vi_t[t])^2) + ((dest->vr_t[t])^2)));
            Flow_P_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
            Flow_P_To -= (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi_t[t]*src->vr_t[t] - dest->vr_t[t]*src->vi_t[t]);
            Flow_P_To = 0;
            _model->addConstraint(Flow_P_To);
            /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
             q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
             +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
             + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
             */
            Constraint Flow_Q_From("Flow_Q_From"+a->qi._name+ "_" + to_string(t));
            Flow_Q_From += a->qi_t[t];
            Flow_Q_From += (a->ch/2+a->b)*((((src->vi_t[t])^2) + ((src->vr_t[t])^2)))/pow(a->tr,2.);
            Flow_Q_From += (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
            Flow_Q_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi_t[t]*dest->vr_t[t] - src->vr_t[t]*dest->vi_t[t]);
            Flow_Q_From = 0;
            _model->addConstraint(Flow_Q_From);
            /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
             q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
             +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
             + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
             */
            Constraint Flow_Q_To("Flow_Q_To"+a->qj._name+ "_" + to_string(t));



            

            Flow_Q_To += a->qj_t[t];
            Flow_Q_To += (a->ch/2+a->b)*(((dest->vi_t[t])^2) + ((dest->vr_t[t])^2));
            Flow_Q_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
            Flow_Q_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi_t[t]*src->vr_t[t] - dest->vr_t[t]*src->vi_t[t]);
            Flow_Q_To = 0;
            _model->addConstraint(Flow_Q_To);



        }
    }

}

void PowerModel::add_AC_Power_Flow(Arc *a, bool polar){
    
    
    //    Flow_P_From += a->pi;
    //    Flow_P_From -= constant(a->g/pow(a->tr,2.))*(src->_V_.square_magnitude());
    //    Flow_P_From += constant(a->g/a->tr)*((src->v)*(dest->v)*cos(src->theta - dest->theta - a->as));
    //    Flow_P_From += a->b/a->tr*((src->v)*(dest->v)*sin(src->theta - dest->theta - a->as));
    //    Flow_P_From = 0;
    
    if (a->status==1) {
        /** subject to Flow_P_From {(l,i,j) in arcs_from}:
         p[l,i,j] = g[l]*(v[i]/tr[l])^2
         + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Node* src = a->src;
        Node* dest = a->dest;
        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;
        Flow_P_From -= a->g*(src->_V_.square_magnitude())/pow(a->tr,2.);
        if (polar) {
            Flow_P_From += a->g/a->tr*((src->v)*(dest->v)*cos(src->theta - dest->theta - a->as)); /** TODO write the constraints in Complex form */
            Flow_P_From += a->b/a->tr*((src->v)*(dest->v)*sin(src->theta - dest->theta - a->as));
        }
        else{
            Flow_P_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vr*dest->vr + src->vi*dest->vi);
            Flow_P_From -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi*dest->vr - src->vr*dest->vi);
        }
        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);
        /** subject to Flow_P_To {(l,i,j) in arcs_to}:
         p[l,i,j] = g[l]*v[i]^2
         + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To -= a->g*(dest->_V_.square_magnitude());
        if (polar) {
            Flow_P_To += a->g/a->tr*(dest->v*src->v*cos(dest->theta - src->theta + a->as));
            Flow_P_To += a->b/a->tr*(dest->v*src->v*sin(dest->theta - src->theta + a->as));
        }
        else {
            Flow_P_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr*src->vr + dest->vi*src->vi);
            Flow_P_To -= (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi*src->vr - dest->vr*src->vi);
        }
        Flow_P_To = 0;
        _model->addConstraint(Flow_P_To);
        /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
         q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
         +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Constraint Flow_Q_From("Flow_Q_From"+a->qi._name);
        Flow_Q_From += a->qi;
        Flow_Q_From += (a->ch/2+a->b)*(src->_V_.square_magnitude())/pow(a->tr,2.);
        if (polar) {
            Flow_Q_From -= a->b/a->tr*(src->v*dest->v*cos(src->theta - dest->theta - a->as));
            Flow_Q_From += a->g/a->tr*(src->v*dest->v*sin(src->theta - dest->theta - a->as));
        }
        else {
            Flow_Q_From += (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr*src->vr + dest->vi*src->vi);
            Flow_Q_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi*dest->vr - src->vr*dest->vi);
        }
        Flow_Q_From = 0;
        _model->addConstraint(Flow_Q_From);
        /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
         q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
         +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_Q_To("Flow_Q_To"+a->qj._name);
        Flow_Q_To += a->qj;
        Flow_Q_To += (a->ch/2+a->b)*(dest->_V_.square_magnitude());
        if (polar) {
            Flow_Q_To -= a->b/a->tr*(dest->v*src->v*cos(dest->theta - src->theta + a->as));
            Flow_Q_To += a->g/a->tr*(dest->v*src->v*sin(dest->theta - src->theta + a->as));
        }
        else{
            Flow_Q_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr*src->vr + dest->vi*src->vi);
            Flow_Q_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi*src->vr - dest->vr*src->vi);
        }
        Flow_Q_To = 0;
        _model->addConstraint(Flow_Q_To);
    }
    
}

void PowerModel::add_AC_link_Batt_Time(Node*n){


    //with Gen
    Constraint SOC_Batt_init("Initial State of Charge");
//    SOC_Batt_init += n->soc_t[0] - n->batt_cap;
    SOC_Batt_init += n->soc_t[0];
    SOC_Batt_init = 0;
    _model->addConstraint(SOC_Batt_init);
    
    
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        
/*
        Constraint Link_Batt("Link_Battery"+n->_name + "_" + to_string(t));                         //soc[t+1]=soc[t]+pch-pdis
        Link_Batt += n->soc_t[t+1]-n->soc_t[t] - 0.85 * n->pch_t[t] + n->pdis_t[t];                 //charge efficiency=85%
        Link_Batt = 0;
        _model->addConstraint(Link_Batt);*/
        //new addition
        Constraint Limit_to_charging("Limit_to_charging"+n->_name+"_"+ to_string(t));
        Limit_to_charging += n->soc_t[t];
        Limit_to_charging -= n->batt_cap;
        Limit_to_charging <=0;
        _model->addConstraint(Limit_to_charging);
        //end of new addition
        
       //Avoid charging and discharging simultaneously
        /*Constraint Avoid_simul_charging_and_discharging("Avoid_simul_charging_and_discharging"+n->_name+"_"+ to_string(t));
        Avoid_simul_charging_and_discharging += n->pch_t[t]*n->pdis_t[t];
        Avoid_simul_charging_and_discharging = 0;
        Avoid_simul_charging_and_discharging.print();
        _model->addConstraint(Avoid_simul_charging_and_discharging);*/


/*        Constraint Limit_to_charging("Limit_to_charging"+n->_name+"_"+ to_string(t));  //setting batt_cap=0 (Testing)
        Limit_to_charging += n->soc_t[t];
        Limit_to_charging += n->batt_cap;
        Limit_to_charging =0;
        _model->addConstraint(Limit_to_charging);*/

    }
    
    for (int t = 0; t < _timesteps*_nb_days-1; t++){ //change timestep back to _timesteps-1 with new addition

        //new addition
        Constraint Link_Batt("Link_Battery"+n->_name + "_" + to_string(t));
        Link_Batt += n->soc_t[t+1];
        Link_Batt -= n->soc_t[t] ;
        Link_Batt -= (0.80*n->pch_t[t]);
        Link_Batt += (0.85*n->pdis_t[t]);
        Link_Batt = 0;
        _model->addConstraint(Link_Batt);
        //end of new addition
/*        Constraint Discharging_Batt("Disharging_Batt"+n->_name + "_" + to_string(t));               //pdis<=80%soc   depth of discharge=80%
        Discharging_Batt += n->pdis_t[t] - 0.8 * n->soc_t[t];
        Discharging_Batt <= 0;
        _model->addConstraint(Discharging_Batt);*/

      /*  Constraint ChargingState_Batt("ChargingState_Batt"+n->_name + "_" + to_string(t));          //either pch or pdis must be zero at one timestep
        ChargingState_Batt += (n->pdis_t[t])*(n->pch_t[t]);
        ChargingState_Batt = 0;
        _model->addConstraint(ChargingState_Batt);*/

        //new addition
        Constraint Change_of_charge_limit("Maximum_Charging"+n->_name+"_"+to_string(t));
        Change_of_charge_limit += n->pch_t[t+1] - n->pch_t[t] - (0.5*24/_timesteps)*n->batt_cap;
        //cout<<"timestep is "<< _timesteps<<" ; "<<endl;
        Change_of_charge_limit <= 0;
        _model->addConstraint(Change_of_charge_limit);

        Constraint Change_of_discharge_limit("Maximum_Discharging"+n->_name+"_"+to_string(t));
        Change_of_discharge_limit += n->pdis_t[t+1] - n->pdis_t[t] - (0.5*24/_timesteps)*n->batt_cap; // limit of rate, assumed as 5% of battery cap
        Change_of_discharge_limit <= 0;
        _model->addConstraint(Change_of_discharge_limit);
        //end of new addition

      /*  Constraint Charging_Batt("Charging_Batt"+n->_name + "_" + to_string(t));                   //pch+soc<=batt_cap
        Charging_Batt += n->soc_t[t] + 0.85 * n->pch_t[t] - n->batt_cap;
        Charging_Batt <= 0;
        _model->addConstraint(Charging_Batt);*/



    }
    //new  addition
        Constraint Last_step_ch("Last_step"+n->_name);
        Last_step_ch += n->pch_t[_timesteps*_nb_days-1];
        Last_step_ch = 0;
        _model->addConstraint(Last_step_ch);

        Constraint Last_step_dis("Last_step"+n->_name);
        Last_step_dis += n->pdis_t[_timesteps*_nb_days-1];
        Last_step_dis = 0;
        _model->addConstraint(Last_step_dis);
    //end of new addition
}



void PowerModel::add_link_PV_Rate(Node*n){
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        Constraint Link_PV_Rate("Link_PV_Rate" + n->_name + "_" + to_string(t));
        Link_PV_Rate += n->pv_t[t];
        Link_PV_Rate -= (_net->_radiation[t]) * (n->pv_rate) * (0.86/ (1000 * _net->bMVA));
//        cout << "t = " << t << " and t mod 24 = " << t%24 << endl;
        Link_PV_Rate = 0;
        _model->addConstraint(Link_PV_Rate);


//        Constraint PV_bound("PV_bound" + n->_name);
//        PV_bound += n->pv_rate;
//        PV_bound <= 100.0; //100 m^2 limit.
//        _model->addConstraint(PV_bound);
    }

}

void PowerModel::add_link_PV_fixed_Rate(Node*n){
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        Constraint Link_PV_Rate("Link_PV_Rate" + n->_name + "_" + to_string(t));
        Link_PV_Rate += n->pv_t[t];
        Link_PV_Rate -= (_net->_radiation[t]) * (n->max_pv_size) * (0.86/ (1000 * _net->bMVA));
//        cout << "t = " << t << " and t mod 24 = " << t%24 << endl;
        Link_PV_Rate = 0;
        _model->addConstraint(Link_PV_Rate);
        
        
        //        Constraint PV_bound("PV_bound" + n->_name);
        //        PV_bound += n->pv_rate;
        //        PV_bound <= 100.0; //100 m^2 limit.
        //        _model->addConstraint(PV_bound);
    }
    
}       

void PowerModel::add_AC_KCL_PV_Batt_Time(Node* n){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) { // for each time step
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }
        
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        
        
        
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));  //Active Power. name: Bus identifier
        KCL_P += sum_Time(n->get_out(), "pi", t);  //add output power of arc?????
        KCL_P += sum_Time(n->get_in(), "pj", t);  //add input power of arc????
        KCL_P -= sum_Time(n->_gen, "pg", t); //subtract generated power, remember to add to models with gen???? where and how????
        if(n->candidate() || n->inst()){  //if this node is a candidate for PV installation or if PV is already installed here
            KCL_P -= n->pv_t[t];  //subtract Active power from PV pannels (time dependent)
            if(n->candidate()){  //if this node is a candidate for PV installation
                KCL_P -= n->pdis_t[t];   //active power discharging value                                   //discharge efficiency=70%
                KCL_P += n->pch_t[t];   //active power charging value
            }
        }
        KCL_P += n->gs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) + n->pl(t);//gs:Returns the real part of the bus shunt. pl:Returns the active power load at this bus at given time step. gs*|W|^2+pl, sum for all time steps
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P); //add constraints to model(number of variables?)?????
        
        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));  //Reactive Power
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        //cout<<"n gen is "<<n->_gen[0]<<endl;
        KCL_Q -= n->bs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);  //bs:Returns the real part of the bus shunt. ql:Returns the reactive power load at this bus at given time step. bs*|W|^2+ql, sum for all time steps.
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }
    
}

void PowerModel::add_AC_KCL_Batt_Time(Node* n){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps; t++) {

        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */

        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }

        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t); //remember to add to models with gen
        if (n->candidate()) {
            KCL_P -= n->pdis_t[t];                                               //discharge efficiency=70%
            KCL_P += n->pch_t[t];
        }
        if(n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) + n->pl(t);
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}
void PowerModel::add_SOCP_KCL_Batt_Time(Node* n){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps; t++) {

        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }
        


        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if (n->candidate()) {
            KCL_P -= n->pdis_t[t];                                               //discharge efficiency=70%
            KCL_P += n->pch_t[t];
        }
        if(n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs()*((n->w_t[t])^2) + n->pl(t);
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*((n->w_t[t])^2) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}


void PowerModel::add_SOCP_KCL_PV_Batt_Time(Node* n){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps; t++) {

        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }

        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->candidate() || n->inst()){
            KCL_P -= n->pv_t[t];
            if(n->candidate()){
                KCL_P -= n->pdis_t[t];                                               //discharge efficiency=70%
                KCL_P += n->pch_t[t];
            }
        }
        KCL_P += n->gs()*(n->w_t[t]) + n->pl(t);
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*(n->w_t[t]) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}


void PowerModel::add_SOCP_KCL_PV_Time(Node* n){
     double _power_factor = 0;
    for (int t = 0; t < _timesteps; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */

        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }

        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->candidate() || n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs()*((n->w_t[t])) + n->pl(t);
//        KCL_P += n->gs()*((n->wi_t[t]) + (n->wr_t[t])) + n->pl(t); Commented out 19/09/2016
//        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*((n->w_t[t])) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);
//        KCL_Q -= n->bs()*((n->wi_t[t]) + (n->wr_t[t])) - n->ql();  Commented out 19/09/2016
//            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}


//////////***//////////




void PowerModel::add_AC_KCL_PV_Time(Node* n){
     double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }
        
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->candidate() || n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) + n->pl(t);
//        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);
        
        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*(n->pl(t));
//            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }
    
}

void PowerModel::add_AC_KCL_PV_Time_Compact(Node* n){
     double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }
        
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time_Compact(n->get_out(), "pi", t);
        KCL_P += sum_Time_Compact(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->candidate() || n->inst()){
            KCL_P -= (_net->_radiation[t]) * (n->max_pv_size) * (0.86/ (1000 * _net->bMVA));
        }
        KCL_P += n->gs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) + (n->pl(t));
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);
        
        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time_Compact(n->get_out(), "qi", t);
        KCL_Q += sum_Time_Compact(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*(n->pl(t));
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }
    
}


void PowerModel::add_AC_KCL_Time(Node* n){
     double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
            
        }
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) + (n->pl(t));
//        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs()*(((n->vi_t[t])^2) + ((n->vr_t[t])^2)) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*(n->pl(t));
//            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}


void PowerModel::add_AC_KCL_NF(Node* n, bool PV){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
            
        }
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P -= sum_Time(n->get_in(), "pi", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->inst()|| (PV && n->candidate())){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += (n->pl(t));
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);
        
        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q -= sum_Time(n->get_in(), "qi", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q += (sqrt(1 - pow(_power_factor,2))/_power_factor)*(n->pl(t));
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }
    
}

void PowerModel::add_AC_KCL_NF_PV(Node* n){
    double _power_factor = 0;
    for (int t = 0; t < _timesteps*_nb_days; t++) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
         */
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
            
        }
        Constraint KCL_P("KCL_P_"+ n->_name + "_" + to_string(t));
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P -= sum_Time(n->get_in(), "pi", t);
        KCL_P -= sum_Time(n->_gen, "pg", t);
        if(n->inst() || n->candidate()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += (n->pl(t));
        //        cout << n->_name << " pload = " << n->pl(t) << endl;
        KCL_P = 0;
        _model->addConstraint(KCL_P);
        
        Constraint KCL_Q("KCL_Q_"+ n->_name + "_" + to_string(t));
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q -= sum_Time(n->get_in(), "qi", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q += (sqrt(1 - pow(_power_factor,2))/_power_factor)*(n->pl(t));
        //            cout << n->_name << " qload = " << n->ql() << endl;
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }
    
}


void PowerModel::add_AC_KCL_PV(Node* n, bool switch_lines){
    /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
     subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
     */
    Constraint KCL_P("KCL_P");
    KCL_P += sum(n->get_out(), "pi",switch_lines);
    KCL_P += sum(n->get_in(), "pj",switch_lines);
    KCL_P -= sum(n->_gen, "pg");
    KCL_P -= n->pv;
    KCL_P += n->gs()*(n->_V_.square_magnitude()) + n->pl();
    KCL_P = 0;
    _model->addConstraint(KCL_P);

    Constraint KCL_Q("KCL_Q");
    KCL_Q += sum(n->get_out(), "qi",switch_lines);
    KCL_Q += sum(n->get_in(), "qj",switch_lines);
    KCL_Q -= sum(n->_gen, "qg");
    KCL_Q -= n->bs()*(n->_V_.square_magnitude()) - n->ql();
    KCL_Q = 0;
    _model->addConstraint(KCL_Q);
}

void PowerModel::add_AC_KCL_SOCP_Time(Node *n){
     double _power_factor = 0;
    /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
     subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
     */
    for (int t=0; t<_timesteps ; t++) {
        if (t<24) {
            _power_factor = _summer_power_factor;
        }
        else{
            _power_factor = _winter_power_factor;
        }
        Constraint KCL_P("KCL_P");
        KCL_P += sum_Time(n->get_out(), "pi", t);
        KCL_P += sum_Time(n->get_in(), "pj", t);
        KCL_P -= sum_Time(n->_gen, "pg",t);
        if(n->inst()){
            KCL_P -= n->pv_t[t];
        }
        KCL_P += n->gs() * (n->w_t[t]) + n->pl(t);

        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q");
        KCL_Q += sum_Time(n->get_out(), "qi", t);
        KCL_Q += sum_Time(n->get_in(), "qj", t);
        KCL_Q -= sum_Time(n->_gen, "qg", t);
        KCL_Q -= n->bs() * (n->w_t[t]) - (sqrt(1 - pow(_power_factor,2))/_power_factor)*n->pl(t);

        KCL_Q = 0;
        _model->addConstraint(KCL_Q);
    }

}

void PowerModel::add_AC_KCL(Node* n, bool switch_lines){
    /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*v[i]^2 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
     subject to KCL_Q {i in buses}: sum{(l,i,j) in arcs} q[l,i,j] - shunt_b[i]*v[i]^2 + load_q[i] = sum{(i,gen) in bus_gen} qg[gen];
     */
    Constraint KCL_P("KCL_P");
    KCL_P += sum(n->get_out(), "pi",switch_lines);
    KCL_P += sum(n->get_in(), "pj",switch_lines);
    KCL_P -= sum(n->_gen, "pg");
    KCL_P += n->gs()*(n->_V_.square_magnitude()) + n->pl();

    KCL_P = 0;
    _model->addConstraint(KCL_P);
    
    Constraint KCL_Q("KCL_Q");
    KCL_Q += sum(n->get_out(), "qi",switch_lines);
    KCL_Q += sum(n->get_in(), "qj",switch_lines);
    KCL_Q -= sum(n->_gen, "qg");
    KCL_Q -= n->bs()*(n->_V_.square_magnitude()) - n->ql();

    KCL_Q = 0;
    _model->addConstraint(KCL_Q);
}


void PowerModel::post_AC_Polar(){
    for (auto n:_net->nodes) {
        add_AC_Voltage_Bounds(n);
        add_AC_KCL(n, false);
    }
    meta_var* p_ij = new meta_var("p_ij", _model);
    meta_var* q_ij = new meta_var("q_ij", _model);
    meta_var* p_ji = new meta_var("p_ji", _model);
    meta_var* q_ji = new meta_var("q_ji", _model);
    meta_var* v_i = new meta_var("v_i",_model);
    meta_var* v_j = new meta_var("v_j",_model);
    meta_var* th_i = new meta_var("th_i",_model);
    meta_var* th_j = new meta_var("th_j",_model);
    meta_constant* g = new meta_constant("g",_model);
    meta_constant* b = new meta_constant("b",_model);
    meta_constant* tr = new meta_constant("tr",_model);
    meta_constant* as = new meta_constant("as",_model);
    meta_constant* ch = new meta_constant("ch",_model);
    meta_constant* S = new meta_constant("S",_model);
    /** subject to Flow_P_From {(l,i,j) in arcs_from}:
     p[l,i,j] = g[l]*(v[i]/tr[l])^2
     + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
     + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
     */
    meta_Constraint* Meta_Flow_P_From = new meta_Constraint();
    *Meta_Flow_P_From = (*p_ij) - (*g)*(((*v_i)/(*tr))^2) + (*g)/(*tr)*((*v_i)*(*v_j)*cos(*th_i - *th_j - *as)) + (*b)/(*tr)*((*v_i)*(*v_j)*sin(*th_i - *th_j - *as));
    *Meta_Flow_P_From = 0;
//    Meta_Flow_P_From->print();
//    Meta_Flow_P_From->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_P_From);

    /** subject to Flow_P_To {(l,i,j) in arcs_to}:
     p[l,i,j] = g[l]*v[i]^2
     + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
     + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
     */
    meta_Constraint* Meta_Flow_P_To = new meta_Constraint();
    *Meta_Flow_P_To = (*p_ji) - (*g)*(Function((*v_j)^2)) + (*g)/(*tr)*((*v_i)*(*v_j)*cos(*th_j - *th_i + *as)) + (*b)/(*tr)*((*v_i)*(*v_j)*sin(*th_j - *th_i + *as));
    *Meta_Flow_P_To = 0;
//    Meta_Flow_P_To->print();
//    Meta_Flow_P_To->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_P_To);

    /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
     q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
     +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
     + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
     */
    meta_Constraint* Meta_Flow_Q_From = new meta_Constraint();
    *Meta_Flow_Q_From = (*q_ij) + ((0.5*(*ch))+(*b))*(((*v_i)/(*tr))^2) - (*b)/(*tr)*((*v_i)*(*v_j)*cos(*th_i - *th_j - *as)) + (*g)/(*tr)*((*v_i)*(*v_j)*sin(*th_i - *th_j - *as));
    *Meta_Flow_Q_From = 0;
//    Meta_Flow_Q_From->print();
//    Meta_Flow_Q_From->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_Q_From);

    /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
     q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
     +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
     + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
     */
    meta_Constraint* Meta_Flow_Q_To = new meta_Constraint();
    *Meta_Flow_Q_To = (*q_ji) + ((0.5*(*ch))+(*b))*(Function((*v_j)^2)) - (*b)/(*tr)*((*v_i)*(*v_j)*cos(*th_j - *th_i + *as)) + (*g)/(*tr)*((*v_i)*(*v_j)*sin(*th_j - *th_i + *as));
    *Meta_Flow_Q_To = 0;
//    Meta_Flow_Q_To->print();
//    Meta_Flow_Q_To->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_Q_To);

    meta_Constraint* Thermal_Limit_From = new meta_Constraint();
    *Thermal_Limit_From = (*p_ij)*(*p_ij) + (*q_ij)*(*q_ij) - (*S)*(*S);
    *Thermal_Limit_From <= 0;
//    Thermal_Limit_From->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Thermal_Limit_From);

    meta_Constraint* Thermal_Limit_To = new meta_Constraint();
    *Thermal_Limit_To = (*p_ij)*(*p_ij) + (*q_ij)*(*q_ij) - (*S)*(*S);
    *Thermal_Limit_To <= 0;
//    Thermal_Limit_To->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Thermal_Limit_To);

    _model->init_functions(_net->arcs.size()+1);
    _model->print_functions();
//    exit(-1);
    for (auto a:_net->arcs) {
        if(a->status==1){
            add_AC_Angle_Bounds(a, false);
            *as = a->as;
            *g = a->g;
            *b = a->b;
            *tr = a->tr;
            *ch = a->ch;
            *S = a->limit;
            *p_ij = a->pi;
            *q_ij = a->qi;
            *p_ji = a->pj;
            *q_ji = a->qj;
            *v_i = a->src->v;
            *v_j = a->dest->v;
            *th_i = a->src->theta;
            *th_j = a->dest->theta;
            _model->concretise(*Meta_Flow_P_From, a->id, "Flow_From_"+a->pi._name);
            _model->concretise(*Meta_Flow_Q_From, a->id, "Flow_From_"+a->qi._name);
            _model->concretise(*Meta_Flow_P_To, a->id, "Flow_To_"+a->pi._name);
            _model->concretise(*Meta_Flow_Q_To, a->id, "Flow_To_"+a->qi._name);

            add_AC_thermal(a, false);
        }
    }
}

void PowerModel::post_QC(){
    //    Constraint ref("t_ref");
    //    ref += _net->nodes[27]->theta;
    //    ref = 0;
    //    _model->addConstraint(ref);
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }

        //        Constraint Theta_Delta_UB_W("Theta_Delta_UB_W");
        //        Theta_Delta_UB_W += a->vsn - tan(a->tbound.max)*a->vcs;
        //        Theta_Delta_UB_W <= 0;
        //        _model->addConstraint(Theta_Delta_UB_W);
        //        Constraint Theta_Delta_LB_W("Theta_Delta_LB_W");
        //        Theta_Delta_LB_W += a->vsn - tan(a->tbound.min)*a->vcs;
        //        Theta_Delta_LB_W >= 0;
        //        _model->addConstraint(Theta_Delta_LB_W);

        //add_AC_Angle_Bounds(a, false);

        Constraint soc("soc");
        soc += a->vcs*a->vcs + a->vsn*a->vsn;
        soc -= a->src->w*a->dest->w;
        soc <= 0;
        _model->addConstraint(soc);

        add_AC_thermal(a, false);

        Node* src = a->src;
        Node* dest = a->dest;

        Constraint Delta("Delta"+a->_name);
        Delta += a->delta - src->theta + dest->theta + a->as;
        Delta = 0;
        _model->addConstraint(Delta);

        /** AC Power Flow */

        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;

        Flow_P_From -= a->g*src->w/(pow(a->cc,2)+pow(a->dd,2));
        Flow_P_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vcs;
        Flow_P_From -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vsn;

        //         Flow_P_From -= a->g*(src->w)/pow(a->tr,2.);
        //         Flow_P_From += a->g/a->tr*a->vcs;
        //         Flow_P_From += a->b/a->tr*a->vsn;

        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);

        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To -= a->g*(dest->w);

        Flow_P_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vcs;
        Flow_P_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vsn;

        //         Flow_P_To += a->g/a->tr*a->vcs;
        //         Flow_P_To -= a->b/a->tr*a->vsn;

        Flow_P_To = 0;
        _model->addConstraint(Flow_P_To);

        Constraint Flow_Q_From("Flow_Q_From"+a->qi._name);
        Flow_Q_From += a->qi;

        Flow_Q_From += (a->ch/2+a->b)*src->w/(pow(a->cc,2)+pow(a->dd,2));
        Flow_Q_From += (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vcs;
        Flow_Q_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vsn;

        //         Flow_Q_From += (a->ch/2+a->b)*(src->w)/pow(a->tr,2.);
        //         Flow_Q_From -= a->b/a->tr*a->vcs;
        //         Flow_Q_From += a->g/a->tr*a->vsn;
        Flow_Q_From = 0;
        _model->addConstraint(Flow_Q_From);


        Constraint Flow_Q_To("Flow_Q_To"+a->qj._name);
        Flow_Q_To += a->qj;
        Flow_Q_To += (a->ch/2+a->b)*(dest->w);

        Flow_Q_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vcs;
        Flow_Q_To += (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->vsn;

        //         Flow_Q_To -= a->b/a->tr*a->vcs;
        //         Flow_Q_To -= a->g/a->tr*a->vsn;
        Flow_Q_To = 0;
        _model->addConstraint(Flow_Q_To);

        /** cos */

        double theta_u;
        double tm, tl = a->tbound.min - a->as, tu = a->tbound.max - a->as, delta;
        theta_u = max(-tl,tu);
        delta = (cos(tu) - cos(tl))/(tu - tl);

        if (tl >= 0) tm = tu + 0.001;
        else if (tu <= 0) tm = tl - 0.001;
        else tm = 0;

        Constraint Cs_UBound("Cs_UBound"+a->_name);
        Cs_UBound += a->cs;
//        Cs_UBound -= 1 - (1 - cos(theta_u))/pow(theta_u,2)*a->delta*a->delta;
        Cs_UBound -= (cos(tm) - cos(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl)*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl);
        Cs_UBound -= delta*(-tu + a->delta) + cos(tu);
        //Cs_UBound -= cos(a->delta);
        Cs_UBound <= 0;
        _model->addConstraint(Cs_UBound);

        Constraint Cs_LBound("Cs_LBound"+a->_name);
        Cs_LBound += a->cs;
        Cs_LBound -= -(cos(tl)-cos(tu))*(tl-a->delta)/(tl-tu) + cos(tl);
        Cs_LBound >= 0;
        _model->addConstraint(Cs_LBound);

        /** sin */
        delta = (sin(tu) - sin(tl))/(tu - tl);
        Constraint Sn_UBound("Sn_UBound"+a->_name);
        Sn_UBound += a->sn;
        Constraint Sn_LBound("Sn_LBound"+a->_name);
        Sn_LBound += a->sn;
        double *x = new double[_model->get_nb_vars()];
        if (tl < 0 && tu > 0) {
            double theta_m = max(-tl, tu);
            Sn_UBound -= -cos(theta_m/2)*(theta_m/2-a->delta)+sin(theta_m/2);
            Sn_LBound -= cos(theta_m/2)*(theta_m/2+a->delta)-sin(theta_m/2);
        }else if(tl >= 0){

            Sn_UBound -= (sin(tm) - sin(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl)*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl);
            Sn_UBound -= delta*(-tu + a->delta) + sin(tu);


            Function Sin;
            Sin += sin(a->delta);
            x[a->delta.get_idx()] = (tu+tl)/2;
//            Sn_UBound -= Sin.outer_approx(x);

            //            Sn_UBound -= sin(a->delta);

            Sn_LBound -= -(sin(tl)-sin(tu))*(tl-a->delta)/(tl-tu) + sin(tl);
        }else if(tu <= 0){

            Function Sin;
            Sin += sin(a->delta);
            x[a->delta.get_idx()] = (tl+tu)/2;
            Sn_UBound -= -(sin(tl)-sin(tu))*(tl-a->delta)/(tl-tu) + sin(tl);

//            Sn_LBound -= Sin.outer_approx(x);

            double A = (sin(tm) - sin(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl);
            Sn_LBound -= A*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl);
            Sn_LBound -= delta*(-tu + a->delta) + sin(tu);

            //            Sn_LBound -= sin(a->delta);
        }
        delete[] x;
        Sn_UBound <= 0;
        _model->addConstraint(Sn_UBound);
        Sn_LBound >= 0;
        _model->addConstraint(Sn_LBound);

        /** McCormick for vi*vj */

        _model->add_McCormick("VV"+a->_name, a->vv, src->v, dest->v);

        /** McCormick for vi*vj*cs */

        _model->add_McCormick("VCS"+a->_name, a->vcs, a->vv, a->cs);

        /** McCormick for vi*vj*sn */

        _model->add_McCormick("VSN"+a->_name, a->vsn, a->vv, a->sn);

        /** Current magnitude */

        //        Constraint Current_Magnitude_From("Current_Magnitude_From"+a->_name);
        //        Current_Magnitude_From += a->pi*a->pi + a->qi*a->qi;
        //        Current_Magnitude_From -= src->w*a->ci/pow(a->tr, 2.);
        //        Current_Magnitude_From <= 0;
        //        _model->addConstraint(Current_Magnitude_From);

        /** Losses */

        //        Constraint P_Loss_From("P_Loss_From"+a->_name);
        //        P_Loss_From += a->r*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*src->w);
        //        P_Loss_From -= a->pi + a->pj;
        //        P_Loss_From = 0;
        //        _model->addConstraint(P_Loss_From);

        //        Constraint Q_Loss_From("Q_Loss_From"+a->_name);
        //        Q_Loss_From += a->x*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*src->w) - ((a->ch/2)/pow(a->tr,2))*src->w - (a->ch/2)*dest->w;
        //        Q_Loss_From -= a->qi + a->qj;
        //        Q_Loss_From = 0;
        //        _model->addConstraint(Q_Loss_From);
    }

    for (auto n:_net->nodes) {
        add_AC_KCL(n, false);
        /** Quadratic */
        Constraint W_UBound("W_UBound"+n->_name);
        W_UBound += n->w;
        W_UBound -= (n->vbound.max+n->vbound.min)*n->v - n->vbound.max*n->vbound.min;
        W_UBound <= 0;
        _model->addConstraint(W_UBound);
        Constraint W_LBound("W_LBound"+n->_name);
        W_LBound += n->w;
        W_LBound -= n->v*n->v;
        W_LBound >= 0;
        _model->addConstraint(W_LBound);
    }
//    _model->print_solution();
}

/*void PowerModel::post_AC_OTS(){
    for (auto n:_net->nodes) {
        add_AC_KCL(n, true);
    }
    for (auto a:_net->arcs) {
        add_AC_Angle_Bounds(a, true);
        add_AC_Power_Flow(a, true);
        add_AC_thermal(a, true);
    }
}*/

void PowerModel::post_AC_OTS(){
    for (auto n:_net->nodes) {
        //add_AC_Voltage_Bounds(n);
        add_AC_KCL(n, true);
    }
    meta_var* p_ij = new meta_var("p_ij", _model);
    meta_var* q_ij = new meta_var("q_ij", _model);
    meta_var* p_ji = new meta_var("p_ji", _model);
    meta_var* q_ji = new meta_var("q_ji", _model);
    meta_var* v_i = new meta_var("v_i",_model);
    meta_var* v_j = new meta_var("v_j",_model);
    meta_var* th_i = new meta_var("th_i",_model);
    meta_var* th_j = new meta_var("th_j",_model);
    meta_constant* g = new meta_constant("g",_model);
    meta_constant* b = new meta_constant("b",_model);
    meta_constant* tr = new meta_constant("tr",_model);
    meta_constant* as = new meta_constant("as",_model);
    meta_constant* ch = new meta_constant("ch",_model);
    meta_constant* S = new meta_constant("S",_model);
    /** subject to Flow_P_From {(l,i,j) in arcs_from}:
     p[l,i,j] = g[l]*(v[i]/tr[l])^2
     + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
     + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
     */
    meta_Constraint* Meta_Flow_P_From = new meta_Constraint();
    *Meta_Flow_P_From = (*p_ij) - (*g)*(((*v_i)/(*tr))^2) + (*g)/(*tr)*((*v_i)*(*v_j)*cos(*th_i - *th_j - *as)) + (*b)/(*tr)*((*v_i)*(*v_j)*sin(*th_i - *th_j - *as));
    *Meta_Flow_P_From = 0;
//    Meta_Flow_P_From->print();
//    Meta_Flow_P_From->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_P_From);

    /** subject to Flow_P_To {(l,i,j) in arcs_to}:
     p[l,i,j] = g[l]*v[i]^2
     + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
     + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
     */
    meta_Constraint* Meta_Flow_P_To = new meta_Constraint();
    *Meta_Flow_P_To = (*p_ji) - (*g)*(Function((*v_j)^2)) + (*g)/(*tr)*((*v_i)*(*v_j)*cos(*th_j - *th_i + *as)) + (*b)/(*tr)*((*v_i)*(*v_j)*sin(*th_j - *th_i + *as));
    *Meta_Flow_P_To = 0;
//    Meta_Flow_P_To->print();
//    Meta_Flow_P_To->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_P_To);

    /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
     q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
     +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
     + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
     */
    meta_Constraint* Meta_Flow_Q_From = new meta_Constraint();
    *Meta_Flow_Q_From = (*q_ij) + ((0.5*(*ch))+(*b))*(((*v_i)/(*tr))^2) - (*b)/(*tr)*((*v_i)*(*v_j)*cos(*th_i - *th_j - *as)) + (*g)/(*tr)*((*v_i)*(*v_j)*sin(*th_i - *th_j - *as));
    *Meta_Flow_Q_From = 0;
//    Meta_Flow_Q_From->print();
//    Meta_Flow_Q_From->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_Q_From);

    /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
     q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
     +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
     + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
     */
    meta_Constraint* Meta_Flow_Q_To = new meta_Constraint();
    *Meta_Flow_Q_To = (*q_ji) + ((0.5*(*ch))+(*b))*(Function((*v_j)^2)) - (*b)/(*tr)*((*v_i)*(*v_j)*cos(*th_j - *th_i + *as)) + (*g)/(*tr)*((*v_i)*(*v_j)*sin(*th_j - *th_i + *as));
    *Meta_Flow_Q_To = 0;
//    Meta_Flow_Q_To->print();
//    Meta_Flow_Q_To->_val.resize(_net->arcs.size());
    _model->addMetaConstraint(*Meta_Flow_Q_To);

//    meta_Constraint* Thermal_Limit_From = new meta_Constraint();
//    *Thermal_Limit_From = (*p_ij)*(*p_ij) + (*q_ij)*(*q_ij) - (*S)*(*S);
//    *Thermal_Limit_From <= 0;
////    Thermal_Limit_From->_val.resize(_net->arcs.size());
//    _model->addMetaConstraint(*Thermal_Limit_From);
//
//    meta_Constraint* Thermal_Limit_To = new meta_Constraint();
//    *Thermal_Limit_To = (*p_ij)*(*p_ij) + (*q_ij)*(*q_ij) - (*S)*(*S);
//    *Thermal_Limit_To <= 0;
////    Thermal_Limit_To->_val.resize(_net->arcs.size());
//    _model->addMetaConstraint(*Thermal_Limit_To);

    _model->init_functions(_net->arcs.size()+1);
    _model->print_functions();
//    exit(-1);
    for (auto a:_net->arcs) {
        if(a->status==1){
            add_AC_Angle_Bounds(a, false);
            *as = a->as;
            *g = a->g;
            *b = a->b;
            *tr = a->tr;
            *ch = a->ch;
            *S = a->limit;
            *p_ij = a->pi;
            *q_ij = a->qi;
            *p_ji = a->pj;
            *q_ji = a->qj;
            *v_i = a->src->v;
            *v_j = a->dest->v;
            *th_i = a->src->theta;
            *th_j = a->dest->theta;
            _model->concretise(*Meta_Flow_P_From, a->id, "Flow_From_"+a->pi._name);
            _model->concretise(*Meta_Flow_Q_From, a->id, "Flow_From_"+a->qi._name);
            _model->concretise(*Meta_Flow_P_To, a->id, "Flow_To_"+a->pi._name);
            _model->concretise(*Meta_Flow_Q_To, a->id, "Flow_To_"+a->qi._name);

            add_AC_thermal(a, true);
        }
    }
}

void PowerModel::post_AC_SOCP_Time() {
    Node *src = NULL;
    Node *dest = NULL;

    for (auto n:_net->nodes) {
        add_AC_KCL_SOCP_Time(n);
        if (n->inst()) {
            add_link_PV_fixed_Rate(n);
        }
    }


    for (auto a:_net->arcs) {
        Arc *ap = nullptr;
        if (a->status == 1) {
            for (int t = 0; t < _timesteps; t++) {
                src = a->src;
                dest = a->dest;
                /** subject to Flow_P_From {(l,i,j) in arcs_from}:
                 p[l,i,j] = g[l]*(v[i]/tr[l])^2
                 + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                 + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
                 */
                Constraint Flow_P_From("Flow_P_From" + a->pi_t[t]._name);
                Flow_P_From += a->pi_t[t];
                //        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
                //            cout << "ok";
                //            cout << endl;
                //        }

                Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w_t[t];
                ap = _net->get_arc(a->src, a->dest);
                Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_From = 0;
                _model->addConstraint(Flow_P_From);

                /* * subject to Flow_P_To {(l,i,j) in arcs_to}:
                  p[l,i,j] = g[l]*v[i]^2
                  + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                  + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);*/

                Constraint Flow_P_To("Flow_P_To" + a->pj_t[t]._name);
                Flow_P_To += a->pj_t[t];
                Flow_P_To -= a->g * dest->w_t[t];
                Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_P_To = 0;
                _model->addConstraint(Flow_P_To);

                /*       * subject to Flow_Q_From {(l,i,j) in arcs_from}:
                        q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
                        +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
                        + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);*/

                Constraint Flow_Q_From("Flow_Q_From" + a->qi_t[t]._name);
                Flow_Q_From += a->qi_t[t];
                Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w_t[t];
                Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_From = 0;
                _model->addConstraint(Flow_Q_From);

                /*        * subject to Flow_Q_To {(l,i,j) in arcs_to}:
                         q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
                         +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
                         + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);*/

                Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
                Flow_Q_To += a->qj_t[t];
                Flow_Q_To += (a->ch / 2 + a->b) * dest->w_t[t];
                Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr_t[t];
                Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi_t[t];
                Flow_Q_To = 0;
                _model->addConstraint(Flow_Q_To);

                /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
                 */
                Constraint SOCP("SOCP");
                SOCP += a->src->w_t[t] * a->dest->w_t[t];
                SOCP -= ((ap->wr_t[t]) ^ 2);
                SOCP -= ((ap->wi_t[t]) ^ 2);
                SOCP >= 0;
                _model->addConstraint(SOCP);  //


            }
            add_AC_thermal_Time(a);
            add_SOCP_Angle_Bounds_Time(a);
        }
//        add_Wr_Wi_time(a);
    }
}

void PowerModel::post_AC_SOCP(){
    Node* src = NULL;
    Node* dest = NULL;
    for (auto n:_net->nodes) {
        add_AC_KCL(n, false);
    }
    Arc* ap = nullptr;

    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        src = a->src;
        dest = a->dest;
        /** subject to Flow_P_From {(l,i,j) in arcs_from}:
         p[l,i,j] = g[l]*(v[i]/tr[l])^2
         + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;
//        if(!a->src->_name.compare("9006") && !a->dest->_name.compare("9003")){
//            cout << "ok";
//            cout << endl;
//        }

        Flow_P_From -= (a->g/(pow(a->cc,2)+pow(a->dd,2)))*src->w;
        ap = _net->get_arc(a->src, a->dest);
        Flow_P_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wr;
        Flow_P_From -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wi;
        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);

        /** subject to Flow_P_To {(l,i,j) in arcs_to}:
         p[l,i,j] = g[l]*v[i]^2
         + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To -= a->g*dest->w;
        Flow_P_To -= (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wr;
        Flow_P_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wi;
        Flow_P_To = 0;
        _model->addConstraint(Flow_P_To);

        /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
         q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
         +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Constraint Flow_Q_From("Flow_Q_From"+a->qi._name);
        Flow_Q_From += a->qi;
        Flow_Q_From += (a->ch/2+a->b)/(pow(a->cc,2)+pow(a->dd,2))*src->w;
        Flow_Q_From += (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wr;
        Flow_Q_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wi;
        Flow_Q_From = 0;
        _model->addConstraint(Flow_Q_From);

        /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
         q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
         +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_Q_To("Flow_Q_To"+a->qj._name);
        Flow_Q_To += a->qj;
        Flow_Q_To += (a->ch/2+a->b)*dest->w;
        Flow_Q_To += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wr;
        Flow_Q_To += (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*ap->wi;
        Flow_Q_To = 0;
        _model->addConstraint(Flow_Q_To);

        /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
         */
        Constraint SOCP("SOCP");
        SOCP += a->src->w*a->dest->w;
        SOCP -= ((ap->wr)^2);
        SOCP -= ((ap->wi)^2);
        SOCP >= 0;
        _model->addConstraint(SOCP);

        add_AC_thermal(a, false);
        add_Wr_Wi(a);
    }
}


void PowerModel::add_AC_Voltage_Bounds_Time(Node* n){
    if (n->_has_gen) {
//        cout<< endl << endl << n->_name << " has generator." << endl<< endl;
        for (int t = 0; t < _timesteps*_nb_days; t++) {
            Constraint V_FIX("V_Fixed");
            V_FIX += ((n->vi_t[t])^2) + ((n->vr_t[t])^2);
            V_FIX = pow(n->vbound.max,2);
            _model->addConstraint(V_FIX);
            
//            Constraint V_LB("V_LB");
//            V_LB += ((n->vi_t[t])^2) + ((n->vr_t[t])^2);
//            V_LB = 1;
//            _model->addConstraint(V_LB);
        }
        return;
    }
    /** subject to V_UB{i in buses}: vr[i]^2 + vi[i]^2 <= v_max[i]^2;
     subject to V_LB{i in buses}: vr[i]^2 + vi[i]^2 >= v_min[i]^2;
     */
     for (int t = 0; t < _timesteps*_nb_days; t++) {
        Constraint V_UB("V_UB");
        V_UB += ((n->vi_t[t])^2) + ((n->vr_t[t])^2);
        V_UB <= pow(n->vbound.max,2);
        _model->addConstraint(V_UB);
        
        Constraint V_LB("V_LB");
        V_LB += ((n->vi_t[t])^2) + ((n->vr_t[t])^2);
        V_LB >= pow(n->vbound.min,2);
        _model->addConstraint(V_LB);
     }
}


void PowerModel::add_AC_Voltage_Bounds(Node* n){
    /** subject to V_UB{i in buses}: vr[i]^2 + vi[i]^2 <= v_max[i]^2;
     subject to V_LB{i in buses}: vr[i]^2 + vi[i]^2 >= v_min[i]^2;
     */
    Constraint V_UB("V_UB");
    V_UB += (n->_V_.square_magnitude());
    V_UB <= pow(n->vbound.max,2);
    _model->addConstraint(V_UB);

    Constraint V_LB("V_LB");
    V_LB += (n->_V_.square_magnitude());
    V_LB >= pow(n->vbound.min,2);
    _model->addConstraint(V_LB);
}


void PowerModel::post_AC_Rect(){

    for (auto n:_net->nodes) {
        add_AC_Voltage_Bounds(n);
        add_AC_KCL(n, false);
    }
    for (auto a:_net->arcs) {
        add_AC_Power_Flow(a, false);
        add_AC_thermal(a, false);
    }


}


void PowerModel::post_DC(){
    Node* src = NULL;
    Node* dest = NULL;
    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        src = a->src;
        dest = a->dest;
        /** Adding phase angle difference bound constraint
         subject to Theta_Delta_LB{(l,i,j) in arcs_from}: t[i]-t[j] >= -theta_bound;
         subject to Theta_Delta_UB{(l,i,j) in arcs_from}: t[i]-t[j] <= theta_bound;
         */
        Constraint Theta_Delta_UB("Theta_Delta_UB");
        Theta_Delta_UB += src->theta - dest->theta;
        Theta_Delta_UB <= a->tbound.max;
        _model->addConstraint(Theta_Delta_UB);
        Constraint Theta_Delta_LB("Theta_Delta_LB");
        Theta_Delta_LB += src->theta - dest->theta;
        Theta_Delta_LB >= a->tbound.min;
        _model->addConstraint(Theta_Delta_LB);

        /** subject to Flow_P_From {(l,i,j) in arcs_from}:
         p[l,i,j] = g[l]*(v[i]/tr[l])^2
         + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;
        Flow_P_From += a->b/a->tr*(src->theta - dest->theta - a->as);
        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);
        /** subject to Flow_P_To {(l,i,j) in arcs_to}:
         p[l,i,j] = p[l,j,i];
         */
        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To += a->pi;
        Flow_P_To = 0;
        _model->addConstraint(Flow_P_To);

        /** subject to Thermal_Limit {(l,i,j) in arcs}: p[l,i,j] <=  s[l];
         */
        Constraint Thermal_Limit_from("Thermal_Limit_from");
        Thermal_Limit_from += a->pi;
        Thermal_Limit_from <= a->limit;
        _model->addConstraint(Thermal_Limit_from);
        Constraint Thermal_Limit_to("Thermal_Limit_to");
        Thermal_Limit_to += a->pj;
        Thermal_Limit_to <= a->limit;
        _model->addConstraint(Thermal_Limit_to);
    }

    for (auto n:_net->nodes) {
        /** subject to KCL_P {i in buses}: sum{(l,i,j) in arcs} p[l,i,j] + shunt_g[i]*1.0 + load_p[i] = sum{(i,gen) in bus_gen} pg[gen];
         */
        Constraint KCL_P("KCL_P");
        for (auto a:n->branches) {
            if (a->status==0) {
                continue;
            }
            if(a->src==n){
                KCL_P += a->pi;
            }
            else {
                KCL_P += a->pj;
            }
        }
        for (auto g:n->_gen) {
            if (!g->_active)
                continue;
            KCL_P -= g->pg;
        }
        KCL_P += n->gs() + n->pl();
        KCL_P = 0;
        _model->addConstraint(KCL_P);
    }
}
//void PowerModel::post_AC_Polar(){
//    for (auto a:_net->arcs) {
//        if (a->status==0) {
//            continue;
//        }
//        add_AC_Angle_Bounds(a);
//        add_AC_Power_Flow(a, true);
//        add_AC_thermal(a);
//    }
//    for (auto n:_net->nodes) {
//        add_AC_KCL(n);
//    }
//
//}

/*void PowerModel::add_Cosine(Arc *a){
    double *x = new double[_model->get_nb_vars()];
    double theta_u, tm;
    double Ml = _net->m_theta_lb;
    double Mu = _net->m_theta_ub;
    double l = a->tbound.min;
    double u = a->tbound.max;
    Function CSR; // cs(theta,u) if theta > 0
    theta_u = max(-l,u);
    double alpha = (1-cos(theta_u))/pow(theta_u,2);
    double on_cont;

//    Function Cs_r;
//    Cs_r += a->delta - Mu*(1-a->on) - sqrt((a->on*a->on-a->cs*a->on)/alpha);
//    on_cont = 0.5;
//    CSR += on_cont - alpha*(a->delta*a->delta - 2*Mu*a->delta*(1-on_cont) + pow(Mu,2)*(1-on_cont)*(1-on_cont))/on_cont;
//    x[a->on.get_idx()] = on_cont;
//    x[a->delta.get_idx()] = u;
//    x[a->delta.get_idx()] = Mu - Mu*on_cont + u*on_cont/2;
//    x[a->cs.get_idx()] = CSR.eval(x);
//    Constraint CS_UB_OAR("CS_UB_OAR");
//    CS_UB_OAR += Cs_r.outer_approx(x);
//    CS_UB_OAR <= 0;
//    _model->addConstraint(CS_UB_OAR);

//    Function CSL; // cs(theta,u) if theta < 0
//    Function Cs_l;
//    Cs_l += Ml*(1-a->on) - a->delta - sqrt((a->on*a->on-a->cs*a->on)/alpha);
//    on_cont = 0.5;
//    CSL += on_cont - alpha*(a->delta*a->delta - 2*Ml*a->delta*(1-on_cont) + pow(Ml,2)*(1-on_cont)*(1-on_cont))/on_cont;
//    x[a->on.get_idx()] = on_cont;
//    x[a->delta.get_idx()] = Ml - Ml*on_cont + l*on_cont/2;
//    x[a->cs.get_idx()] = CSL.eval(x);
//    Constraint CS_UB_OAL("CS_UB_OAL");
//    CS_UB_OAL += Cs_l.outer_approx(x);
//    CS_UB_OAL <= 0;
//    _model->addConstraint(CS_UB_OAL);


   // Function f = alpha*a->delta*a->delta - 1 + a->cs;

    if (l >= 0) tm = u + 0.001;
    else if (u <= 0) tm = l - 0.001;
    else tm = 0;

    double delta = (cos(u) - cos(l))/(u - l);
    double A = (cos(tm) - cos(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l);
    Function f = -A*(a->delta*a->delta - (u+l)*a->delta + u*l) - delta*(-u + a->delta) - cos(u) + a->cs;

    if(u > 0) {
        Function Cs_r;
        Cs_r += a->delta - Mu*(1-a->on) - sqrt((a->on*a->on-a->cs*a->on)/alpha);

        for(int i = 5; i <= 5; ++i) {
            on_cont = i*0.2;
            if (on_cont == 0) on_cont = 0.01;
            CSR.reset();
            CSR += on_cont - alpha*(a->delta*a->delta - 2*Mu*a->delta*(1-on_cont) + pow(Mu,2)*(1-on_cont)*(1-on_cont))/on_cont;

            x[a->on.get_idx()] = on_cont;

////            if (on_cont >= 0.4) {
//              if(0.01 >= Mu - Mu*on_cont) {
//                x[a->delta.get_idx()] = 0.01;
////                x[a->delta.get_idx()] = Mu - Mu*on_cont + 0.01;
//                x[a->cs.get_idx()] = CSR.eval(x);
//                Constraint CS_UB_OAR1("CS_UB_OAR1");
//                CS_UB_OAR1 += Cs_r.outer_approx(x);
//                CS_UB_OAR1 <= 0;
//                _model->addConstraint(CS_UB_OAR1);
//            }

//            if(u/2 >= Mu - Mu*on_cont) {
//                x[a->delta.get_idx()] = u/2;
////                x[a->delta.get_idx()] = Mu - Mu*on_cont + u*on_cont/2;
//                x[a->cs.get_idx()] = CSR.eval(x);
//                Constraint CS_UB_OAR2("CS_UB_OAR2");
//                CS_UB_OAR2 += Cs_r.outer_approx(x);
//                CS_UB_OAR2 <= 0;
//                _model->addConstraint(CS_UB_OAR2);
//            }

//            if(u >= Mu - Mu*on_cont) {
//            if (on_cont >= 0.4) {
//                x[a->delta.get_idx()] = u; //need to check for zero
            x[a->delta.get_idx()] = Mu + on_cont*(u - Mu);

            Function fch = f.quad_ch_right(a->delta, a->cs, a->on, x);
            cout << "f_right_approx: " << endl;
            fch.print(false);
            cout << endl;

            x[a->cs.get_idx()] = CSR.eval(x);
            Constraint CS_UB_OAR3("CS_UB_OAR3");
            CS_UB_OAR3 += Cs_r.outer_approx(x);
            CS_UB_OAR3 <= 0;
                _model->addConstraint(CS_UB_OAR3);
//            }

        }
    }

    if(l < 0) {
        Function CSL; // cs(theta,u) if theta < 0
        Function Cs_l;
        Cs_l += Ml*(1-a->on) - a->delta - sqrt((a->on*a->on-a->cs*a->on)/alpha);

        for(int i = 5; i <= 5; ++i) {
            on_cont = i*0.2;
            if (on_cont == 0) on_cont = 0.01;
            CSL.reset();
            CSL += on_cont - alpha*(a->delta*a->delta - 2*Ml*a->delta*(1-on_cont) + pow(Ml,2)*(1-on_cont)*(1-on_cont))/on_cont;

            x[a->on.get_idx()] = on_cont;

//            if(-0.01 <= Ml - Ml*on_cont) {
////            if (on_cont >= 0.4) {
//                x[a->on.get_idx()] = on_cont;
//                x[a->delta.get_idx()] = -0.01;
////                x[a->delta.get_idx()] = Ml - Ml*on_cont - 0.01;
//                x[a->cs.get_idx()] = CSL.eval(x);
//                Constraint CS_UB_OAL1("CS_UB_OAL1");
//                CS_UB_OAL1 += Cs_l.outer_approx(x);
//                CS_UB_OAL1 <= 0;
//                _model->addConstraint(CS_UB_OAL1);
//            }

//            if(l/2 <= Ml - Ml*on_cont) {
//                x[a->delta.get_idx()] = l/2;
////                x[a->delta.get_idx()] = Ml - Ml*on_cont + l*on_cont/2;
//                x[a->cs.get_idx()] = CSL.eval(x);
//                Constraint CS_UB_OAL2("CS_UB_OAL2");
//                CS_UB_OAL2 += Cs_l.outer_approx(x);
//                CS_UB_OAL2 <= 0;
//                _model->addConstraint(CS_UB_OAL2);
//            }

//            if(l <= Ml - Ml*on_cont) {
//            if (on_cont >= 0.4) {
//                x[a->delta.get_idx()] = l; //need to check for zero
            x[a->delta.get_idx()] = Ml + on_cont*(l - Ml);

            Function fch = f.quad_ch_left(a->delta, a->cs, a->on, x);
            cout << "f_left_approx: " << endl;
            fch.print(false);
            cout << endl;

            x[a->cs.get_idx()] = CSL.eval(x);
            Constraint CS_UB_OAL3("CS_UB_OAL3");
            CS_UB_OAL3 += Cs_l.outer_approx(x);
            CS_UB_OAL3 <= 0;
                _model->addConstraint(CS_UB_OAL3);
//            }

        }
    }
}*/

void PowerModel::add_Cosine(Arc *a){
    double *x = new double[_model->get_nb_vars()];
    double tm;
    double l = a->tbound.min - a->as;
    double u = a->tbound.max - a->as;
    if (l >= 0) tm = u + 0.001;
    else if (u <= 0) tm = l - 0.001;
    else tm = 0;
    double delta = (cos(u) - cos(l))/(u - l);
    double A = (cos(tm) - cos(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l);
    //double theta_u = max(-l,u);
    //double alpha = (1-cos(theta_u))/pow(theta_u,2);
    Function f = -A*(a->delta*a->delta - (u+l)*a->delta + u*l) - delta*(-u + a->delta) - cos(u) + a->cs;
    //Function f = alpha*a->delta*a->delta - 1 + a->cs;
    if(u > 0) {
        x[a->delta.get_idx()] = u;
        Constraint CS_UB_OAR("CS_UB_OAR");
        CS_UB_OAR += f.quad_ch_right(a->delta, a->cs, a->on, x);
        CS_UB_OAR <= 0;
        _model->addConstraint(CS_UB_OAR);
    }
    if(l < 0) {
        x[a->delta.get_idx()] = l;
        Constraint CS_UB_OAL("CS_UB_OAL");
        CS_UB_OAL += f.quad_ch_left(a->delta, a->cs, a->on, x);
        CS_UB_OAL <= 0;
        _model->addConstraint(CS_UB_OAL);
    }
}

void PowerModel::add_SineCutsPos(Arc *a){
    double *x = new double[_model->get_nb_vars()];
    double tm;
    double l = a->tbound.min - a->as;
    double u = a->tbound.max - a->as;
    tm = u + 0.001;
    if (l < 0)  {
        cerr << "\nadd_SineCutsPos called with unknown or negative sign of a->delta.";
        exit(1);
    }
    double delta = (sin(u) - sin(l))/(u - l);
//    double A = (sin(tm) - sin(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l);
    Function f = -1*(sin(tm) - sin(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l)*(a->delta*a->delta - (u+l)*a->delta + u*l) - delta*(-u + a->delta) - sin(u) + a->sn;

    x[a->delta.get_idx()] = u;
    Constraint SN_UB_OA("SN_UB_OA");
    SN_UB_OA += f.quad_ch_right(a->delta, a->sn, a->on, x);
    SN_UB_OA <= 0;
    _model->addConstraint(SN_UB_OA);
}

void PowerModel::add_SineCutsNeg(Arc *a){
    double *x = new double[_model->get_nb_vars()];
    double tm;
    double l = a->tbound.min - a->as;
    double u = a->tbound.max - a->as;
    tm = l - 0.001;
    if (u > 0)  {
        cerr << "\nadd_SineCutsNeg called with unknown or positive sign of a->delta.";
        exit(1);
    }
    double delta = (sin(u) - sin(l))/(u - l);
//    double A = (sin(tm) - sin(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l);
    Function f = (sin(tm) - sin(u) - delta*(tm - u))/(tm*tm - tm*(u+l) + u*l)*(a->delta*a->delta - (u+l)*a->delta + u*l) + delta*(-u + a->delta) + sin(u) - a->sn;

    x[a->delta.get_idx()] = l;
    Constraint SN_LB_OA("SN_LB_OA");
    SN_LB_OA += f.quad_ch_right(a->delta, a->sn, a->on, x);
    SN_LB_OA <= 0;
    _model->addConstraint(SN_LB_OA);
}

void PowerModel::post_QC_OTS(bool lin_cos_cuts, bool quad_cos){
    Constraint ref("t_ref");
    ref += _net->arcs.front()->src->theta;
    ref = 0;
    _model->addConstraint(ref);

    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        Node* src = a->src;
        Node* dest = a->dest;

        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;
        Flow_P_From -= a->g*(a->w_line_ij)/pow(a->tr,2.);
        Flow_P_From += a->g/a->tr*a->wr;
        Flow_P_From += a->b/a->tr*a->wi;
        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);

        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To -= a->g*(a->w_line_ji);
        Flow_P_To += a->g/a->tr*a->wr;
        Flow_P_To -= a->b/a->tr*a->wi;
        Flow_P_To = 0;
        _model->addConstraint(Flow_P_To);

        Constraint Flow_Q_From("Flow_Q_From"+a->qi._name);
        Flow_Q_From += a->qi;
        Flow_Q_From += (a->ch/2+a->b)*(a->w_line_ij)/pow(a->tr,2.);
        Flow_Q_From -= a->b/a->tr*a->wr;
        Flow_Q_From += a->g/a->tr*a->wi;
        Flow_Q_From = 0;
        _model->addConstraint(Flow_Q_From);

        Constraint Flow_Q_To("Flow_Q_To"+a->qj._name);
        Flow_Q_To += a->qj;
        Flow_Q_To += (a->ch/2+a->b)*(a->w_line_ji);
        Flow_Q_To -= a->b/a->tr*a->wr;
        Flow_Q_To -= a->g/a->tr*a->wi;
        Flow_Q_To = 0;
        _model->addConstraint(Flow_Q_To);

        Constraint W_line_ij("W_line");
        W_line_ij += a->w_line_ij - src->w;
        W_line_ij = 0;
        _model->on_off(W_line_ij,a->on);
        Constraint W_line_ji("W_line");
        W_line_ji += a->w_line_ji - dest->w;
        W_line_ji = 0;
        _model->on_off(W_line_ji,a->on);

        _model->on_off(a->w_line_ij, a->on);
        _model->on_off(a->w_line_ji, a->on);

        Constraint Delta("Delta");
        Delta += a->delta - src->theta + dest->theta + a->as;
        Delta = 0;
        _model->addConstraint(Delta);

        _model->on_off(a->delta,a->on);

        Constraint Sn_UBound("Sn_UBound"+a->_name);
        Sn_UBound += a->sn;
        Constraint Sn_LBound("Sn_LBound"+a->_name);
        Sn_LBound += a->sn;
//        double *x = new double[_model->get_nb_vars()];
        double tm, tl = a->tbound.min - a->as, tu = a->tbound.max - a->as;
        double A;
        double delta = (sin(tu) - sin(tl))/(tu - tl);
        if (tl < 0 && tu > 0) {
            double theta_m = max(-tl, tu);
            Sn_UBound -= -cos(theta_m/2)*(theta_m/2-a->delta)+sin(theta_m/2);
            Sn_LBound -= cos(theta_m/2)*(theta_m/2+a->delta)-sin(theta_m/2);
            Sn_UBound <= 0;
            _model->on_off(Sn_UBound,a->on);
            Sn_LBound >= 0;
            _model->on_off(Sn_LBound,a->on);
        }else if(tl >= 0){
//            Function Sin;
//            Sin += sin(a->delta);
//            x[a->delta.get_idx()] = (tu+tl)/2;
//            Sn_UBound -= Sin.outer_approx(x);
//            Sn_UBound <= 0;
//            _model->on_off(Sn_UBound,a->on);

            if (quad_cos) {
                tm = tu + 0.001;
                double M, M1, M2;
                M = max(_net->m_theta_ub, -_net->m_theta_lb);
                if(tu + tl < 0) M1 = _net->m_theta_ub;
                else M1 = _net->m_theta_lb;
                if(delta < 0) M2 = _net->m_theta_ub;
                else M2 = _net->m_theta_lb;
                A = (sin(tm) - sin(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl);
                Sn_UBound -= A*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl) + delta*(-tu + a->delta) + sin(tu);
                Sn_UBound += (1-a->on)*(A*(M*M - (tu+tl)*M1 + tu*tl) + delta*(M2 - tu) + sin(tu));
                Sn_UBound <= 0;
                _model->addConstraint(Sn_UBound);
            }

            Sn_LBound -= -(sin(tl)-sin(tu))*(tl-a->delta)/(tl-tu) + sin(tl);
            Sn_LBound >= 0;
            _model->on_off(Sn_LBound,a->on);

            if (lin_cos_cuts) add_SineCutsPos(a);

        }else if(tu <= 0){
//            Function Sin;
//            Sin += sin(a->delta);
//            x[a->delta.get_idx()] = (tl+tu)/2;
//            Sn_LBound -= Sin.outer_approx(x);
//            Sn_LBound >= 0;
//            _model->on_off(Sn_LBound,a->on);

            Sn_UBound -= -(sin(tl)-sin(tu))*(tl-a->delta)/(tl-tu) + sin(tl);
            Sn_UBound <= 0;
            _model->on_off(Sn_UBound,a->on);

            if (quad_cos) {
                tm = tl - 0.001;
                double M, M1, M2;
                M = max(_net->m_theta_ub, -_net->m_theta_lb);
                if(tu + tl < 0) M1 = _net->m_theta_ub;
                else M1 = _net->m_theta_lb;
                if(delta < 0) M2 = _net->m_theta_lb;
                else M2 = _net->m_theta_ub;
                A = (sin(tm) - sin(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl);
                Sn_LBound -= A*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl) + delta*(-tu + a->delta) + sin(tu);
                Sn_LBound += (1-a->on)*(A*(M*M - (tu+tl)*M1 + tu*tl) + delta*(M2 - tu) + sin(tu));
                Sn_LBound >= 0;
                _model->addConstraint(Sn_LBound);
            }

            if (lin_cos_cuts) add_SineCutsNeg(a);
        }

        _model->on_off(a->sn,a->on);


        if (lin_cos_cuts) add_Cosine(a);

        if (quad_cos) {
            double theta_u;
            theta_u = max(-tl,tu);
            double M = max(_net->m_theta_ub, -_net->m_theta_lb);

            delta = (cos(tu) - cos(tl))/(tu - tl);

            if (tl >= 0) tm = tu + 0.001;
            else if (tu <= 0) tm = tl - 0.001;
            else tm = 0;

            Constraint Cs_UB("Cs_UB");
//            Cs_UB += a->cs + (1-cos(theta_u))/pow(theta_u,2)*a->delta*a->delta;
//            Cs_UB -= pow(M,2)*(1-cos(theta_u))/pow(theta_u,2) - (pow(M,2)*(1-cos(theta_u))/pow(theta_u,2))*a->on + a->on;
//            Cs_UB <= 0;

            A = (cos(tm) - cos(tu) - delta*(tm - tu))/(tm*tm - tm*(tu+tl) + tu*tl);

            double M1, M2;

            if(tu + tl < 0) M1 = _net->m_theta_ub;
            else M1 = _net->m_theta_lb;

            if(delta < 0) M2 = _net->m_theta_ub;
            else M2 = _net->m_theta_lb;

            Cs_UB += a->cs;
            Cs_UB -= A*(a->delta*a->delta - (tu+tl)*a->delta + tu*tl);
            Cs_UB -= delta*(-tu + a->delta) + cos(tu);
            Cs_UB += (1-a->on)*(A*(M*M - (tu+tl)*M1 + tu*tl) + delta*(M2 - tu) + cos(tu));
            Cs_UB <= 0;

            _model->addConstraint(Cs_UB);
        }

        Constraint Cs_LB("Cs_LB");
        Cs_LB += a->cs;
        Cs_LB -= (cos(tu)-cos(tl))*(tl-a->delta)/(tl-tu) + cos(tl);
        Cs_LB >= 0;
        _model->on_off(Cs_LB, a->on);

        _model->on_off(a->cs,a->on);

        _model->on_off(a->vv,a->on);

        _model->add_on_off_McCormick("VV", a->vv, src->v, dest->v, a->on);

        _model->on_off(a->wr,a->on);

        _model->add_on_off_McCormick("WR", a->wr, a->cs, a->vv, a->on);

        _model->on_off(a->wi,a->on);

        _model->add_on_off_McCormick("WI", a->wi, a->sn, a->vv, a->on);

        Constraint soc("soc");
        soc += a->wr*a->wr + a->wi*a->wi;
        soc -= a->src->w*a->dest->w;
        soc <= 0;
        _model->addConstraint(soc);

        Constraint soc_bis("soc_bis");
        soc_bis += a->wr*a->wr + a->wi*a->wi;
        soc_bis -= a->on*a->src->w*pow(a->dest->vbound.max,2);
        soc_bis <= 0;
        _model->addConstraint(soc_bis);

        Constraint soc_bis2("soc_bis2");
        soc_bis2 += a->wr*a->wr + a->wi*a->wi;
        soc_bis2 -= a->on*a->dest->w*pow(a->src->vbound.max,2);
        soc_bis2 <= 0;
        _model->addConstraint(soc_bis2);

        Constraint Theta_Delta_UB_W("Theta_Delta_UB_W");
        Theta_Delta_UB_W += a->wi - tan(tu)*a->wr;
        Theta_Delta_UB_W <= 0;
        _model->addConstraint(Theta_Delta_UB_W);
        Constraint Theta_Delta_LB_W("Theta_Delta_LB_W");
        Theta_Delta_LB_W += a->wi - tan(tl)*a->wr;
        Theta_Delta_LB_W >= 0;
        _model->addConstraint(Theta_Delta_LB_W);

        Constraint Current_Magnitude_From("Current_Magnitude_From");
        Current_Magnitude_From += a->pi*a->pi + a->qi*a->qi;
        Current_Magnitude_From -= src->w*a->ci/pow(a->tr, 2.);
        Current_Magnitude_From <= 0;
        _model->addConstraint(Current_Magnitude_From);
        Constraint Current_Magnitude_From_bis("Current_Magnitude_From_bis");
        Current_Magnitude_From_bis += a->pi*a->pi + a->qi*a->qi;
        Current_Magnitude_From_bis -= pow(src->vbound.max,2)*a->ci*a->on/pow(a->tr, 2.);
        Current_Magnitude_From_bis <= 0;
        _model->addConstraint(Current_Magnitude_From_bis);

        Constraint P_Loss_From("P_Loss_From");
        P_Loss_From += a->r*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*a->w_line_ij);
        P_Loss_From -= a->pi + a->pj;
        P_Loss_From = 0;
        _model->addConstraint(P_Loss_From);
        Constraint Q_Loss_From("Q_Loss_From");
        Q_Loss_From += a->x*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*a->w_line_ij) - ((a->ch/2)/pow(a->tr,2))*a->w_line_ij - (a->ch/2)*a->w_line_ji;
        Q_Loss_From -= a->qi + a->qj;
        Q_Loss_From = 0;
        _model->addConstraint(Q_Loss_From);

        Constraint Thermal_Limit_from("Thermal_Limit_from");
        Thermal_Limit_from += a->_Si_.square_magnitude();
        Thermal_Limit_from -= pow(a->limit,2.)*a->on*a->on;
        Thermal_Limit_from <= 0;
        _model->addConstraint(Thermal_Limit_from);

        Constraint Thermal_Limit_to("Thermal_Limit_to");
        Thermal_Limit_to += a->_Sj_.square_magnitude();
        Thermal_Limit_to -= pow(a->limit,2.)*a->on*a->on;
        Thermal_Limit_to <= 0;
        _model->addConstraint(Thermal_Limit_to);
    }

    for (auto n:_net->nodes) {
        Constraint KCL_P("KCL_P");
        KCL_P += sum(n->get_out(), "pi",false);
        KCL_P += sum(n->get_in(), "pj",false);
        KCL_P -= sum(n->_gen, "pg");
        KCL_P += n->gs()*n->w + n->pl();
        KCL_P = 0;
        _model->addConstraint(KCL_P);

        Constraint KCL_Q("KCL_Q");
        KCL_Q += sum(n->get_out(), "qi",false);
        KCL_Q += sum(n->get_in(), "qj",false);
        KCL_Q -= sum(n->_gen, "qg");
        KCL_Q -= n->bs()*n->w - n->ql();
        KCL_Q = 0;
        _model->addConstraint(KCL_Q);

        /** Quadratic */
        Constraint W_UBound("W_UBound");
        W_UBound += n->w;
        W_UBound -= (n->vbound.max+n->vbound.min)*n->v - n->vbound.max*n->vbound.min;
        W_UBound <= 0;
        _model->addConstraint(W_UBound);
        Constraint W_LBound("W_LBound");
        W_LBound += n->w;
        W_LBound -= n->v*n->v;
        W_LBound >= 0;
        _model->addConstraint(W_LBound);
    }
}

//void PowerModel::post_QC_OTS(bool lin_cos_cuts, bool quad_cos){
//    Constraint ref("t_ref");
//    ref += _net->arcs.front()->src->theta;
//    ref = 0;
//    _model->addConstraint(ref);
//
//    for (auto a:_net->arcs) {
//        if (a->status==0) {
//            continue;
//        }
//        Node* src = a->src;
//        Node* dest = a->dest;
//
//        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
//        Flow_P_From += a->pi;
//        Flow_P_From -= a->g*(a->w_line_ij)/pow(a->tr,2.);
//        Flow_P_From += a->g/a->tr*a->wr;
//        Flow_P_From += a->b/a->tr*a->wi;
//        Flow_P_From = 0;
//        _model->addConstraint(Flow_P_From);
//
//        Constraint Flow_P_To("Flow_P_To"+a->pj._name);
//        Flow_P_To += a->pj;
//        Flow_P_To -= a->g*(a->w_line_ji);
//        Flow_P_To += a->g/a->tr*a->wr;
//        Flow_P_To -= a->b/a->tr*a->wi;
//        Flow_P_To = 0;
//        _model->addConstraint(Flow_P_To);
//
//        Constraint Flow_Q_From("Flow_Q_From"+a->qi._name);
//        Flow_Q_From += a->qi;
//        Flow_Q_From += (a->ch/2+a->b)*(a->w_line_ij)/pow(a->tr,2.);
//        Flow_Q_From -= a->b/a->tr*a->wr;
//        Flow_Q_From += a->g/a->tr*a->wi;
//        Flow_Q_From = 0;
//        _model->addConstraint(Flow_Q_From);
//
//        Constraint Flow_Q_To("Flow_Q_To"+a->qj._name);
//        Flow_Q_To += a->qj;
//        Flow_Q_To += (a->ch/2+a->b)*(a->w_line_ji);
//        Flow_Q_To -= a->b/a->tr*a->wr;
//        Flow_Q_To -= a->g/a->tr*a->wi;
//        Flow_Q_To = 0;
//        _model->addConstraint(Flow_Q_To);
//
//        Constraint W_line_ij("W_line");
//        W_line_ij += a->w_line_ij - src->w;
//        W_line_ij = 0;
//        _model->on_off(W_line_ij,a->on);
//        Constraint W_line_ji("W_line");
//        W_line_ji += a->w_line_ji - dest->w;
//        W_line_ji = 0;
//        _model->on_off(W_line_ji,a->on);
//
//        _model->on_off(a->w_line_ij, a->on);
//        _model->on_off(a->w_line_ji, a->on);
//
//        Constraint Delta("Delta");
//        Delta += a->delta - src->theta + dest->theta;
//        Delta = 0;
//        _model->addConstraint(Delta);
//
//        _model->on_off(a->delta,a->on);
//
//        Constraint Sn_UBound("Sn_UBound"+a->_name);
//        Sn_UBound += a->sn;
//        Constraint Sn_LBound("Sn_LBound"+a->_name);
//        Sn_LBound += a->sn;
//        double *x = new double[_model->get_nb_vars()];
//        if (a->tbound.min < 0 && a->tbound.max > 0) {
//            double theta_m = max(-a->tbound.min, a->tbound.max);
//            Sn_UBound -= -cos(theta_m/2)*(theta_m/2-a->delta)+sin(theta_m/2);
//            Sn_LBound -= cos(theta_m/2)*(theta_m/2+a->delta)-sin(theta_m/2);
//        }else if(a->tbound.min >= 0){
//            Function Sin;
//            Sin += sin(a->delta);
//            x[a->delta.get_idx()] = (a->tbound.max+a->tbound.min)/2;
//            Sn_UBound -= Sin.outer_approx(x);
//            Sn_LBound -= -(sin(a->tbound.min)-sin(a->tbound.max))*(a->tbound.min-a->delta)/(a->tbound.min-a->tbound.max) + sin(a->tbound.min);
//
//            //            Constraint Sn_UBoundR("Sn_UBoundR");
//            //            Sn_UBoundR += a->sn;
//            //            x[a->delta.get_idx()] = a->tbound.max;
//            //            Sn_UBoundR -= Sin.outer_approx(x);
//            //            Sn_UBoundR <= 0;
//            //            _model->on_off(Sn_UBoundR,a->on);
//            //            Constraint Sn_UBoundL("Sn_UBoundL");
//            //            Sn_UBoundL += a->sn;
//            //            x[a->delta.get_idx()] = a->tbound.min;
//            //            Sn_UBoundL -= Sin.outer_approx(x);
//            //            Sn_UBoundL <= 0;
//            //            _model->on_off(Sn_UBoundL,a->on);
//        }else if(a->tbound.max <= 0){
//            Function Sin;
//            Sin += sin(a->delta);
//            x[a->delta.get_idx()] = (a->tbound.min+a->tbound.max)/2;
//            Sn_UBound -= -(sin(a->tbound.min)-sin(a->tbound.max))*(a->tbound.min-a->delta)/(a->tbound.min-a->tbound.max) + sin(a->tbound.min);
//            Sn_LBound -= Sin.outer_approx(x);
//
//            //            Constraint Sn_LBoundR("Sn_LBoundR");
//            //            Sn_LBoundR += a->sn;
//            //            x[a->delta.get_idx()] = a->tbound.max;
//            //            Sn_LBoundR -= Sin.outer_approx(x);
//            //            Sn_LBoundR >= 0;
//            //            _model->on_off(Sn_LBoundR,a->on);
//            //            Constraint Sn_LBoundL("Sn_LBoundL");
//            //            Sn_LBoundL += a->sn;
//            //            x[a->delta.get_idx()] = a->tbound.min;
//            //            Sn_LBoundL -= Sin.outer_approx(x);
//            //            Sn_LBoundL >= 0;
//            //            _model->on_off(Sn_LBoundL,a->on);
//        }
//        Sn_UBound <= 0;
//        _model->on_off(Sn_UBound,a->on);
//        Sn_LBound >= 0;
//        _model->on_off(Sn_LBound,a->on);
//
//        _model->on_off(a->sn,a->on);
//
//
//        if (lin_cos_cuts) add_Cosine(a);
//
//        if (quad_cos) {
//            double theta_u;
//            theta_u = max(-a->tbound.min,a->tbound.max);
//            double M = max(_net->m_theta_ub, -_net->m_theta_lb);
//
//            Constraint Cs_UB("Cs_UB");
//            Cs_UB += a->cs + (1-cos(theta_u))/pow(theta_u,2)*a->delta*a->delta;
//            Cs_UB -= pow(M,2)*(1-cos(theta_u))/pow(theta_u,2) - (pow(M,2)*(1-cos(theta_u))/pow(theta_u,2))*a->on + a->on;
//            Cs_UB <= 0;
//            _model->addConstraint(Cs_UB);
//        }
//
//        Constraint Cs_LB("Cs_LB");
//        Cs_LB += a->cs;
//        Cs_LB -= (cos(a->tbound.max)-cos(a->tbound.min))*(a->tbound.min-a->delta)/(a->tbound.min-a->tbound.max) + cos(a->tbound.min);
//        Cs_LB >= 0;
//        _model->on_off(Cs_LB, a->on);
//
//        _model->on_off(a->cs,a->on);
//
//        _model->on_off(a->vv,a->on);
//
//        _model->add_on_off_McCormick("VV", a->vv, src->v, dest->v, a->on);
//
//        _model->on_off(a->wr,a->on);
//
//        _model->add_on_off_McCormick("WR", a->wr, a->cs, a->vv, a->on);
//
//        _model->on_off(a->wi,a->on);
//
//        _model->add_on_off_McCormick("WI", a->wi, a->sn, a->vv, a->on);
//
//        Constraint soc("soc");
//        soc += a->wr*a->wr + a->wi*a->wi;
//        soc -= a->src->w*a->dest->w;
//        soc <= 0;
//        _model->addConstraint(soc);
//
//        Constraint Theta_Delta_UB_W("Theta_Delta_UB_W");
//        Theta_Delta_UB_W += a->wi - tan(a->tbound.max)*a->wr;
//        Theta_Delta_UB_W <= 0;
//        _model->addConstraint(Theta_Delta_UB_W);
//        Constraint Theta_Delta_LB_W("Theta_Delta_LB_W");
//        Theta_Delta_LB_W += a->wi - tan(a->tbound.min)*a->wr;
//        Theta_Delta_LB_W >= 0;
//        _model->addConstraint(Theta_Delta_LB_W);
//
//        Constraint Current_Magnitude_From("Current_Magnitude_From");
//        Current_Magnitude_From += a->pi*a->pi + a->qi*a->qi;
//        Current_Magnitude_From -= src->w*a->ci/pow(a->tr, 2.);
//        Current_Magnitude_From <= 0;
//        _model->addConstraint(Current_Magnitude_From);
//        Constraint Current_Magnitude_From_bis("Current_Magnitude_From_bis");
//        Current_Magnitude_From_bis += a->pi*a->pi + a->qi*a->qi;
//        Current_Magnitude_From_bis -= pow(src->vbound.max,2)*a->ci*a->on/pow(a->tr, 2.);
//        Current_Magnitude_From_bis <= 0;
//        _model->addConstraint(Current_Magnitude_From_bis);
//
//        Constraint P_Loss_From("P_Loss_From");
//        P_Loss_From += a->r*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*a->w_line_ij);
//        P_Loss_From -= a->pi + a->pj;
//        P_Loss_From = 0;
//        _model->addConstraint(P_Loss_From);
//        Constraint Q_Loss_From("Q_Loss_From");
//        Q_Loss_From += a->x*(a->ci + a->ch*a->qi + pow((a->ch/2)/a->tr,2.)*a->w_line_ij) - ((a->ch/2)/pow(a->tr,2))*a->w_line_ij - (a->ch/2)*a->w_line_ji;
//        Q_Loss_From -= a->qi + a->qj;
//        Q_Loss_From = 0;
//        _model->addConstraint(Q_Loss_From);
//
//        Constraint Thermal_Limit_from("Thermal_Limit_from");
//        Thermal_Limit_from += a->_Si_.square_magnitude();
//        Thermal_Limit_from -= pow(a->limit,2.)*a->on*a->on;
//        Thermal_Limit_from <= 0;
//        _model->addConstraint(Thermal_Limit_from);
//
//        Constraint Thermal_Limit_to("Thermal_Limit_to");
//        Thermal_Limit_to += a->_Sj_.square_magnitude();
//        Thermal_Limit_to -= pow(a->limit,2.)*a->on*a->on;
//        Thermal_Limit_to <= 0;
//        _model->addConstraint(Thermal_Limit_to);
//    }
//
//    for (auto n:_net->nodes) {
//        Constraint KCL_P("KCL_P");
//        KCL_P += sum(n->get_out(), "pi",false);
//        KCL_P += sum(n->get_in(), "pj",false);
//        KCL_P -= sum(n->_gen, "pg");
//        KCL_P += n->gs()*n->w + n->pl();
//        KCL_P = 0;
//        _model->addConstraint(KCL_P);
//
//        Constraint KCL_Q("KCL_Q");
//        KCL_Q += sum(n->get_out(), "qi",false);
//        KCL_Q += sum(n->get_in(), "qj",false);
//        KCL_Q -= sum(n->_gen, "qg");
//        KCL_Q -= n->bs()*n->w - n->ql();
//        KCL_Q = 0;
//        _model->addConstraint(KCL_Q);
//
//        /** Quadratic */
//        Constraint W_UBound("W_UBound");
//        W_UBound += n->w;
//        W_UBound -= (n->vbound.max+n->vbound.min)*n->v - n->vbound.max*n->vbound.min;
//        W_UBound <= 0;
//        _model->addConstraint(W_UBound);
//        Constraint W_LBound("W_LBound");
//        W_LBound += n->w;
//        W_LBound -= n->v*n->v;
//        W_LBound >= 0;
//        _model->addConstraint(W_LBound);
//    }
//}

void PowerModel::propagate_bounds(){
    if (_model != NULL) {
        cerr << "\npropagate_bounds should be called before building the model.";
        exit(1);
    }
    var<double>* vr;
    double tol = 0.01;
    double* tolsl = new double[_net->arcs.size()];
    double* tolsu = new double[_net->arcs.size()];
    double* tolsnl = new double[_net->nodes.size()];
    double* tolsnu = new double[_net->nodes.size()];
    double maxtol = 1;
    int i = 0;

    for (i = 0; i < _net->arcs.size(); i++) {
        tolsl[i] = 1;
        tolsu[i] = 1;
    }

    for (i = 0; i < _net->nodes.size(); i++) {
        tolsnl[i] = 1;
        tolsnu[i] = 1;
    }
    _solver = new PTSolver(NULL, _stype);

    while(maxtol > tol) {
        cout << "\nmaxtol = " << maxtol << "\n";
        _model = new Model();
//        _solver = new PTSolver(_model,_stype);
        _solver->set_model(_model);
        maxtol = 0;
        if (_type == QC_OTS_N || _type == QC_OTS_O || _type == QC_OTS_L) {
            add_QC_OTS_vars();
            if (_type == QC_OTS_N) post_QC_OTS(true, true);
            if (_type == QC_OTS_O) post_QC_OTS(false, true);
            if (_type == QC_OTS_L) post_QC_OTS(true, false);
        }else if (_type == QC) {
            add_QC_vars();
            post_QC();
        }

        i = 0;
        for (auto& a: _net->arcs) {
            //if (!a->src->_has_gen && !a->dest->_has_gen) {i++; continue;}
            if (tolsl[i] > tol) {
                if (_type == QC_OTS_N || _type == QC_OTS_O || _type == QC_OTS_L) a->on.set_lb(true);
                min_var(a->delta);
                solve(0,true);
                vr = _model->getVar_<double>(a->delta.get_idx());
                a->tbound.min = vr->get_value();
                tolsl[i] = a->tbound.min-vr->get_lb();
                a->tbound.min += a->as;
                if (tolsl[i] > maxtol) maxtol = tolsl[i];
                if (_type == QC_OTS_N || _type == QC_OTS_O || _type == QC_OTS_L) a->on.set_lb(false);
            }
            if(tolsu[i]>tol) {
                if (_type == QC_OTS_N || _type == QC_OTS_O || _type == QC_OTS_L) a->on.set_lb(true);
                max_var(a->delta);
                solve(0,true);
                vr = _model->getVar_<double>(a->delta.get_idx());
                a->tbound.max = vr->get_value();
                tolsu[i] = vr->get_ub()-a->tbound.max;
                a->tbound.max += a->as;
                if (tolsu[i] > maxtol) maxtol = tolsu[i];
                if (_type == QC_OTS_N || _type == QC_OTS_O || _type == QC_OTS_L) a->on.set_lb(false);
            }
            i++;
        }

        i = 0;
        for(auto& n: _net->nodes){
//            if (!n->_has_gen) {i++; continue;}
            if(tolsnl[i] > tol) {
                min_var(n->v);
                solve(0,true);
                vr = _model->getVar_<double>(n->v.get_idx());
                n->vbound.min = vr->get_value();
                tolsnl[i] = n->vbound.min-vr->get_lb();
                if (tolsnl[i] > maxtol) maxtol = tolsnl[i];
            }
            if(tolsnu[i] > tol) {
                max_var(n->v);
                solve(0,true);
                vr = _model->getVar_<double>(n->v.get_idx());
                n->vbound.max = vr->get_value();
                tolsnu[i] = n->vbound.max-vr->get_ub();
                if (tolsnu[i] > maxtol) maxtol = tolsnu[i];
            }
            i++;
        }

/*        cout << endl << "New bounds, maxtol = " << maxtol << ":";
        for (auto& a: _net->arcs) {
            cout << "\nmin = " << a->tbound.min << " max = " << a->tbound.max;
        }
        for (auto& n: _net->nodes) {
            cout << "\nvmin = " << n->vbound.min << " vmax = " << n->vbound.max;
        }*/
        delete _model;
    }
    cout << endl << "New bounds: ";
    i = 0;
    for (auto& a: _net->arcs) {
        cout << "\nmin = " << a->tbound.min << " max = " << a->tbound.max;
        i++;
    }
    delete[] tolsl;
    delete[] tolsu;
}

int PowerModel::get_order(vector<Node*>* bag, int order){
    Node* src = nullptr;
    switch (order) {
        case 1:
            src = bag->at(0);
            if (_net->_clone->has_directed_arc(src, bag->at(1)) &&  _net->_clone->has_directed_arc(src, bag->at(2))){
                    return 0;
            }
            src = bag->at(1);
            if (_net->_clone->has_directed_arc(src, bag->at(0)) &&  _net->_clone->has_directed_arc(src, bag->at(2))){
                return 1;
            }
            return 2;
            break;
        case 2:
            src = bag->at(0);
            if (_net->_clone->has_directed_arc(bag->at(1), src) &&  _net->_clone->has_directed_arc(bag->at(2), src)){
                return 0;
            }
            src = bag->at(1);
            if (_net->_clone->has_directed_arc(bag->at(0), src) &&  _net->_clone->has_directed_arc(bag->at(2), src)){
                return 1;
            }
            return 2;
            break;
        case 3:
            src = bag->at(0);
            if (_net->_clone->has_directed_arc(src, bag->at(1)) &&  _net->_clone->has_directed_arc(bag->at(2), src)){
                return 0;
            }
            if (_net->_clone->has_directed_arc(bag->at(1), src) &&  _net->_clone->has_directed_arc(src, bag->at(2))){
                return 0;
            }
            src = bag->at(1);
            if (_net->_clone->has_directed_arc(src, bag->at(0)) &&  _net->_clone->has_directed_arc(bag->at(2), src)){
                return 1;
            }
            if (_net->_clone->has_directed_arc(bag->at(0), src) &&  _net->_clone->has_directed_arc(src, bag->at(2))){
                return 1;
            }
            return 2;
            break;
            
        default:
            assert(false);
            return -1;
            break;
    }
}

void PowerModel::add_SDP_cuts(int dim){
    assert(dim==3);
    /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
//     */
    meta_var* wr12 = new meta_var("wr12", _model);
    meta_var* wr13 = new meta_var("wr13", _model);
    meta_var* wr31 = new meta_var("wr31", _model);
    meta_var* wr32 = new meta_var("wr32", _model);
    meta_var* wr23 = new meta_var("wr23", _model);
    meta_var* wi12 = new meta_var("wi12", _model);
    meta_var* wi13 = new meta_var("wi13", _model);
    meta_var* wi31 = new meta_var("wi31", _model);
    meta_var* wi23 = new meta_var("wi23", _model);
    meta_var* wi32 = new meta_var("wi32", _model);
    meta_var* w1 = new meta_var("w1", _model);
    meta_var* w2 = new meta_var("w2", _model);
    meta_var* w3 = new meta_var("w3", _model);
    
    meta_Constraint* SDP = new meta_Constraint();
    *SDP = (*wr12)*((*wr32)*(*wr13) - (*wi32)*(*wi13));
    *SDP += (*wi12)*((*wi32)*(*wr13) + (*wr32)*(*wi13));
    *SDP *= 2;
    *SDP -= (((*wr12)^2)+((*wi12)^2))*(*w3);
    *SDP -= (((*wr13)^2)+((*wi13)^2))*(*w2);
    *SDP -= (((*wr32)^2)+((*wi32)^2))*(*w1);
    *SDP += (*w1)*(*w2)*(*w3);
    *SDP >= 0;
    _model->addMetaConstraint(*SDP);
    
    meta_Constraint* SDPr = new meta_Constraint();// Rotational SDP
    *SDPr = (*wr12)*((*wr23)*(*wr31) - (*wi23)*(*wi31));
    *SDPr -= (*wi12)*((*wi23)*(*wr31) + (*wr23)*(*wi31));
    *SDPr *= 2;
    *SDPr -= (((*wr12)^2)+((*wi12)^2))*(*w3);
    *SDPr -= (((*wr31)^2)+((*wi31)^2))*(*w2);
    *SDPr -= (((*wr23)^2)+((*wi23)^2))*(*w1);
    *SDPr += (*w1)*(*w2)*(*w3);
    *SDPr >= 0;
    _model->addMetaConstraint(*SDPr);
    
    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;
    _model->init_functions(_net->_bags->size()+1);
    _model->print_functions();
    
    int id = 0;
    int n_i1, n_i2, n_i3 = 0;
    bool rot_bag = false;
    for (auto b: *_net->_bags){
//        if (id>=1) {
//            continue;
//        }
//        string ok;
        if (b->size()==dim) {
            rot_bag = _net->rotation_bag(b);
            if (rot_bag) {
//                cout << "rot_bag: ";
                n1 = b->at(0);
                if (_net->_clone->has_directed_arc(n1, b->at(1))){
                    n2 = b->at(1);
                    n3 = b->at(2);
                }
                else {
                    n2 = b->at(2);
                    n3 = b->at(1);
                }
                a13 = _net->get_arc(n3,n1);
                a32 = _net->get_arc(n2,n3);
            }
            else {
                n_i1 = get_order(b,1);
                n1 = b->at(n_i1);
                n_i2 = get_order(b,2);
                n2 = b->at(n_i2);
                n_i3 = get_order(b,3);
                n3 = b->at(n_i3);
                a13 = _net->get_arc(n1,n3);
                a32 = _net->get_arc(n3,n2);
            }
            a12 = _net->get_arc(n1,n2);
            
            if(!a12 || a12->status==0) {
//                continue;
                if (!a12) {
                    a12 = _net->_clone->get_arc(n1,n2)->clone();
                    a12->status=1;
                    a12->src = _net->get_node(a12->src->_name);
                    a12->dest = _net->get_node(a12->dest->_name);
                    a12->connect();
                    _net->add_arc(a12);
                }
//                else {
//                    assert(false);
//                }
                if (a12->tbound.min < 0 && a12->tbound.max > 0)
                    a12->cs.init("cs("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",min(cos(a12->tbound.min), cos(a12->tbound.max)), 1.);
                else
                    a12->cs.init("cs("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",min(cos(a12->tbound.min), cos(a12->tbound.max)), max(cos(a12->tbound.min),cos(a12->tbound.max)));
                a12->vv.init("vv("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->src->vbound.min*a12->dest->vbound.min,a12->src->vbound.max*a12->dest->vbound.max);
                if (_type==QC_SDP) {
                    a12->vcs.init("vcs("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*a12->cs.get_lb(), a12->vv.get_ub()*a12->cs.get_ub());
                    if(a12->tbound.min < 0 && a12->tbound.max > 0)
                        a12->vsn.init("vsn("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.min >= 0)
                        a12->vsn.init("vsn("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.max <= 0)
                        a12->vsn.init("vsn("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_lb()*sin(a12->tbound.max));
                    _model->addVar(a12->vcs);
                    a12->vcs = 1;
                    _model->addVar(a12->vsn);
                }
                else {
                    a12->wr.init("wr("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*a12->cs.get_lb(), a12->vv.get_ub()*a12->cs.get_ub());
                    if(a12->tbound.min < 0 && a12->tbound.max > 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.min >= 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.max <= 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_lb()*sin(a12->tbound.max));
                    a12->wr = 1;
                    _model->addVar(a12->wr);
                    _model->addVar(a12->wi);
                }
                Constraint SOCP1("SOCP1");
                SOCP1 += _net->get_node(n1->_name)->w*_net->get_node(n2->_name)->w;
                if (_type==QC_SDP) {
                    SOCP1 -= ((a12->vcs)^2);
                    SOCP1 -= ((a12->vsn)^2);
                }
                else{
                    SOCP1 -= ((a12->wr)^2);
                    SOCP1 -= ((a12->wi)^2);
                }
                SOCP1 >= 0;
                _model->addConstraint(SOCP1);
//                SOCP1.print();
//                _net->_clone->remove_arc(a12);
            }
            if(!a13 || a13->status==0) {
                if(!a13) {
//                    continue;
                    if (rot_bag) {
                        a13 = _net->_clone->get_arc(n3,n1)->clone();
                    }
                    else {
                        a13 = _net->_clone->get_arc(n1,n3)->clone();
                    }
                    a13->status=1;
                    a13->src = _net->get_node(a13->src->_name);
                    a13->dest = _net->get_node(a13->dest->_name);
                    a13->connect();
                    _net->add_arc(a13);
                }
//                else {
//                    assert(false);
//                }
                if (a13->tbound.min < 0 && a13->tbound.max > 0)
                    a13->cs.init("cs("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",min(cos(a13->tbound.min), cos(a13->tbound.max)), 1.);
                else
                    a13->cs.init("cs("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",min(cos(a13->tbound.min), cos(a13->tbound.max)), max(cos(a13->tbound.min),cos(a13->tbound.max)));
                a13->vv.init("vv("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->src->vbound.min*a13->dest->vbound.min,a13->src->vbound.max*a13->dest->vbound.max);

                if (_type==QC_SDP) {
                    a13->vcs.init("vcs("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*a13->cs.get_lb(), a13->vv.get_ub()*a13->cs.get_ub());
                    if(a13->tbound.min < 0 && a13->tbound.max > 0)
                        a13->vsn.init("vsn("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.min >= 0)
                        a13->vsn.init("vsn("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.max <= 0)
                        a13->vsn.init("vsn("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_lb()*sin(a13->tbound.max));
                    _model->addVar(a13->vcs);
                    a13->vcs = 1;
                    _model->addVar(a13->vsn);
                }
                else {
                    a13->wr.init("wr("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*a13->cs.get_lb(), a13->vv.get_ub()*a13->cs.get_ub());
                    if(a13->tbound.min < 0 && a13->tbound.max > 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.min >= 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.max <= 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_lb()*sin(a13->tbound.max));
                    a13->wr = 1;
                    _model->addVar(a13->wr);
                    _model->addVar(a13->wi);
                }
                Constraint SOCP2("SOCP2");
                SOCP2 += _net->get_node(n1->_name)->w*_net->get_node(n3->_name)->w;
                if (_type==QC_SDP) {
                    SOCP2 -= ((a13->vcs)^2);
                    SOCP2 -= ((a13->vsn)^2);
                }
                else{
                    SOCP2 -= ((a13->wr)^2);
                    SOCP2 -= ((a13->wi)^2);
                }
                SOCP2 >= 0;
                _model->addConstraint(SOCP2);
                //                _net->_clone->remove_arc(a13);
            }
            if(!a32 || a32->status==0) {
                if (!a32) {
//                    continue;
                    if (rot_bag) {
                        a32 = _net->_clone->get_arc(n2,n3)->clone();
                    }
                    else {
                        a32 = _net->_clone->get_arc(n3,n2)->clone();
                    }
                    a32->status=1;
                    a32->src = _net->get_node(a32->src->_name);
                    a32->dest = _net->get_node(a32->dest->_name);
                    a32->connect();
                    _net->add_arc(a32);
                }
//                else {
//                    assert(false);
//                }
                if (a32->tbound.min < 0 && a32->tbound.max > 0)
                    a32->cs.init("cs("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",min(cos(a32->tbound.min), cos(a32->tbound.max)), 1.);
                else
                    a32->cs.init("cs("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",min(cos(a32->tbound.min), cos(a32->tbound.max)), max(cos(a32->tbound.min),cos(a32->tbound.max)));
                a32->vv.init("vv("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->src->vbound.min*a32->dest->vbound.min,a32->src->vbound.max*a32->dest->vbound.max);

                if (_type==QC_SDP) {
                    a32->vcs.init("vcs("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*a32->cs.get_lb(), a32->vv.get_ub()*a32->cs.get_ub());
                    if(a32->tbound.min < 0 && a32->tbound.max > 0)
                        a32->vsn.init("vsn("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.min >= 0)
                        a32->vsn.init("vsn("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.max <= 0)
                        a32->vsn.init("vsn("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_lb()*sin(a32->tbound.max));
                    _model->addVar(a32->vcs);
                    a32->vcs = 1;
                    _model->addVar(a32->vsn);
                }
                else {
                    a32->wr.init("wr("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*a32->cs.get_lb(), a32->vv.get_ub()*a32->cs.get_ub());
                    if(a32->tbound.min < 0 && a32->tbound.max > 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.min >= 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.max <= 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_lb()*sin(a32->tbound.max));
                    a32->wr = 1;
                    _model->addVar(a32->wr);
                    _model->addVar(a32->wi);
                }
                Constraint SOCP3("SOCP3");
                SOCP3 += _net->get_node(n3->_name)->w*_net->get_node(n2->_name)->w;
                if (_type==QC_SDP) {
                    SOCP3 -= ((a32->vcs)^2);
                    SOCP3 -= ((a32->vsn)^2);
                }
                else{
                    SOCP3 -= ((a32->wr)^2);
                    SOCP3 -= ((a32->wi)^2);
                }
                SOCP3 >= 0;
                _model->addConstraint(SOCP3);
                //                _net->_clone->remove_arc(a32);
            }

            if (_type==QC_SDP) {
                *wr12 = a12->vcs;
                *wi12 = a12->vsn;
                *wr13 = a13->vcs;
                *wi13 = a13->vsn;
                *wr31 = a13->vcs;
                *wi31 = a13->vsn;
                *wr32 = a32->vcs;
                *wi32 = a32->vsn;
                *wr23 = a32->vcs;
                *wi23 = a32->vsn;
            }
            else{
                *wr12 = a12->wr;
                *wi12 = a12->wi;
                *wr13 = a13->wr;
                *wi13 = a13->wi;
                *wr31 = a13->wr;
                *wi31 = a13->wi;
                *wr32 = a32->wr;
                *wi32 = a32->wi;
                *wr23 = a32->wr;
                *wi23 = a32->wi;
            }
            *w1 = _net->get_node(n1->_name)->w;
            *w2 = _net->get_node(n2->_name)->w;
            *w3 = _net->get_node(n3->_name)->w;
            if (rot_bag) {
                _model->concretise(*SDPr, id, "SDP"+to_string(id));
                id++;
            }
            else {
                _model->concretise(*SDP, id, "SDP"+to_string(id));
                id++;
            }
        }
    }
    cout << "total number of 3d SDP cuts added = " << id << endl;
    
}





void PowerModel::post_SOCP_OTS(){
    Node* src = NULL;
    Node* dest = NULL;
    for (auto n:_net->nodes) {
        add_AC_KCL(n, false);
    }

    for (auto a:_net->arcs) {
        if (a->status==0) {
            continue;
        }
        src = a->src;
        dest = a->dest;

        Constraint Flow_P_From("Flow_P_From"+a->pi._name);
        Flow_P_From += a->pi;
        Flow_P_From -= (a->g/(pow(a->cc,2)+pow(a->dd,2)))*a->w_line_ij;
        Flow_P_From -= (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->wr;
        Flow_P_From -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*a->wi;
        Flow_P_From = 0;
        _model->addConstraint(Flow_P_From);
    }
}




Function PowerModel::sum(vector<Arc*> vec, string var, bool switch_lines){
    Function res;
    for (auto& a:vec) {
        if (!switch_lines) {
            if (a->status==1) {
                if (var.compare("pi")==0) {
                    res += a->pi;
                }
                if (var.compare("pj")==0) {
                    res += a->pj;
                }
                if (var.compare("qi")==0) {
                    res += a->qi;
                }
                if (var.compare("qj")==0) {
                    res += a->qj;
                }
            }
        }
        else {
            if (var.compare("pi")==0) {
                res += a->pi*a->on;
            }
            if (var.compare("pj")==0) {
                res += a->pj*a->on;
            }
            if (var.compare("qi")==0) {
                res += a->qi*a->on;
            }
            if (var.compare("qj")==0) {
                res += a->qj*a->on;
            }
        }
    }
    return res;
}


Function PowerModel::sum_Time(vector<Arc*> vec, string var, int t){
    Function res;
    for (auto& a:vec) {

            if (a->status==1) {
                if (var.compare("pi")==0) {
                    res += a->pi_t[t];
                }
                if (var.compare("pj")==0) {
                    res += a->pj_t[t];
                }
                if (var.compare("qi")==0) {
                    res += a->qi_t[t];
                }
                if (var.compare("qj")==0) {
                    res += a->qj_t[t];
                }
            }
     }
    return res;
}

Function PowerModel::sum_Time_Compact(vector<Arc*> vec, string var, int t){
    Function res;
    for (auto& a:vec) {
        Node* src = a->src;
        Node* dest = a->dest;

        if (a->status==1) {
            if (var.compare("pi")==0) {
                res += a->g*((((src->vi_t[t])^2) + ((src->vr_t[t])^2)))/pow(a->tr,2.);
                res += (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vr_t[t]*dest->vr_t[t] + src->vi_t[t]*dest->vi_t[t]);
                res += (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi_t[t]*dest->vr_t[t] - src->vr_t[t]*dest->vi_t[t]);
            }
            if (var.compare("pj")==0) {
                res += a->g*((((dest->vi_t[t])^2) + ((dest->vr_t[t])^2)));
                res += (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
                res += (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi_t[t]*src->vr_t[t] - dest->vr_t[t]*src->vi_t[t]);
            }
            if (var.compare("qi")==0) {
                res -= (a->ch/2+a->b)*((((src->vi_t[t])^2) + ((src->vr_t[t])^2)))/pow(a->tr,2.);
                res -= (-a->b*a->cc - a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
                res += (-a->g*a->cc + a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(src->vi_t[t]*dest->vr_t[t] - src->vr_t[t]*dest->vi_t[t]);
            }
            if (var.compare("qj")==0) {
                res -= (a->ch/2+a->b)*(((dest->vi_t[t])^2) + ((dest->vr_t[t])^2));
                res -= (-a->b*a->cc + a->g*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vr_t[t]*src->vr_t[t] + dest->vi_t[t]*src->vi_t[t]);
                res += (-a->g*a->cc - a->b*a->dd)/(pow(a->cc,2)+pow(a->dd,2))*(dest->vi_t[t]*src->vr_t[t] - dest->vr_t[t]*src->vi_t[t]);
            }
        }
    }
    return res;
}



Function PowerModel::sum_Time(vector<Gen*> vec, string var, int time){
    Function res;
    for (auto& g:vec) {
        if (g->_active) {
            if (var.compare("pg")==0) {
                res += g->pg_t[time];
            }
            if (var.compare("qg")==0) {
                res += g->qg_t[time];
            }
        }
    }
    return res;
}


Function PowerModel::sum(vector<Gen*> vec, string var){
    Function res;
    for (auto& g:vec) {
        if (g->_active) {
            if (var.compare("pg")==0) {
                res += g->pg;
            }
            if (var.compare("qg")==0) {
                res += g->qg;
            }
        }
    }
    return res;
}


