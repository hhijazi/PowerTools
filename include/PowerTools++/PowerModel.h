//
//  PowerModel.h
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____PowerModel__
#define __PowerTools____PowerModel__

#include <stdio.h>
#include <PowerTools++/Model.h>
#include "PowerTools++/PTSolver.h"
#include <PowerTools++/Net.h>
#include <PowerTools++/Constraint.h>
#include <PowerTools++/meta_constant.h>
#include <PowerTools++/meta_var.h>
#include <PowerTools++/meta_Constraint.h>
#include <PowerTools++/Type.h>


typedef enum { MinCost, MinLoss, MinDelta, MaxDelta } Obj;

class PowerModel {
public:
    PowerModelType      _type;
    Obj                 _objective;
    Model*              _model;
    PTSolver *             _solver;
    Net*                _net;
    SolverType          _stype;
    
    PowerModel();
    PowerModel(PowerModelType type, Net* net, SolverType stype);
    ~PowerModel();
    void reset();
    void build();
    void run_grb_lin_test();
    void run_grb_quad_test();
    /** Accessors */
    Function* objective();
    /** Variables */
    void add_AC_gen_vars();
    void add_AC_Pol_vars();
    void add_AC_Rect_vars();
    void add_AC_Ref_vars();
    void add_QC_vars();
    void add_AC_OTS_vars();
    void add_AC_SOCP_vars();
    void add_DC_vars();
    void add_QC_OTS_vars();
    void add_SOCP_OTS_vars();
    /** Constraints */
    void add_Wr_Wi(Arc* a);
    void add_AC_thermal(Arc* a, bool switch_lines);
    void add_AC_Angle_Bounds(Arc* a, bool switch_lines);
    void add_AC_Voltage_Bounds(Node* n);
    void add_AC_Power_Flow(Arc* a, bool polar);
    void add_AC_KCL(Node* n, bool switch_lines);
    void add_Cosine(Arc* a);
    int add_SDP_cuts(int dim);
    /** Operators */
    int get_order(vector<Node*>* bag, int i);
    
    Function sum(vector<Arc*> vec, string var, bool switch_lines);
    Function sum(vector<Gen*> vec, string var);
    
    /** Models */
    void post_AC_Polar();
    void post_AC_Rect();
    void post_AC_Ref();
    void post_QC();
    void post_AC_SOCP();
    void post_AC_OTS();
    void post_DC();
    void post_QC_OTS(bool lin_cos_cuts, bool quad_cos);
    void post_SOCP_OTS();
    /** Presolve */
    void propagate_bounds();

    /** Solve */
    int solve(int output = 1,bool relax = false);
    void min_cost();
    void min_var(var<> v);
    void max_var(var<> v);
    void min_cost_load();
    void print();

    /** Dynamic SDP cut generation */
    int add_SDP_cuts_dyn();

    /** Checking whether SDP conditions are satisfied for a 3D bag */
    bool SDP_satisfied(vector<Node *> *b);

    bool SDP_satisfied_new(vector<Node *> *b);

    bool SDP_satisfied_new1(Bag *b);

    /** Checks if two circles with given centers and radii have common points */
    bool circles_intersect(double xc, double yc, double R1, double R2, Arc *a);

    /** For a 3D bag with one unknown line, calculates the values of wr and wi
     * for this line using the SDP conditions */
    bool fix_bag(Bag *b);

    /** Add any missing SOCP constraints for a 3D bag */
    void add_bag_SOCP(Bag *b, int id);

#ifdef USE_IGRAPH
    /** For a partially defined bag, add SDP constraints for all non-PSD maximal cliques */
    void add_violated_cliques(Bag *b);

    bool clique_psd(Bag *b, igraph_vector_t *cl);

    void add_clique(Bag *b, igraph_vector_t *cl);
#endif

    void add_SDP_OA();

    Complex *determinant(vector<vector<Complex *>> A);

    int add_SDP_OA_closest_point();

    void add_SDP_OA_projections();

    void add_SDP_OA_deepest_cut();

    void add_SDP_OA_best_proj_obj();

    void add_SDP_OA_cp_and_deepcut();

    Function ort_plane(const double *x0, const double *x1, vector<int> *ind) const;
};

#endif /* defined(__PowerTools____PowerModel__) */
