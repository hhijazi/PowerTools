//
//  PowerModel.h
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____PowerModel__
#define __PowerTools____PowerModel__
#include <ctime>
#include <stdio.h>
#include <PowerTools++/Model.h>
#include "PowerTools++/PTSolver.h"
#include <PowerTools++/Net.h>
#include <PowerTools++/Constraint.h>
#include <PowerTools++/meta_constant.h>
#include <PowerTools++/meta_var.h>
#include <PowerTools++/meta_Constraint.h>
#include <vector>
#include <random>

typedef enum { COPPER_PEAK, COPPER_PV_PEAK, NF, NF_PEAK, NF_PV_PEAK, ACPF_PV_PEAK, ACPF_PEAK, COMPACT_FIXED_PV, FIXED_PV, SOCP_BATT_T_NO_GEN, ACPF_BATT_T_NO_GEN, SOCP_PV_BATT_T, SOCP_BATT_T, ACPF_T, SOCP_T, SOCP_PV_T, ACPF_BATT_T, ACPF_PV_BATT_T, ACPF_PV_T, ACPF,ACPF_PV, ACPOL, ACRECT, QC, QC_SDP, OTS, DF, SOCP, SDP, DC, QC_OTS_L, QC_OTS_N, QC_OTS_O, SOCP_OTS, GRB_TEST } PowerModelType;
typedef enum { MinCostBatt_t, MinCostPv_T, MinCostPv,MinCostPvBatt, MinCost, MinCostTime,  MinLoss, MinDelta, MaxDelta } Obj;

class PowerModel {
public:
    PowerModelType      _type;
    Obj                 _objective;
    Model*              _model;
    PTSolver *           _solver;
    Net*                _net;
    SolverType          _stype;
    int                 _timesteps = 24; // number of time steps per day
    double              _optimal;
    double              _summer_power_factor = 0.96;
    double              _winter_power_factor = 0.97;
//    double              _price_inflation = 1;
//    double              _peak_tariff = 0.167*1.1;
//    double              _peak_tariff = 0.16952*1.1;
//    double              _peak_tariff = 0.191*1.1;
//    double              _demand_growth = 1;
    int                 _nb_days = 31;
//    int                 _nb_years = 15;
//    double              _pv_cost = 1800;// $ per Watt for PV rooftop installation
    time_t              _rawtime;
    struct tm*          _start_date;// First day in simulation
    struct tm*          _demand_start_date;// First day in historical demand data
    struct tm*          _irrad_start_date;// First day in historical irradiance data
    vector<int>         _random_load;
    vector<int>         _random_weather;
    vector<double>      _random_load_uncert;
    vector<double>      _random_PV_uncert;
    
    std::random_device       _rd; // obtain a random number from hardware
    std::mt19937             _eng; // seed the generator
    uniform_int_distribution<> _distr_year_load; // randomly pick a year for load historical data
    uniform_int_distribution<> _distr_year_PV; // randomly pick a year for PV historical data
    uniform_int_distribution<> _distr_uncert; // +-5% uncertainty on PV and load data
    uniform_int_distribution<> _distr_day; // randomly pick a day in month

    
    
    PowerModel();
    PowerModel(PowerModelType type, Net* net, SolverType stype);
    ~PowerModel();
    void reset();
    void build(int time_steps=1);
    void random_generator(double uncert_perc);
    void run_grb_lin_test();
    void run_grb_quad_test();
    /** Accessors */
    Function* objective();
    /** Variables */
    void add_AC_gen_vars();
    void add_AC_gen_vars_Time();

    void add_Battery_vars();
    
    void add_AC_Pol_vars();
    void add_AC_Rect_vars();
    void add_AC_Rect_vars_Time();
    void add_AC_Rect_vars_Peak();
    void add_AC_Rect_PV_vars_Peak();
    void add_AC_Rect_vars_Time_fixed();
    void add_AC_Rect_vars_Time_fixed_compact();
    
    void add_AC_Rect_PV_vars();
    void add_AC_Rect_PV_vars_Time();
    void add_NF_vars(bool PV = false);
    void add_NF_Peak_vars(bool PV = false);
    void add_COPPER_vars(bool PV = false);    
    void add_AC_Rect_Batt_vars_Time();
    void add_AC_Rect_PV_Batt_vars_Time();
    void add_SOCP_Rect_PV_Batt_vars_Time();
    void add_SOCP_Rect_Batt_vars_Time();
    void add_SOCP_Rect_PV_vars_Time();



