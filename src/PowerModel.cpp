//
//  PowerModel.cpp
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#include "PowerTools++/PowerModel.h"

PowerModel::PowerModel():PowerModel(ACPOL, new Net(), ipopt){};

PowerModel::PowerModel(PowerModelType type, Net* net, SolverType stype):_type(type), _net(net), _stype(stype){
    _model = NULL;
    build();
};

void PowerModel::build(){
    _model = new Model();
    _solver = new Solver(_model,_stype);
    switch (_type) {
        case ACPOL:
            add_AC_Pol_vars();
            post_AC_Polar();
            break;
        case ACRECT:
            add_AC_Rect_vars();
            post_AC_Rect();
            break;
//        case QC:
//            add_QC_vars();
//            post_QC();
//            break;
        case OTS:
            add_AC_OTS_vars();
            post_AC_OTS();
            break;
        case SOCP:
            add_AC_SOCP_vars();
            post_AC_SOCP();
            break;
        case DC:
            add_DC_vars();
            post_DC();
            break;
//        case QC_OTS_L:
//            add_QC_OTS_vars();
//            post_QC_OTS(true,false);
//            break;
//        case QC_OTS_N:
//            add_QC_OTS_vars();
//            post_QC_OTS(true,true);
//            break;
//        case QC_OTS_O:
//            add_QC_OTS_vars();
//            post_QC_OTS(false,true);
//            break;
        case SOCP_OTS:
            add_SOCP_OTS_vars();
            post_SOCP_OTS();
            break;
        case GRB_TEST:
            //run_grb_lin_test();
            run_grb_quad_test();
            break;
        default:
            break;
    }
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
    //    obj->print(true);
    //    _solver->run();
}


void PowerModel::solve(int output, bool relax){
    _solver->run(output,relax);
}


void PowerModel::add_AC_gen_vars(){
    for (auto g:_net->gens) {
        if (!g->_active)
            continue;
        g->pg.init("pg"+g->_name+":"+g->_bus->_name, g->pbound.min, g->pbound.max);
        g->pg = g->ps;
        _model->addVar(g->pg);
        g->qg.init("qg"+g->_name+":"+g->_bus->_name, g->qbound.min, g->qbound.max);
        g->qg = g->qs;
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
        
    }
    add_AC_gen_vars();
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


void PowerModel::add_AC_SOCP_vars(){
    string key, src, dest;
    set<string> parallel;
    Arc* ap = nullptr;
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
        ap = *(_net->lineID[a->src->_name+","+a->dest->_name]->begin());
            ap->wr.init("wr"+a->_name, a->src->vbound.min*a->dest->vbound.min*cos(a->tbound.min), a->src->vbound.max*a->dest->vbound.max);
            ap->wr = 1;
            _model->addVar(ap->wr);
            ap->wi.init("wi"+a->_name, -a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max), a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max));
            _model->addVar(ap->wi);
//        else {
//            cout << "avoiding parallel line\n";
//        }
        a->init_complex();
        
    }
    for (auto n:_net->nodes) {
        n->w.init("w"+n->_name, pow(n->vbound.min,2), pow(n->vbound.max,2));
//        n->w = pow(n->vs,2);
        n->w = 1.001;
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

void PowerModel::add_AC_thermal(Arc* a, bool switch_lines){
    /** subject to Thermal_Limit {(l,i,j) in arcs}: p[l,i,j]^2 + q[l,i,j]^2 <= s[l]^2;*/
    if (a->status==1 || switch_lines) {
        Constraint Thermal_Limit_from("Thermal_Limit_from");
        Thermal_Limit_from += a->_Si_.square_magnitude();
        if (!switch_lines){
            Thermal_Limit_from <= pow(a->limit,2.);
        }
        else{
            //Thermal_Limit_from -= (0+a->on)*pow(a->limit,2.) + (1-a->on)*a->smax;
            Thermal_Limit_from -= (0+a->on)*(pow(a->limit,2.)-a->smax) + a->smax;
            Thermal_Limit_from <= 0;
        }
        _model->addConstraint(Thermal_Limit_from);
        Constraint Thermal_Limit_to("Thermal_Limit_to");
        Thermal_Limit_to += a->_Sj_.square_magnitude();
        if (!switch_lines){
            Thermal_Limit_to <= pow(a->limit,2.);
        }
        else{
            //Thermal_Limit_to -= (0+a->on)*pow(a->limit,2.) + (1-a->on)*a->smax;
            Thermal_Limit_to -= (0+a->on)*(pow(a->limit,2.)-a->smax) + a->smax;
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
            //Theta_Delta_UB -= (0+a->on)*(a->tbound.max-_net->m_theta_ub) + _net->m_theta_ub;
            Theta_Delta_UB <= 0;
        }
        _model->addConstraint(Theta_Delta_UB);
        Constraint Theta_Delta_LB("Theta_Delta_LB");
        Theta_Delta_LB += src->theta - dest->theta;
        if (!switch_lines){
            Theta_Delta_LB >= a->tbound.min;
        }
        else{
            //Theta_Delta_LB -= a->tbound.min*a->on + (1-a->on)*_net->m_theta_lb;
            Theta_Delta_LB -= (0+a->on)*(a->tbound.min-_net->m_theta_lb) + _net->m_theta_lb;
            Theta_Delta_LB >= 0;
        }
        _model->addConstraint(Theta_Delta_LB);
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

    _model->init_functions(_net->arcs.size());
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

void PowerModel::post_AC_OTS(){
    for (auto n:_net->nodes) {
        add_AC_KCL(n, true);
    }
    for (auto a:_net->arcs) {
        add_AC_Angle_Bounds(a, true);
        add_AC_Power_Flow(a, true);
        add_AC_thermal(a, true);
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
        ap = *(_net->lineID[a->src->_name+","+a->dest->_name]->begin());
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


PowerModel::~PowerModel(){
    delete _model;
    delete _solver;
    //    delete _net;
};