    void add_QC_vars();
    void add_AC_OTS_vars();
    void add_AC_SOCP_vars();
    void add_DC_vars();
    void add_QC_OTS_vars();
    void add_SOCP_OTS_vars();
    void add_AC_SOCP_vars_Time();
    /** Constraints */
    void add_Wr_Wi(Arc* a);
    void add_SOCP_Angle_Bounds_Time(Arc *a);
    void add_AC_Angle_Bounds_Time(Arc *a);
    void add_AC_thermal(Arc* a, bool switch_lines);
    
    void add_AC_thermal_Time(Arc* a);
    
    void add_AC_Angle_Bounds(Arc* a, bool switch_lines);
    void add_AC_Voltage_Bounds(Node* n);

    void add_AC_Voltage_Bounds_Time(Node* n);
    void add_SOCP_Voltage_Bounds_Time(Node* n);
    void add_AC_Power_Flow(Arc* a, bool polar);
    
    void add_AC_Power_Flow_Time(Arc* a);

    void add_AC_KCL_Time(Node *n);
    void add_AC_KCL_NF(Node *n, bool PV = false);
    void add_AC_KCL_NF_PV(Node *n);
    void add_AC_KCL_SOCP_Time(Node *n);
    void add_AC_KCL_PV_Time(Node* n);
    void add_AC_KCL_PV_Time_Compact(Node* n);
    void add_AC_KCL_Batt_Time(Node* n);
    void add_AC_KCL_PV_Batt_Time(Node* n);
    void add_AC_link_Batt_Time(Node* n);
    void add_SOCP_KCL_PV_Time(Node* n);
    void add_SOCP_KCL_Batt_Time(Node* n);
    void add_SOCP_KCL_PV_Batt_Time(Node* n);
    
    void add_link_PV_Rate(Node* n);
    void add_link_PV_fixed_Rate(Node* n);
    
    void add_AC_KCL(Node* n, bool switch_lines);
    void add_AC_KCL_PV(Node* n, bool switch_lines);
    void add_Cosine(Arc* a);
    void add_SDP_cuts(int dim);
    void add_fixed_V(Gen* g);
    void add_fixed_Pg(Gen* g);
    /** Operators */
    int get_order(vector<Node*>* bag, int i);
    
    Function sum(vector<Arc*> vec, string var, bool switch_lines);
    Function sum(vector<Gen*> vec, string var);

    Function sum_Time(vector<Arc*> vec, string var, int time);
    Function sum_Time_Compact(vector<Arc*> vec, string var, int time);
    Function sum_Time(vector<Gen*> vec, string var, int time);

    int random_weather(tm time_c, int start_year, int end_year);
    int random_load(tm time_c, int start_year, int end_year);
    int get_week_day_of_1st(tm time_c);
    /** Models */
    void post_AC_Polar();
    void post_AC_PF();
    void post_NF(bool PV = false);
    void post_NF_Peak(bool PV = false);
    void post_AC_PF_Time();
    void post_AC_PF_Peak();
    void post_AC_PF_PV();
    void post_AC_PF_PV_Time();
    void post_AC_PF_PV_Peak();
    void post_COPPER(bool PV = false);
    void post_COPPER_static(bool PV = false, bool Batt = false);
    void post_AC_PF_Time_Fixed();
    void post_AC_PF_Time_Fixed_Compact();
    void post_AC_PF_PV_Batt_Time();
    void post_AC_PF_Batt_Time_No_Gen();
    void post_AC_PF_Batt_Time();
    void post_SOCP_PF_Batt_Time_No_Gen();
    void post_SOCP_PF_Batt_Time();
    void post_SOCP_PF_PV_Batt_Time();
    void post_AC_Rect();
    void post_QC();
    void post_AC_SOCP();
    void post_AC_SOCP_Time();

    void post_AC_OTS();
    void post_DC();
    void post_QC_OTS(bool lin_cos_cuts, bool quad_cos);
    void post_SOCP_OTS();
    void post_SOCP_PF_PV_Time();
    /** Presolve */
    void propagate_bounds();

    /** Solve */
    int solve(int output = 1,bool relax = false);
    void min_cost();
    void min_cost_time();
    void min_cost_peak();
    void min_cost_pv();
    void min_cost_pv_fixed();
    void min_cost_batt();
    void min_cost_pv_batt();

    void min_var(var<>& v);
    void max_var(var<>& v);
    void min_cost_load();
    void compute_losses();
    int get_nb_days_in_month(const tm& timeinfo) const;
    int get_nb_days_in_year(const tm& timeinfo) const;
    void print();

    void add_SineCutsPos(Arc *a);

    void add_SineCutsNeg(Arc *a);
};

#endif /* defined(__PowerTools____PowerModel__) */
