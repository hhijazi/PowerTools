//
//  PowerModel.cpp
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#include "PowerTools++/PowerModel.h"
#ifdef USE_ARMADILLO
#include <armadillo>
#endif

//  Windows
#ifdef _WIN32
#include <Windows.h>

//  Posix/Linux
#else
#include <time.h>
#include <sys/time.h>
double get_wall_time1(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time1(){
    return (double)clock() / CLOCKS_PER_SEC;
}
#endif

PowerModel::PowerModel():PowerModel(ACPOL, new Net(), ipopt){};

PowerModel::PowerModel(PowerModelType type, Net* net, SolverType stype):_type(type), _net(net), _stype(stype){
    _model = NULL;
};


PowerModel::~PowerModel(){
    delete _model;
    delete _solver;
    //    delete _net;
};


void PowerModel::build(){
    _model = new Model();
    _solver = new PTSolver(_model, _stype);
    switch (_type) {
        case ACPOL:
            add_AC_Pol_vars();
            post_AC_Polar();
            break;
        case ACRECT:
            add_AC_Rect_vars();
            post_AC_Rect();
            break;
        case ACREF:
            add_AC_Ref_vars();
            post_AC_Ref();
            break;
        case QC:
            add_QC_vars();
            post_QC();
            break;
        case QC_SDP:
            add_QC_vars();
            post_QC();
            add_SDP_cuts(3);
            break;
        case OTS:
            add_AC_OTS_vars();
            post_AC_OTS();
            break;
        case SOCP:
            add_AC_SOCP_vars();
            post_AC_SOCP();
            break;
        case SDP:
            add_AC_SOCP_vars();
            post_AC_SOCP();
            //addSOCP();
            //add_SDP_cuts(3);
            break;
        case DC:
            add_DC_vars();
            post_DC();
            break;
        case QC_OTS_L:
            add_QC_OTS_vars();
            post_QC_OTS(true,false);
            break;
        case QC_OTS_N:
            add_QC_OTS_vars();
            post_QC_OTS(true,true);
            break;
        case QC_OTS_O:
            add_QC_OTS_vars();
            post_QC_OTS(false,true);
            break;
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
    int i = 0;
    for (auto g:_net->gens) {
        if (!g->_active || g->_bus->_type == 4)
            continue;
        i++;
        *obj += _net->bMVA*g->_cost->c1*(g->pg) + pow(_net->bMVA,2)*g->_cost->c2*(g->pg^2) + g->_cost->c0;
    }
    cout << "\nNumber of gens = " << i << "\n";
    _model->setObjective(obj);
    _model->setObjectiveType(minimize); // currently for gurobi
             obj->print(false);
    //    _solver->run();
}

void PowerModel::min_var(var<> v){
    _objective = MinDelta;
    Function* obj = new Function();
    *obj += v;
    _model->setObjective(obj);
    _model->setObjectiveType(minimize);
}

void PowerModel::max_var(var<> v){
    _objective = MaxDelta;
    Function* obj = new Function();
    *obj += v;
    _model->setObjective(obj);
    _model->setObjectiveType(maximize);
}

int PowerModel::solve(int output, bool relax){
    int cuts = 0;
    bool oa = true;
    double obj, time = 0;
    int sdp_alg = _net->sdp_alg;
    int status;
    double wall0,wall1,cpu0,cpu1;
    double tol = 0.0001;
    if(_type != SDP) status = _solver->run(output,relax);
    else{
        wall0 = get_wall_time1();
        cpu0 = get_cpu_time1();
        if(sdp_alg!=0) {
            status = _solver->run(output, relax);
            wall1 = get_wall_time1();
            cout << "\ntime spent on SOCP = " << wall1-wall0 << "\n";
            if(!oa) cuts = add_SDP_cuts_dyn();
//            else add_SDP_OA_deepest_cut();
            else cuts = add_SDP_OA_closest_point();
//            else add_SDP_OA_cp_and_deepcut();
            _solver->warm_start = true;
        }
        else cuts = add_SDP_cuts(3);

        status = _solver->run(output,relax);
//        wall1 = get_wall_time1();
//        time = wall1-wall0;
//        cout << "\ntime = " << wall1-wall0 << "\n";

        //Second iteration
//        for(auto a:_net->arcs){ // Unfix non-existent lines
//            if(a->imaginary) a->status = 0;
//        }
        if(oa) {
            _model->_opt = 1;
            double prev_opt = 0;
            int iter = 0;
            while((_model->_opt - prev_opt)/_model->_opt > tol) {
                prev_opt = _model->_opt;
    //                add_SDP_cuts_dyn();
//                add_SDP_OA_deepest_cut();
                cuts += add_SDP_OA_closest_point();
                _solver->run(output, relax);
//                cout << "\nDiff = " << (_model->_opt - prev_opt) / _model->_opt * 100;
                iter++;
            }
            wall1 = get_wall_time1();
            cpu1 = get_cpu_time1();
//            time = wall1 - wall0;
            time = cpu1 - cpu0;
            cout << "\nNumber of iterations = " << iter;
        }else if(0){
//            _model->_opt = 1;
//            double prev_opt = 0;
//            int iter = 0;
//            for (int i = 0; i < 8; i++) {
//        while((_model->_opt - prev_opt)/_model->_opt > tol) {
//                prev_opt = _model->_opt;
            add_SDP_cuts_dyn();
//                add_SDP_OA();
                _solver->run(output, relax);
                wall1 = get_wall_time1();
                cout << "\ntime2 = " << wall1 - wall0 << "\n";
//                cout << "\nDiff = " << (_model->_opt - prev_opt) / _model->_opt * 100;
//                iter++;
//            }
//            time = wall1 - wall0;
//            cout << "\nNumber of iterations = " << iter;
        }

//        Checks after
//        for (auto b: *_net->_bagsnew) {
//            if (b->size() != 3) continue;
//            cout << "\nBag #" << b->_id;
//            if (SDP_satisfied_new1(b)) cout << "CUT SATISFIED";
//            else cout << "CUT NOT SATISFIED!";
//        }
    }
    cpu1 = get_cpu_time1();
    time = cpu1 - cpu0;

    obj = _model->_opt;
    cout.setf(ios::fixed);
    cout.precision(2);
    cout << "\nResults: " << time << " & " << obj << " & " << cuts << "\n";
    return status;
}


void PowerModel::add_AC_gen_vars(){
    for (auto g:_net->gens) {
        if (!g->_active)
            continue;
        g->pg.init("pg"+g->_name+":"+g->_bus->_name, g->pbound.min, g->pbound.max);
//        g->pg = g->ps;
        g->pg = g->pbound.max;
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

void PowerModel::add_AC_Ref_vars(){
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
        n->w.init("w"+n->_name, n->vbound.min*n->vbound.min, n->vbound.max*n->vbound.max);
        _model->addVar(n->w);
        n->init_complex(true);
    }
    add_AC_gen_vars();
}

void PowerModel::add_QC_vars(){
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
        a->pi.init("p("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->qi.init("q("+a->_name+","+a->src->_name+","+a->dest->_name+")");
        a->pj.init("p("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        a->qj.init("q("+a->_name+","+a->dest->_name+","+a->src->_name+")");
        _model->addVar(a->pi);
        _model->addVar(a->pj);
        _model->addVar(a->qi);
        _model->addVar(a->qj);
        if (a->tbound.min < 0 && a->tbound.max > 0)
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(a->tbound.min), cos(a->tbound.max)), 1.);
        else
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(a->tbound.min), cos(a->tbound.max)), max(cos(a->tbound.min),cos(a->tbound.max)));
        a->vv.init("vv("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->src->vbound.min*a->dest->vbound.min,a->src->vbound.max*a->dest->vbound.max);
        a->wr.init("wr("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_lb()*a->cs.get_lb(), a->vv.get_ub()*a->cs.get_ub());
        if(a->tbound.min < 0 && a->tbound.max > 0)
            a->wi.init("wi("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_ub()*sin(a->tbound.min), a->vv.get_ub()*sin(a->tbound.max));
        if (a->tbound.min >= 0)
            a->wi.init("wi("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_lb()*sin(a->tbound.min), a->vv.get_ub()*sin(a->tbound.max));
        if (a->tbound.max <= 0)
            a->wi.init("wi("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->vv.get_ub()*sin(a->tbound.min), a->vv.get_lb()*sin(a->tbound.max));
        a->sn.init("sn("+a->_name+","+a->src->_name+","+a->dest->_name+")",sin(a->tbound.min), sin(a->tbound.max));
        a->ci.init("ci("+a->_name+","+a->src->_name+","+a->dest->_name+")", 0.0, INFINITY);
        a->delta.init("delta("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->tbound.min,a->tbound.max);
        _model->addVar(a->wr);
        a->wr = 1;
        _model->addVar(a->wi);
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

void PowerModel::add_QC_OTS_vars() {
    double m_theta_ub = 0, m_theta_lb = 0;
    for (auto a:_net->arcs) {
        m_theta_ub += a->tbound.max;
        m_theta_lb += a->tbound.min;
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
        
        a->vv.init("vv("+a->_name+","+a->src->_name+","+a->dest->_name+")",a->src->vbound.min*a->dest->vbound.min,a->src->vbound.max*a->dest->vbound.max,0,0);
        if (a->tbound.min < 0 && a->tbound.max > 0)
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(a->tbound.min),cos(a->tbound.max)),1,0,0);
        else
            a->cs.init("cs("+a->_name+","+a->src->_name+","+a->dest->_name+")",min(cos(a->tbound.min),cos(a->tbound.max)),max(cos(a->tbound.min),cos(a->tbound.max)),0,0);
        a->wr.init("wr"+a->_name, a->vv.get_lb()*a->cs.get_lb(), a->vv.get_ub()*a->cs.get_ub(),0,0);
        if(a->tbound.min < 0 && a->tbound.max > 0)
            a->wi.init("wi"+a->_name, a->vv.get_ub()*sin(a->tbound.min), a->vv.get_ub()*sin(a->tbound.max),0,0);
        if (a->tbound.min >= 0)
            a->wi.init("wi"+a->_name, a->vv.get_lb()*sin(a->tbound.min), a->vv.get_ub()*sin(a->tbound.max),0,0);
        if (a->tbound.max <= 0)
            a->wi.init("wi"+a->_name, a->vv.get_ub()*sin(a->tbound.min), a->vv.get_lb()*sin(a->tbound.max),0,0);
        a->w_line_ij.init("w_line("+a->_name+","+a->src->_name+","+a->dest->_name+")", pow(a->src->vbound.min,2), pow(a->src->vbound.max,2),0,0);
        a->w_line_ji.init("w_line("+a->_name+","+a->dest->_name+","+a->src->_name+")", pow(a->dest->vbound.min,2), pow(a->dest->vbound.max,2),0,0);
        
        a->sn.init("sn("+a->_name+","+a->src->_name+","+a->dest->_name+")",sin(a->tbound.min), sin(a->tbound.max),0,0);
        a->ci.init("ci("+a->_name+","+a->src->_name+","+a->dest->_name+")", 0, INFINITY);
        a->delta.init("delta("+a->_name+","+a->src->_name+","+a->dest->_name+")", a->tbound.min, a->tbound.max, _net->m_theta_lb, _net->m_theta_ub);
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

void PowerModel::post_QC_OTS(bool lin_cos_cuts, bool quad_cos){
    cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
    exit(-1);
}

void PowerModel::propagate_bounds(){
    if (_model != NULL) {
        cerr << "\npropagate_bounds should be called before building the model.";
        exit(1);
    }
    var<double>* delta;
    var<double>* voltage;
    double tol = 0.01;
    double* tol_delta_l = new double[_net->arcs.size()];
    double* tol_delta_u = new double[_net->arcs.size()];
    double* tol_v_l = new double[_net->nodes.size()];
    double* tol_v_u = new double[_net->nodes.size()];
    double maxtol = 1;
    int i = 0;
    fill_n(tol_delta_l, _net->arcs.size(), 1);
    fill_n(tol_delta_u, _net->arcs.size(), 1);
    fill_n(tol_v_l, _net->nodes.size(), 1);
    fill_n(tol_v_u, _net->nodes.size(), 1);

    _solver = new PTSolver(NULL,_stype);
    
    while(maxtol > tol) {
        cout << "\nmaxtol = " << maxtol << "\n";
        _model = new Model();
        //        _solver = new PTSolver(_model,_stype);
        _solver->set_model(_model);
        maxtol = 0;
        //        add_QC_OTS_vars();
        //        if (_type == QC_OTS_N) post_QC_OTS(true,true);
        //        if (_type == QC_OTS_O) post_QC_OTS(false,true);
        //        if (_type == QC_OTS_L) post_QC_OTS(true,false);
        switch (_type) {
            case QC:
                cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
                exit(-1);
                break;
            case QC_OTS_L:
                cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
                exit(-1);
                break;
            case QC_OTS_N:
                cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
                exit(-1);
                break;
            case QC_OTS_O:
                cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
                exit(-1);
                break;
            default:
                cerr << "\nBound propagation is not supported for this model yet\n";
                return;
                break;
        }
        
        i = 0;
        for (auto& a: _net->arcs) {
            //if (!a->src->_has_gen && !a->dest->_has_gen) {i++; continue;}
            if (tol_delta_l[i] > tol) {
                if(_type==QC_OTS_L || _type==QC_OTS_N || _type==QC_OTS_O || _type==SOCP_OTS)
                    a->on.set_lb(true);
                min_var(a->delta);
                solve(0,true);
                delta = _model->getVar_<double>(a->delta.get_idx());
                a->tbound.min = delta->get_value();
                tol_delta_l[i] = a->tbound.min-delta->get_lb();
                if (tol_delta_l[i] > maxtol) maxtol = tol_delta_l[i];
                if(_type==QC_OTS_L || _type==QC_OTS_N || _type==QC_OTS_O || _type==SOCP_OTS)
                    a->on.set_lb(false);
            }
            if(tol_delta_u[i]>tol) {
                if(_type==QC_OTS_L || _type==QC_OTS_N || _type==QC_OTS_O || _type==SOCP_OTS)
                    a->on.set_lb(true);
                max_var(a->delta);
                solve(0,true);
                delta = _model->getVar_<double>(a->delta.get_idx());
                a->tbound.max = delta->get_value();
                tol_delta_u[i] = delta->get_ub()-a->tbound.max;
                if (tol_delta_u[i] > maxtol) maxtol = tol_delta_u[i];
                if(_type==QC_OTS_L || _type==QC_OTS_N || _type==QC_OTS_O || _type==SOCP_OTS)
                    a->on.set_lb(false);
            }
            i++;
        }
        
        i = 0;
        for(auto& n: _net->nodes){
            //            if (!n->_has_gen) {i++; continue;}
            if(tol_v_l[i] > tol) {
                min_var(n->v);
                solve(0,true);
                voltage = _model->getVar_<double>(n->v.get_idx());
                n->vbound.min = voltage->get_value();
                tol_v_l[i] = n->vbound.min-voltage->get_lb();
                if (tol_v_l[i] > maxtol) maxtol = tol_v_l[i];
            }
            if(tol_v_u[i] > tol) {
                max_var(n->v);
                solve(0,true);
                voltage = _model->getVar_<double>(n->v.get_idx());
                n->vbound.max = voltage->get_value();
                tol_v_u[i] = n->vbound.max-voltage->get_ub();
                if (tol_v_u[i] > maxtol) maxtol = tol_v_u[i];
            }
            i++;
        }
        
        cout << endl << "New bounds, maxtol = " << maxtol << ":";
        for (auto& a: _net->arcs) {
            cout << "\nmin = " << a->tbound.min << " max = " << a->tbound.max;
        }
        for (auto& n: _net->nodes) {
            cout << "\nvmin = " << n->vbound.min << " vmax = " << n->vbound.max;
        }
        delete _model;
    }
    cout << endl << "New bounds: ";
    i = 0;
    for (auto& a: _net->arcs) {
        cout << "\nmin = " << a->tbound.min << " max = " << a->tbound.max;
        i++;
    }
    delete[] tol_delta_l;
    delete[] tol_delta_u;
    delete[] tol_v_l;
    delete[] tol_v_u;
 
}

void PowerModel::post_QC(){
    cout << "QC model not available in the open source version of PowerTools, please contact us to get a license.\n";
    exit(-1);
}

void PowerModel::add_AC_SOCP_vars(){
    string key, src, dest;
    set<string> parallel;
//    Arc* ap = nullptr;
    for (auto a:_net->arcs) {
        if (a->status==0 || a->src->_type==4 || a->dest->_type==4) {
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
//        ap = *(_net->lineID[a->src->_name+","+a->dest->_name]->begin());
//            ap->wr.init("wr"+a->_name, a->src->vbound.min*a->dest->vbound.min*cos(a->tbound.min), a->src->vbound.max*a->dest->vbound.max);
//            ap->wr = 1;
//            _model->addVar(ap->wr);
//            ap->wi.init("wi"+a->_name, -a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max), a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max));
//            _model->addVar(ap->wi);
        if (!a->parallel) {
            //a->wr.init("wr"+a->_name, a->src->vbound.min*a->dest->vbound.min*cos(a->tbound.min), a->src->vbound.max*a->dest->vbound.max);
            a->wr.init("wr"+a->src->_name+a->dest->_name, a->src->vbound.min*a->dest->vbound.min*cos(a->tbound.min), a->src->vbound.max*a->dest->vbound.max);
//            if(a->src->ID > a->dest->ID) cout << "\n" << a->pi._name;
//            a->wr.init("wr"+a->src->_name+a->dest->_name, 0, 1.21);
            a->wr = 1;
            _model->addVar(a->wr);
            //a->wi.init("wi"+a->_name, -a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max), a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max));
            a->wi.init("wi"+a->src->_name+a->dest->_name, -a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max), a->src->vbound.max*a->dest->vbound.max*sin(a->tbound.max));
//            a->wi.init("wi"+a->src->_name+a->dest->_name, -1.21, 1.21);
            _model->addVar(a->wi);
        }
//        else {
//            cout << "avoiding parallel line\n";
//        }
        a->init_complex();
    }
    for (auto n:_net->nodes) {
        if(n->_type==4) continue;
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
//    if(n->gs()!=0) KCL_P.print();
//    KCL_P.print();
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

void PowerModel::add_Cosine(Arc *a){
    double *x = new double[_model->get_nb_vars()];
    double theta_u;
    double Ml = _net->m_theta_lb;
    double Mu = _net->m_theta_ub;
    double l = a->tbound.min;
    double u = a->tbound.max;
    theta_u = max(-l,u);
    Function CSR; // cs(theta,u) if theta > 0
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
            x[a->cs.get_idx()] = CSL.eval(x);
            Constraint CS_UB_OAL3("CS_UB_OAL3");
            CS_UB_OAL3 += Cs_l.outer_approx(x);
            CS_UB_OAL3 <= 0;
            _model->addConstraint(CS_UB_OAL3);
            //            }
            
        }
    }
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
        if(n->_type==4) continue;
        add_AC_KCL(n, false);
    }
    Arc* ap = nullptr;

    int i = 1;
    for (auto a:_net->arcs) {
        if (a->status == 0  || a->src->_type==4 || a->dest->_type==4) {
            continue;
        }
        src = a->src;
        dest = a->dest;
//        if(src->ID > dest->ID) cout << "\n" << a->pi._name;
        if (a->status == 0  || src->_type==4 || dest->_type==4) {
            continue;
        }
        /** subject to Flow_P_From {(l,i,j) in arcs_from}:
         p[l,i,j] = g[l]*(v[i]/tr[l])^2
         + -g[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -b[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        ap = _net->get_arc(src, dest);

        Constraint Flow_P_From("Flow_P_From" + a->pi._name);
        Flow_P_From += a->pi;
        Flow_P_From -= (a->g / (pow(a->cc, 2) + pow(a->dd, 2))) * src->w;
        Flow_P_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr;
        if(a->src == ap->src)//(src->ID < dest->ID)
            Flow_P_From -= (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        else {
            cout << "\nReversed line " << a->pi._name;
            Flow_P_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        }
        Flow_P_From = 0;
//        if(a->src == ap->src)
            _model->addConstraint(Flow_P_From);

//        Flow_P_From.print();

        /** subject to Flow_P_To {(l,i,j) in arcs_to}:
         p[l,i,j] = g[l]*v[i]^2
         + -g[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -b[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_P_To("Flow_P_To" + a->pj._name);
        Flow_P_To += a->pj;
        Flow_P_To -= a->g * dest->w;
        Flow_P_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr;
        if(a->src == ap->src)//(src->ID < dest->ID)
            Flow_P_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        else
            Flow_P_To -= (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        Flow_P_To = 0;
//        if(a->src == ap->src)
            _model->addConstraint(Flow_P_To);

        /** subject to Flow_Q_From {(l,i,j) in arcs_from}:
         q[l,i,j] = -(charge[l]/2+b[l])*(v[i]/tr[l])^2
         +  b[l]*v[i]/tr[l]*v[j]*cos(t[i]-t[j]-as[l])
         + -g[l]*v[i]/tr[l]*v[j]*sin(t[i]-t[j]-as[l]);
         */
        Constraint Flow_Q_From("Flow_Q_From" + a->qi._name);
        Flow_Q_From += a->qi;
        Flow_Q_From += (a->ch / 2 + a->b) / (pow(a->cc, 2) + pow(a->dd, 2)) * src->w;
        Flow_Q_From += (-a->b * a->cc - a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr;
        if(a->src == ap->src)//(src->ID < dest->ID)
            Flow_Q_From -= (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        else
            Flow_Q_From += (-a->g * a->cc + a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        Flow_Q_From = 0;
//        if(a->src == ap->src)
            _model->addConstraint(Flow_Q_From);

        /** subject to Flow_Q_To {(l,i,j) in arcs_to}:
         q[l,i,j] = -(charge[l]/2+b[l])*v[i]^2
         +  b[l]*v[i]*v[j]/tr[l]*cos(t[i]-t[j]+as[l])
         + -g[l]*v[i]*v[j]/tr[l]*sin(t[i]-t[j]+as[l]);
         */
        Constraint Flow_Q_To("Flow_Q_To" + a->qj._name);
        Flow_Q_To += a->qj;
        Flow_Q_To += (a->ch / 2 + a->b) * dest->w;
        Flow_Q_To += (-a->b * a->cc + a->g * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wr;
        if(a->src == ap->src)//(src->ID < dest->ID)
            Flow_Q_To += (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        else
            Flow_Q_To -= (-a->g * a->cc - a->b * a->dd) / (pow(a->cc, 2) + pow(a->dd, 2)) * ap->wi;
        Flow_Q_To = 0;
//        if(a->src == ap->src)
            _model->addConstraint(Flow_Q_To);

        /** subject to PSD {l in lines}: w[from_bus[l]]*w[to_bus[l]] >= wr[l]^2 + wi[l]^2;
         */
        Constraint SOCP("SOCP("+src->_name+","+dest->_name+")");
        SOCP += a->src->w * a->dest->w;
        SOCP -= ((ap->wr) ^ 2);
        SOCP -= ((ap->wi) ^ 2);
        SOCP >= 0;
        SOCP.is_cut = true;
        _model->addConstraint(SOCP);
//
        add_AC_thermal(a, false);
        add_Wr_Wi(ap);
    }
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

bool PowerModel::circles_intersect(double xc, double yc, double R1, double R2, Arc* a){
    double tol = 0.000001;
//    if (R1 > 0) {
//        if(sqrt(xc*xc + yc*yc) <= sqrt(R1) + sqrt(R2)) {
//            //cout << "\n Dist from 0 = " << sqrt(xc*xc + yc*yc);
//            //cout << "\n Sum of radii = " << sqrt(R1) + sqrt(R2);
//            //cout << "\nRadii ok.";
//            //return true;
//        }
//        else {
//            //cout << "\nRadii not ok.";
//            //return false;
//        }
//    }
    //cout << "\nR2 = " << R2;
//    if(fabs(R1) < tol) {
        tol = 0;
        if (xc*xc + yc*yc <= R2+tol && xc >= a->wr.get_lb()-tol && xc <= a->wr.get_ub()+tol && yc >= a->wi.get_lb()-tol && yc <= a->wi.get_ub()+tol) {
            //cout << "\nAn SDP completion exists!";
            return true;
        }else {
            if (!(xc*xc + yc*yc <= R2+tol)) {
                cout << "\nSOC is violated:";// (x,y) = (" << xc << ", " << yc << "), wr lb, ub = (" << a->wr.get_lb() << ", " << a->wr.get_ub() << "), wi lb, ub = (" << a->wi.get_lb() << ", " << a->wi.get_ub() << ")";
                cout << " res for SOC = " << R2 - xc*xc - yc*yc;
            }
            if(!(xc >= a->wr.get_lb()-tol)) {
                cout << "\nBound is violated: ";
                cout << "res for xlb = " << xc - a->wr.get_lb();
            }
            if(!(xc <= a->wr.get_ub()+tol)) {
                cout << "\nBound is violated: ";
                cout << "res for xub = " << a->wr.get_ub()-xc;
            }
            if(!(yc >= a->wi.get_lb()-tol)) {
                cout << "\nBound is violated: ";
                cout << "res for ylb = " << yc - a->wi.get_lb();
            }
            if(!(yc <= a->wi.get_ub()+tol)) {
                cout << "\nBound is violated: ";
                cout << "res for yub = " << a->wi.get_ub() - yc;
            }


            //else cout << "\nR2 = " << R2;
//            cout << "\n Squared dist from 0 = " << xc*xc + yc*yc;
            return false;
        }
//    }else{
//        cout << "\nCircle with non-zero radius, not implemented!";
//        return false;
//    }
}

bool PowerModel::fix_bag(Bag *b){
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;
    bool a12b, a13b, a32b;
    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;
    double wr12, wi12;
    double wr13, wi13;
    double wr32, wi32;
    double tol = 0.00001;

    n1 = b->_n1; n2 = b->_n2; n3 = b->_n3;

    double w1 = _net->get_node(n1->_name)->w.get_value();
    double w2 = _net->get_node(n2->_name)->w.get_value();
    double w3 = _net->get_node(n3->_name)->w.get_value();

    a12 = _net->get_arc(n1,n2);
    a13 = _net->get_arc(n1,n3);
    a32 = _net->get_arc(n3,n2);

    a12b = a12 && a12->status;
    a13b = a13 && a13->status;
    a32b = a32 && a32->status;

    // Check that there is exactly one unknown line in the bag
    assert((a13b && a12b) || (a13b && a32b) || (a12b && a32b));
    assert(!a13b || !a12b || !a32b);

    if(!a13 || a13->status == 0) {
        if (!a13){
            a13 = _net->_clone->get_arc(n1,n3)->clone();
            a13->src = _net->get_node(a13->src->_name);
            a13->dest = _net->get_node(a13->dest->_name);
            a13->connect();
            _net->add_arc(a13);
        }
        a13->status = 1;
        a13->init_vars(SDP);
        _model->addVar(a13->wr);
        _model->addVar(a13->wi);

        wr12 = a12->wr.get_value();
        wi12 = a12->wi.get_value();
        wr32 = a32->wr.get_value();
        wi32 = a32->wi.get_value();

//        cout << "Calculating values for " << a13->_name;
        if(!(b->_rot)) {
            wr13 = (wr12 * wr32 + wi12 * wi32) / w2;
            wi13 = (wi12 * wr32 - wr12 * wi32) / w2;
//            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
//            SDP *= 2;
//            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
//            SDP += w1*w2*w3;
//            double R1 = wr13*wr13 + wi13*wi13 - ((wr12*wr12+wi12*wi12)*w3 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w2;
//            cout << "\nR1 = " << R1 << ", SOC = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC2 = " << wr32*wr32 + wi32*wi32 - w2*w3;
//            cout << "\nNo a13, SDP = " << SDP;
        }else {
            wr13 = (wr12 * wr32 - wi12 * wi32) / w2;
            wi13 = (-wi12 * wr32 - wr12 * wi32) / w2;
//            double R1 = wr13*wr13 + wi13*wi13 - ((wr12*wr12+wi12*wi12)*w3 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w2;
//            cout << "\nR1 = " << R1 << ", SOC = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC2 = " << wr32*wr32 + wi32*wi32 - w2*w3;
//            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
//            SDPr *= 2;
//            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
//            SDPr += w1*w2*w3;
//            cout << "\nNo a13, SDPr = " << SDPr;
        }
        a13->wr.set_val(wr13);
        a13->wi.set_val(wi13);

        if(_net->sdp_alg==1) return false;

        if (!(wr13 >= a13->wr.get_lb()-tol && wr13 <= a13->wr.get_ub()+tol && wi13 >= a13->wi.get_lb()-tol
              && wi13 <= a13->wi.get_ub()+tol)){
//            cout << "\nBounds are violated";
            return false;
        }
        if(wr13*wr13+wi13*wi13 > w1*w3+tol) {
//            cout << "\nSOCP is violated";
            return false;
        }
        return true;
    }

    if(!a32 || a32->status == 0) {
        if (!a32) {
            a32 = _net->_clone->get_arc(n3,n2)->clone();
            a32->src = _net->get_node(a32->src->_name);
            a32->dest = _net->get_node(a32->dest->_name);
            a32->connect();
            _net->add_arc(a32);
        }
        a32->status = 1;
        a32->init_vars(SDP);
        _model->addVar(a32->wr);
        _model->addVar(a32->wi);

        wr12 = a12->wr.get_value();
        wi12 = a12->wi.get_value();
        wr13 = a13->wr.get_value();
        wi13 = a13->wi.get_value();

//        cout << "Calculating values for " << a32->_name;
        if(!(b->_rot)) {
            wr32 = (wr12 * wr13 + wi12 * wi13) / w1;
            wi32 = (wi12 * wr13 - wr12 * wi13) / w1;
//            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
//            SDP *= 2;
//            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
//            SDP += w1*w2*w3;
//            double R1 = (wr12*wr12*wr13*wr13 + wi12*wi12*wi13*wi13 + wi12*wi12*wr13*wr13 + wr12*wr12*wi13*wi13)/(w1*w1);
//            R1 -= ((wr13*wr13+wi13*wi13)*w2 + (wr12*wr12+wi12*wi12)*w3 - w1*w2*w3)/w1;
//            cout << "\nR1 = " << R1 << ", SOC = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC2 = " << wr13*wr13 + wi13*wi13 - w1*w3;
//            cout << "\nNo a32, SDP = " << SDP;
        }else{
            wr32 = (wr12*wr13 - wi12*wi13)/w1;
            wi32 = (-wi12*wr13 - wr12*wi13)/w1;
//            double R1 = wr32*wr32 + wi32*wi32 - ((wr13*wr13+wi13*wi13)*w2 + (wr12*wr12+wi12*wi12)*w3 - w1*w2*w3)/w1;
//            cout << "\nR1 = " << R1 << ", SOC = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC2 = " << wr13*wr13 + wi13*wi13 - w1*w3;
//            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
//            SDPr *= 2;
//            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
//            SDPr += w1*w2*w3;
//            cout << "\nNo a32, SDPr = " << SDPr;
        }
        a32->wr.set_val(wr32);
        a32->wi.set_val(wi32);

        if(_net->sdp_alg==1) return false;

        if (!(wr32 >= a32->wr.get_lb()-tol && wr32 <= a32->wr.get_ub()+tol && wi32 >= a32->wi.get_lb()-tol
              && wi32 <= a32->wi.get_ub()+tol + wr32*wr32+wi32*wi32 <= w3*w2+tol)) {
//            cout << "\nBounds are violated";
            return false;
        }
        return true;
    }

    if(!a12 || a12->status == 0) {
        if (!a12) {
            a12 = _net->_clone->get_arc(n1,n2)->clone();
            a12->src = _net->get_node(a12->src->_name);
            a12->dest = _net->get_node(a12->dest->_name);
            a12->connect();
            _net->add_arc(a12);
        }
        a12->status = 1;
        a12->init_vars(SDP);
        _model->addVar(a12->wr);
        _model->addVar(a12->wi);

        wr13 = a13->wr.get_value();
        wi13 = a13->wi.get_value();
        wr32 = a32->wr.get_value();
        wi32 = a32->wi.get_value();

//        cout << "Calculating values for " << a12->_name;
        if(!(b->_rot)) {
            wr12 = (wr32 * wr13 - wi32 * wi13) / w3;
            wi12 = (wi32 * wr13 + wr32 * wi13) / w3;
//            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
//            SDP *= 2;
//            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
//            SDP += w1*w2*w3;
//            double R1 = wr12*wr12 + wi12*wi12 - ((wr13*wr13+wi13*wi13)*w2 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w3;
//            cout << "\nR1 = " << R1;
//            cout << "\nNo a12, SDP = " << SDP;
        }else{
            wr12 = (wr32*wr13 - wi32*wi13)/w3;
            wi12 = (-wi32*wr13 - wr32*wi13)/w3;
//            double R1 = wr12*wr12 + wi12*wi12 - ((wr13*wr13+wi13*wi13)*w2 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w3;
//            cout << "\nR1 = " << R1;
//            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
//            SDPr *= 2;
//            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
//            SDPr += w1*w2*w3;
//            cout << "\nNo a12, SDPr = " << SDPr;
        }
        a12->wr.set_val(wr12);
        a12->wi.set_val(wi12);

        if(_net->sdp_alg==1) return false;

        if (!(wr12 >= a12->wr.get_lb()-tol && wr12 <= a12->wr.get_ub()+tol && wi12 >= a12->wi.get_lb()-tol
              && wi12 <= a12->wi.get_ub()+tol && wr12*wr12+wi12*wi12 <= w1*w2+tol)) {
//            cout << "\nBounds are violated";
            return false;
        }
        return true;
    }
}

bool PowerModel::SDP_satisfied_new1(Bag* b){
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;
    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;
    double wr12, wi12;
    double wr13, wi13;
    double wr32, wi32;
    double tol = 0.00001;
    bool a12_on, a13_on, a32_on;

    if (b->size()==3) {
        n1 = b->_n1; n2 = b->_n2; n3 = b->_n3;
        a12 = _net->get_arc(n1,n2);
        a13 = _net->get_arc(n1,n3);
        a32 = _net->get_arc(n3,n2);

        a12_on = a12 && a12->status;
        a13_on = a13 && a13->status;
        a32_on = a32 && a32->status;

//        cout << "\nn1 = " << n1->_name << " n2 = " << n2->_name << " n3 = " << n3->_name;

        if ((!a12_on && !a13_on) || (!a12_on && !a32_on) || (!a13_on && !a32_on)) {
//            cout << "\nAt least two free lines in bag, returning 'satisfied'.";
            return true;
        }

//        if(!a12_on) cout << "\n!a12";
//        if(!a13_on) cout << "\n!a13";
//        if(!a32_on) cout << "\n!a32";

        if(!a12_on || !a13_on || !a32_on) return fix_bag(b);

//        if(a12->imaginary) cout << "\na12 is imaginary";
//        if(a13->imaginary) cout << "\na13 is imaginary";
//        if(a32->imaginary) cout << "\na32 is imaginary";
//        if(!a12->imaginary && !a13->imaginary && !a32->imaginary) cout << "\nAll lines are real";

        // all lines exist

        wr12 = a12->wr.get_value();
        wi12 = a12->wi.get_value();
        wr13 = a13->wr.get_value();
        wi13 = a13->wi.get_value();
        wr32 = a32->wr.get_value();
        wi32 = a32->wi.get_value();
        double w1 = _net->get_node(n1->_name)->w.get_value();
        double w2 = _net->get_node(n2->_name)->w.get_value();
        double w3 = _net->get_node(n3->_name)->w.get_value();

        if (!(b->_rot)) {
            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
            SDP *= 2;
            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
            SDP += w1*w2*w3;
//            cout << "\nSDP = " << SDP;
//            if(a12->imaginary || a13->imaginary || a32->imaginary) {
//                cout << ", SOC12 = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC13 = " << wr13*wr13 + wi13*wi13 - w1*w3 << ", SOC32 = " << wr32*wr32 + wi32*wi32 - w2*w3;
//            }
            if(SDP > -tol) {
//                cout << "\n3d cut satisfied";
                return true;
            }
            else {
                if(a12->imaginary) {
                    for(auto db: *a12->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }
                if(a13->imaginary) {
                    for(auto db: *a13->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }
                if(a32->imaginary) {
                    for(auto db: *a32->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }

//                cout << "\nSDP = 2*" << wr12 << "*(" << wr32 << "*" << wr13 << "-" << wi32 << "*" << wi13 << ") + 2*"  <<  wi12 << "*(" << wi32 << "*" << wr13 << " + " << wr32 << "*" << wi13 << ")";
//                cout << " - (" << wr12 << "*" << wr12 << " + " << wi12 << "*" << wi12 << ")*" << w3 << " - (" << wr13 << "*" << wr13 << " + " << wi13 << "*" << wi13 << ")*" << w2 << " - (" << wr32 << "*" << wr32 << " + " << wi32 << "*" << wi32 << ")*" << w1;
//                cout << " + " << w1 << "*" << w2 << "*" << w3;
//                cout << "\nn1 = " << n1->_name << ", n2 = " << n2->_name << ", n3 = " << n3->_name;

//                cout << "\nNot satisfied, SDP = " << SDP;
                return false;
            }
        }else{
            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
            SDPr *= 2;
            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
            SDPr += w1*w2*w3;
//            cout << "\nSDPr = " << SDPr;
//            if(a12->imaginary || a13->imaginary || a32->imaginary) {
//                cout << ", SOC12 = " << wr12*wr12 + wi12*wi12 - w1*w2 << ", SOC13 = " << wr13*wr13 + wi13*wi13 - w1*w3 << ", SOC32 = " << wr32*wr32 + wi32*wi32 - w2*w3;
//            }
            if(SDPr > -tol) {
//                cout << "\n3d cut satisfied";
                return true;
            }
            else {
                if(a12->imaginary) {
                    for(auto db: *a12->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }
                if(a13->imaginary) {
                    for(auto db: *a13->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }
                if(a32->imaginary) {
                    for(auto db: *a32->defining_bags) {
                        db->_need_to_fix=true;
                    }
                }
//                cout << "\nNot satisfied, SDP = " << SDP;
                return false;
            }
        }
    }
    return true;
}

bool PowerModel::SDP_satisfied_new(vector<Node*>* b){
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;

    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;

    double wr12, wi12;
    double wr13, wi13;
    double wr32, wi32;

    double tol = 0.000001;

    bool rot_bag;
    int n_i1, n_i2, n_i3 = 0;

    if (b->size()==3) {
        cout << "\n------------------------------\n";
        rot_bag = _net->rotation_bag(b);
        if (rot_bag) {
            n1 = b->at(0);
            if (_net->_clone->has_directed_arc(n1, b->at(1))) {
                n2 = b->at(1);
                n3 = b->at(2);
            }
            else {
                n2 = b->at(2);
                n3 = b->at(1);
            }
            a13 = _net->get_arc(n3, n1);
            a32 = _net->get_arc(n2, n3);
        }
        else {
            n_i1 = get_order(b, 1);
            n1 = b->at(n_i1);
            n_i2 = get_order(b, 2);
            n2 = b->at(n_i2);
            n_i3 = get_order(b, 3);
            n3 = b->at(n_i3);
            a13 = _net->get_arc(n1, n3);
            a32 = _net->get_arc(n3, n2);
        }
        a12 = _net->get_arc(n1, n2);
        cout << "\nn1 = " << n1->_name << " n2 = " << n2->_name << " n3 = " << n3->_name;

        if(!a13 || a13->status == 0) {
            if (!a13) {
                if(rot_bag) a13 = _net->_clone->get_arc(n3,n1)->clone();
                else a13 = _net->_clone->get_arc(n1,n3)->clone();
                a13->src = _net->get_node(a13->src->_name);
                a13->dest = _net->get_node(a13->dest->_name);
                a13->connect();
                _net->add_arc(a13);
            }
            a13->init_vars(SDP);
            _model->addVar(a13->wr);
            _model->addVar(a13->wi);
        }

        if(!a32 || a32->status == 0) {
            if (!a32) {
                if(rot_bag) a32 = _net->_clone->get_arc(n2,n3)->clone();
                else a32 = _net->_clone->get_arc(n3,n2)->clone();
                a32->src = _net->get_node(a32->src->_name);
                a32->dest = _net->get_node(a32->dest->_name);
                a32->connect();
                _net->add_arc(a32);
            }
            a32->init_vars(SDP);
            _model->addVar(a32->wr);
            _model->addVar(a32->wi);
        }

        if(!a12 || a12->status == 0) {
            if (!a12) {
                a12 = _net->_clone->get_arc(n1,n2)->clone();
                a12->src = _net->get_node(a12->src->_name);
                a12->dest = _net->get_node(a12->dest->_name);
                a12->connect();
                _net->add_arc(a12);
            }
            a12->init_vars(SDP);
            _model->addVar(a12->wr);
            _model->addVar(a12->wi);
        }

        if ((a12->free && a13->free) || (a12->free && a32->free) || (a13->free && a32->free)) {
            cout << "\nAt least two free lines in bag, returning 'satisfied'.";
            // There can never be one free line in a bag, because then it would be fixed
            return true;
        }

        if (a12->status) {
            wr12 = a12->wr.get_value();
            wi12 = a12->wi.get_value();
        }
        if (a13->status) {
            wr13 = a13->wr.get_value();
            wi13 = a13->wi.get_value();
        }
        if (a32->status) {
            wr32 = a32->wr.get_value();
            wi32 = a32->wi.get_value();
        }
        double w1 = _net->get_node(n1->_name)->w.get_value();
        double w2 = _net->get_node(n2->_name)->w.get_value();
        double w3 = _net->get_node(n3->_name)->w.get_value();

        // if we get to here, at most one line is missing

        // Calculate values for missing lines
        cout << "\n";
        // Calculate the parameters of the circles; not rot: 13, 32; rot: 31, 23. 12 same for both
        if(a12->imaginary && !a12->status) {
            cout << "Calculating values for " << a12->_name;
            if(!rot_bag) {
                wr12 = (wr32 * wr13 - wi32 * wi13) / w3;
                wi12 = (wi32 * wr13 + wr32 * wi13) / w3;
            }else{
                wr12 = (wr32*wr13 - wi32*wi13)/w3;
                wi12 = (-wi32*wr13 - wr32*wi13)/w3;
            }
            a12->wr.set_val(wr12);
            a12->wi.set_val(wi12);
            if (!(wr12 >= a12->wr.get_lb() && wr12 <= a12->wr.get_ub() && wi12 >= a12->wi.get_lb() && wi12 <= a12->wi.get_ub())) {
                cout << "\nBounds are violated";
                return false;
            }
            //cout << "\nSDP: wr12*" << 2*(wr32*wr13 - wi32*wi13) << " + wi12*" << 2*(wi32*wr13 + wr32*wi13);
            //cout << " - (wr12^2 + wi12^2)*" << w3 << " + " << w1*w2*w3 - (wr13*wr13 + wi13*wi13)*w2 - (wr32*wr32 + wi32*wi32)*w1;
            a12->status = 1;
        }
        if(a13->imaginary && !a13->status) {
            cout << "Calculating values for " << a13->_name;
            if(!rot_bag) {
                wr13 = (wr12 * wr32 + wi12 * wi32) / w2;
                wi13 = (wi12 * wr32 - wr12 * wi32) / w2;
            }else{
                wr13 = (wr12*wr32 - wi12*wi32)/w2;
                wi13 = (-wi12*wr32 - wr12*wi32)/w2;
            }
            a13->wr.set_val(wr13);
            a13->wi.set_val(wi13);
            if (!(wr13 >= a13->wr.get_lb() && wr13 <= a13->wr.get_ub() && wi13 >= a13->wi.get_lb() && wi13 <= a13->wi.get_ub())) {
                cout << "\nBounds are violated";
                return false;
            }
            //cout << "\nSDP: wr13*" << 2*(wr12*wr32 + wi12*wi32) << " + wi13*" << 2*(wi12*wr32 - wr12*wi32);//
            //cout << " - (wr13^2 + wi13^2)*" << w2 << " + " << w1*w2*w3 - (wr32*wr32 + wi32*wi32)*w1 - (wr12*wr12 + wi12*wi12)*w3;//
            a13->status = 1;
        }
        if(a32->imaginary && !a32->status) {
            cout << "Calculating values for " << a32->_name;
            if(!rot_bag) {
                wr32 = (wr12 * wr13 + wi12 * wi13) / w1;
                wi32 = (wi12 * wr13 - wr12 * wi13) / w1;
            }else{
                wr32 = (wr12*wr13 - wi12*wi13)/w1;
                wi32 = (-wi12*wr13 - wr12*wi13)/w1;
            }
            a32->wr.set_val(wr32);
            a32->wi.set_val(wi32);
            if (!(wr32 >= a32->wr.get_lb() && wr32 <= a32->wr.get_ub() && wi32 >= a32->wi.get_lb() && wi32 <= a32->wi.get_ub())) {
                cout << "\nBounds are violated";
                return false;
            }
            //cout << "\nSDP: wr32*" << 2*(wr12*wr13 + wi12*wi13) << " + wi12*" << 2*(wi12*wr13 - wr12*wi13);
            //cout << " - (wr32^2 + wi32^2)*" << w1 << " + " << w1*w2*w3 - (wr13*wr13 + wi13*wi13)*w2 - (wr12*wr12 + wi12*wi12)*w3;
            a32->status = 1;
        }

        //------------------------
        cout << "\nCheck bag with all known lines";
        if (!rot_bag) {
            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
            SDP *= 2;
            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
            SDP += w1*w2*w3;
            cout << "\nSDP = " << SDP;
            if(SDP > -tol) return true;
            else {
                return false;
            }
        }else{
            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
            SDPr *= 2;
            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
            SDPr += w1*w2*w3;
            cout << "\nSDPr = " << SDPr;
            if(SDPr > -tol) return true;
            else {
                return false;
            }
        }
    }
    return true;
}

bool PowerModel::SDP_satisfied(vector<Node*>* b){
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;

    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;

    double wr12, wi12;
    double wr13, wi13;
    double wr32, wi32;

    double tol = 0.000001;
    double xc, yc, R1, R2; // center of second circle and squared radii

    bool rot_bag;
    int n_i1, n_i2, n_i3 = 0;

    Arc* new_arc = nullptr;

    if (b->size()==3) {
        rot_bag = _net->rotation_bag(b);
        if (rot_bag) {
            n1 = b->at(0);
            if (_net->_clone->has_directed_arc(n1, b->at(1))) {
                n2 = b->at(1);
                n3 = b->at(2);
            }
            else {
                n2 = b->at(2);
                n3 = b->at(1);
            }
            a13 = _net->get_arc(n3, n1);
            a32 = _net->get_arc(n2, n3);
        }
        else {
            n_i1 = get_order(b, 1);
            n1 = b->at(n_i1);
            n_i2 = get_order(b, 2);
            n2 = b->at(n_i2);
            n_i3 = get_order(b, 3);
            n3 = b->at(n_i3);
            a13 = _net->get_arc(n1, n3);
            a32 = _net->get_arc(n3, n2);
        }
        a12 = _net->get_arc(n1, n2);
        //cout << "\nn1 = " << n1->_name << " n2 = " << n2->_name << " n3 = " << n3->_name;

        if(!a13 || a13->status == 0) {
            if (!a13) {
                if(rot_bag) a13 = _net->_clone->get_arc(n3,n1)->clone();
                else a13 = _net->_clone->get_arc(n1,n3)->clone();

                a13->src = _net->get_node(a13->src->_name);
                a13->dest = _net->get_node(a13->dest->_name);
//                a13->free = true;
                a13->connect();
                _net->add_arc(a13);
            }
//            cout << "\nAdding a13";
            a13->status = 1;
            a13->init_vars(SDP);
            _model->addVar(a13->wr);
            _model->addVar(a13->wi);
            //new_arc = a13;
            //a13 = nullptr;
            a13->imaginary = true;
        }

        if(!a32 || a32->status == 0) {
            if (!a32) {
                if(rot_bag) a32 = _net->_clone->get_arc(n2,n3)->clone();
                else a32 = _net->_clone->get_arc(n3,n2)->clone();

                a32->src = _net->get_node(a32->src->_name);
                a32->dest = _net->get_node(a32->dest->_name);
//                a32->free = true;
                a32->connect();
                _net->add_arc(a32);
            }
//            cout << "\nAdding a32";
            a32->status=1;
            a32->init_vars(SDP);
            _model->addVar(a32->wr);
            _model->addVar(a32->wi);
            //new_arc = a32;
            //a32 = nullptr;
            a32->imaginary = true;
        }

        if(!a12 || a12->status == 0) {
            if (!a12) {
                a12 = _net->_clone->get_arc(n1,n2)->clone();

                a12->src = _net->get_node(a12->src->_name);
                a12->dest = _net->get_node(a12->dest->_name);
//                a12->free = true;
                //if(!a12->free) {cout << "\n12 !Imaginary"; a12->free = true;}
                a12->connect();
                _net->add_arc(a12);
            }
//            cout << "\nAdding a12";
            a12->status=1;
            a12->init_vars(SDP);
            _model->addVar(a12->wr);
            _model->addVar(a12->wi);
            //new_arc = a12;
            //a12 = nullptr;
            a12->imaginary = true;
        }

        if (a12->imaginary && a13->imaginary && a32->imaginary) { //nothing to check for bags without lines
            //cout << "\nNo lines in bag, returning 'satisfied'.";
            return true;
        }

        if (!a12->imaginary) {
            cout << "a12 real; ";
            wr12 = a12->wr.get_value();
            wi12 = a12->wi.get_value();
        }
        if (!a13->imaginary) {
            cout << "a13 real; ";
            wr13 = a13->wr.get_value();
            wi13 = a13->wi.get_value();
        }
        if (!a32->imaginary) {
            cout << "a32 real; ";
            wr32 = a32->wr.get_value();
            wi32 = a32->wi.get_value();
        }
        double w1 = _net->get_node(n1->_name)->w.get_value();
        double w2 = _net->get_node(n2->_name)->w.get_value();
        double w3 = _net->get_node(n3->_name)->w.get_value();

        if((a12->imaginary && a13->imaginary) || (a12->imaginary && a32->imaginary) || (a13->imaginary && a32->imaginary)) {
            //cout << "\nTwo lines are missing. Return 'satisfied'.";
            return true;
        }

        // if we get to here, at most one line is missing
        if(!rot_bag) { // Calculate the parameters of the circles; not rot: 13, 32; rot: 31, 23. 12 same for both
            if(a12->imaginary) {
                xc = (wr32*wr13 - wi32*wi13)/w3;
                yc = (wi32*wr13 + wr32*wi13)/w3;
                R1 = (wr32*wr32*wr13*wr13 + wi32*wi32*wi13*wi13 + wi32*wi32*wr13*wr13 + wr32*wr32*wi13*wi13)/(w3*w3);
                R1 -= ((wr13*wr13+wi13*wi13)*w2 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w3;
                //cout << "\nR1 = " << R1 << " xc = " << xc << " yc = " << yc;
                R2 = w1*w2;
                //cout << "\nSDP: wr12*" << 2*(wr32*wr13 - wi32*wi13) << " + wi12*" << 2*(wi32*wr13 + wr32*wi13);
                //cout << " - (wr12^2 + wi12^2)*" << w3 << " + " << w1*w2*w3 - (wr13*wr13 + wi13*wi13)*w2 - (wr32*wr32 + wi32*wi32)*w1;
                //cout << "SOC1 res: " << wr32*wr32+wi32*wi32 - w2*w3 << ", SOC2 res: " << wr13*wr13+wi13*wi13 - w1*w3;
                if ((wr32*wr32+wi32*wi32 - w2*w3 < -0.0001) && (wr13*wr13+wi13*wi13 - w1*w3 < -0.0001)) cout << "\none of the socs is inactive.";
            }
            if(a13->imaginary) {
                xc = (wr12*wr32 + wi12*wi32)/w2;//
                yc = (wi12*wr32 - wr12*wi32)/w2;//
                R1 = (wr12*wr12*wr32*wr32 + wi12*wi12*wi32*wi32 + wi12*wi12*wr32*wr32 + wr12*wr12*wi32*wi32)/(w2*w2);//
                R1 -= ((wr32*wr32+wi32*wi32)*w1 + (wr12*wr12+wi12*wi12)*w3 - w1*w2*w3)/w2;//
                //cout << "\nR1 = " << R1 << " xc = " << xc << " yc = " << yc;//
                R2 = w1*w3;
                //cout << "\nSDP: wr13*" << 2*(wr12*wr32 + wi12*wi32) << " + wi13*" << 2*(wi12*wr32 - wr12*wi32);//
                //cout << " - (wr13^2 + wi13^2)*" << w2 << " + " << w1*w2*w3 - (wr32*wr32 + wi32*wi32)*w1 - (wr12*wr12 + wi12*wi12)*w3;//
                //cout << "\nSOCP: wr13^2 + wi13^2 <= " << R2;
                //cout << "\nFree vars: " << new_arc->wr._name << ", " << new_arc->wr._name;
                //cout << "SOC1 res: " << wr32*wr32+wi32*wi32 - w2*w3 << ", SOC2 res: " << wr12*wr12+wi12*wi12 - w1*w2;
                if ((wr32*wr32+wi32*wi32 - w2*w3 < -0.0001) && (wr12*wr12+wi12*wi12 - w1*w2 < -0.0001)) cout << "\none of the socs is inactive.";
            }
            if(a32->imaginary) {
                xc = (wr12*wr13 + wi12*wi13)/w1;
                yc = (wi12*wr13 - wr12*wi13)/w1;
                R1 = (wr12*wr12*wr13*wr13 + wi12*wi12*wi13*wi13 + wi12*wi12*wr13*wr13 + wr12*wr12*wi13*wi13)/(w1*w1);
                R1 -= ((wr13*wr13+wi13*wi13)*w2 + (wr12*wr12+wi12*wi12)*w3 - w1*w2*w3)/w1;
                //cout << "\nR1 = " << R1 << " xc = " << xc << " yc = " << yc;
                R2 = w3*w2;
                //cout << "\nSDP: wr32*" << 2*(wr12*wr13 + wi12*wi13) << " + wi12*" << 2*(wi12*wr13 - wr12*wi13);
                //cout << " - (wr32^2 + wi32^2)*" << w1 << " + " << w1*w2*w3 - (wr13*wr13 + wi13*wi13)*w2 - (wr12*wr12 + wi12*wi12)*w3;
//                cout << "SOC1 res: " << wr13*wr13+wi13*wi13 - w1*w3 << ", SOC2 res: " << wr12*wr12+wi12*wi12 - w1*w2;
                if ((wr12*wr12+wi12*wi12 - w2*w1 < -0.0001) && (wr13*wr13+wi13*wi13 - w1*w3 < -0.0001)) cout << "\none of the socs is inactive.";
            }
        }else{
            if(a12->imaginary) {
                xc = (wr32*wr13 + wi32*wi13)/w3;
                yc = (wi32*wr13 - wr32*wi13)/w3;
                R1 = (wr32*wr32*wr13*wr13 + wi32*wi32*wi13*wi13 + wi32*wi32*wr13*wr13 + wr32*wr32*wi13*wi13)/(w3*w3);
                R1 -= ((wr13*wr13+wi13*wi13)*w2 + (wr32*wr32+wi32*wi32)*w1 - w1*w2*w3)/w3; // stay the same because all wi are squared?
                //cout << "\nR1 = " << R1 << " xc = " << xc << " yc = " << yc;
                R2 = w1*w2;
                //cout << "\nSDPr: wr12*" << 2*(wr32*wr13 - wi32*wi13) << " - wi12*" << 2*(wi32*wr13 + wr32*wi13);
                //cout << " - (wr12^2 + wi12^2)*" << w3 << " + " << w1*w2*w3 - (wr13*wr13 + wi13*wi13)*w2 - (wr32*wr32 + wi32*wi32)*w1;
            }
            if(a13->imaginary) {
                //R2 = w1*w3;
                cout << "\nNOT IMPLEMENTED ";
            }
            if(a32->imaginary) {
                //R2 = w3*w2;
                cout << "\nNOT IMPLEMENTED ";
            }
        }

        // If a line is missing - check if the circles intersect and return
        if (a12->imaginary) {
            return circles_intersect(xc, yc, R1, R2, a12);
        }
        if (a13->imaginary) {
            return circles_intersect(xc, yc, R1, R2, a13);
        }
        if (a32->imaginary) {
            return circles_intersect(xc, yc, R1, R2, a32);
        }

        // If we get here, then all lines are in place. SOCP should already be satisfied.
        if (!rot_bag) {
            double SDP = wr12*(wr32*wr13 - wi32*wi13) + wi12*(wi32*wr13 + wr32*wi13);
            SDP *= 2;
            SDP -= (wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1;
            SDP += w1*w2*w3;
            if(SDP > -tol) return true;
            else {
                cout << "\nSDP = " << SDP;
                return false;
            }
        }else{
            double SDPr = wr12*(wr32*wr13 - wi32*wi13) - wi12*(wi32*wr13 + wr32*wi13);
            SDPr *= 2;
            SDPr -= ((wr12*wr12 + wi12*wi12)*w3 + (wr13*wr13 + wi13*wi13)*w2 + (wr32*wr32 + wi32*wi32)*w1);
            SDPr += w1*w2*w3;
            if(SDPr > -tol) return true;
            else {
                cout << "\nSDPr = " << SDPr;
                return false;
            }
        }

    }
    return true;
}

void PowerModel::add_bag_SOCP(Bag* b, int id){
    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    n1 = b->_n1;
    n2 = b->_n2;
    n3 = b->_n3;
    a12 = _net->get_arc(n1,n2);
    a13 = _net->get_arc(n1,n3);
    a32 = _net->get_arc(n3,n2);

    if(a12->imaginary && !a12->added) {
        a12->added = true;
        Constraint SOCP1("SOCP1_"+to_string(id));
        SOCP1 += _net->get_node(n1->_name)->w*_net->get_node(n2->_name)->w;
        SOCP1 -= ((a12->wr)^2);
        SOCP1 -= ((a12->wi)^2);
        SOCP1 >= 0;
        SOCP1.is_cut = true;
        _model->addConstraint(SOCP1);
    }
    if(a13->imaginary && !a13->added) {
        a13->added = true;
        Constraint SOCP2("SOCP2_"+to_string(id));
        SOCP2 += _net->get_node(n1->_name)->w*_net->get_node(n3->_name)->w;
        SOCP2 -= ((a13->wr)^2);
        SOCP2 -= ((a13->wi)^2);
        SOCP2 >= 0;
        SOCP2.is_cut = true;
        _model->addConstraint(SOCP2);
    }
    if(a32->imaginary && !a32->added) {
        a32->added = true;
        Constraint SOCP3("SOCP3_"+to_string(id));
        SOCP3 += _net->get_node(n3->_name)->w*_net->get_node(n2->_name)->w;
        SOCP3 -= ((a32->wr)^2);
        SOCP3 -= ((a32->wi)^2);
        SOCP3 >= 0;
        SOCP3.is_cut = true;
        _model->addConstraint(SOCP3);
    }
}
#ifdef USE_IGRAPH
#ifdef USE_ARMADILLO
bool PowerModel::clique_psd(Bag* b, igraph_vector_t* cl){
    double wall0 = get_wall_time1();
    double tol = 0.0005;
    int n = igraph_vector_size(cl); //elements of the vector are long int
    Node* node;
    arma::cx_mat A(n,n);
    cout << "\nSize of clique = " << n;
    vector<Node*> clique_nodes;
    cout << "\nNodes in clique: ";
    for(int i = 0; i< n; i++){
        node = b->_nodes->at((long int)VECTOR(*cl)[i]);
        clique_nodes.push_back(node);
        cout << clique_nodes.at(i)->_name << ", ";
    }
    sort(clique_nodes.begin(), clique_nodes.end(), [](Node* a, Node* b) { return a->ID < b->ID; });
//    cout << "\nSorted nodes: ";
//    for(int i = 0; i< n; i++){
//        cout << "\nNode = " << clique_nodes.at(i)->_name << ", id = " << clique_nodes.at(i)->ID;
//    }

    for(int i = 0; i < n; i++){
        for(int j = 0; j <= i; j++) {
            if(i==j) {
                A(i,j) = arma::cx_double(_net->get_node(clique_nodes[i]->_name)->w.get_value(),0);
            }else{
                double AijI;
//                AijR = _net->get_arc(clique_nodes[j],clique_nodes[i])->wr.get_value();
                if(_net->has_directed_arc(clique_nodes[j],clique_nodes[i])) {
                    AijI = _net->get_arc(clique_nodes[j], clique_nodes[i])->wi.get_value();
                }
                else {
                    AijI = -_net->get_arc(clique_nodes[j], clique_nodes[i])->wi.get_value();
                }
                A(i,j) = arma::cx_double(_net->get_arc(clique_nodes[j],clique_nodes[i])->wr.get_value(),AijI);
                A(j,i) = arma::cx_double(_net->get_arc(clique_nodes[j],clique_nodes[i])->wr.get_value(),-AijI);
            }
        }
    }
//    A.print();
    arma::cx_mat R;
    arma::vec v = arma::eig_sym(A);
    cout << "\n";
    double min_eig = 0, max_eig = -1;
    for(auto eig: v) {
        if(eig < min_eig) min_eig = eig;
        if(eig > max_eig) max_eig = eig;
    }
    double wall1 = get_wall_time1();
    cout << "\nEigen decomposition took " << wall1-wall0 << "s";
    if(min_eig/max_eig > -tol) return true;
    else return false;
//    cout << "\nv:";
//    v.print();
//    cout << "\n";
}
#endif
#endif

Complex* PowerModel::determinant(vector<vector<Complex*>> A){
    int n = A.size();
    if(n==0) {
        cerr << "\ndeterminant() called for an empty matrix";
        return NULL;
    }
    cout << "\nMatrix:\n";
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++) {
            A[i][j]->print();
        }
        cout << "\n";
    }
    Complex* res = new Complex();
    if(n>2) {
        for(auto& a1i: A[0]){
            //construct a matrix of the corresponding minor
            //get its determinant, multiply it by (-1)^i(?)a1i
            //add it to res
        }
    }
    else if(n==2) {
        *res = (*A[0][0])*(*A[1][1]) - (*A[0][1])*(*A[1][0]);
        cout << "\nres in det: ";
        res->print();
    }else if(n==1){
        *res = (*A[0][0]);
    }
    return res;
}

#ifdef USE_IGRAPH
void PowerModel::add_clique(Bag* b, igraph_vector_t* cl){
    cout << "\nThis clique should be added.";
    Complex* det;
    vector<vector<Complex*>> A;
//    int n = igraph_vector_size(cl);
    int n = 2;//for testing
    Node* node;
    vector<Node*> clique_nodes;
    for(int i = 0; i< n; i++){
        node = b->_nodes->at((long int)VECTOR(*cl)[i]);
        clique_nodes.push_back(node);
    }
    sort(clique_nodes.begin(), clique_nodes.end(), [](Node* a, Node* b) { return a->ID < b->ID; });

    Complex* entry;
    for(int i = 0; i < n; i++){
        vector<Complex*> Arow;
        for(int j = 0; j < n; j++) {
            if(i==j) {
                if(i==1) cout << "\nA[1][1] = " << clique_nodes[i]->_name;
                entry = new Complex("W",_net->get_node(clique_nodes[i]->_name)->w,0);
                Arow.push_back(entry);
            }else{
                Function AijI;
                if(_net->has_directed_arc(clique_nodes[j],clique_nodes[i])) {
                    AijI = _net->get_arc(clique_nodes[j], clique_nodes[i])->wi;
                }
                else {
                    AijI = 0-_net->get_arc(clique_nodes[j], clique_nodes[i])->wi;
                }
                entry = new Complex("W",_net->get_arc(clique_nodes[j],clique_nodes[i])->wr,AijI);
                Arow.push_back(entry);
            }
        }
        A.push_back(Arow);
    }
//
    det = determinant(A);
    //loop through minors with decreasing size
    //create a method that will create a det function given a minor
    //det function >= 0
}
#endif

#ifdef USE_IGRAPH
void PowerModel::add_violated_cliques(Bag* b) {// for bags with more than 3 nodes
    igraph_vector_ptr_t* res = new igraph_vector_ptr_t; // will contain indices of vertices in cliques
    igraph_vector_t* clique;
    igraph_vector_ptr_init(res,0);
    igraph_maximal_cliques(b->graph,res,0,b->size());
    for(long i = 0; i < igraph_vector_ptr_size(res); i++) {
//        clique = (igraph_t*)igraph_vector_ptr_e(res,i);
        clique = (igraph_vector_t*)VECTOR(*res)[i];
        if(!clique_psd(b, clique)) add_clique(b, clique);
        else cout << "\nClique is PSD.";
    }
    igraph_vector_ptr_destroy(res);
}
#endif

int PowerModel::add_SDP_cuts_dyn(){
    int sdp_alg = _net->sdp_alg;

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
    SDP->is_cut = true;
    _model->addMetaConstraint(*SDP);

    meta_Constraint* SDPr = new meta_Constraint();
    *SDPr = (*wr12)*((*wr23)*(*wr31) - (*wi23)*(*wi31));
    *SDPr -= (*wi12)*((*wi23)*(*wr31) + (*wr23)*(*wi31));
    *SDPr *= 2;
    *SDPr -= (((*wr12)^2)+((*wi12)^2))*(*w3);
    *SDPr -= (((*wr31)^2)+((*wi31)^2))*(*w2);
    *SDPr -= (((*wr23)^2)+((*wi23)^2))*(*w1);
    *SDPr += (*w1)*(*w2)*(*w3);
    *SDPr >= 0;
    SDPr->is_cut = true;
    _model->addMetaConstraint(*SDPr);

//    _model->init_functions(_net->_bagsnew->size());
//    _model->print_functions();

    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    int id = 0;

    for (auto b: *_net->_bagsnew){
        cout << "\n\nBag number = " << b->_id << ": ";
//        if(b->size() > 3){
//            cout << "\nBag " << b->_id << " of size " << b->size();

//            if(!b->graph_is_chordal()) {
//                cout << "\nBag graph is non chordal, skipping";
//                continue;
//            };
//            add_violated_cliques(b);
//        }else if(b->size()==3){
          if(b->size()==3) {
              if (!SDP_satisfied_new1(b)) {
                  b->_added = true;
                  add_bag_SOCP(b, id);
                  n1 = b->_n1;
                  n2 = b->_n2;
                  n3 = b->_n3;
                  a12 = _net->get_arc(n1, n2);
                  a13 = _net->get_arc(n1, n3);
                  a32 = _net->get_arc(n3, n2);
//            cout << "\nn1 = " << n1->_name << " n2 = " << n2->_name << " n3 = " << n3->_name;
//            cout << "\na12 = " << a12->_name << " a13 = " << a13->_name << " a32 = " << a32->_name;
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
                  *w1 = _net->get_node(n1->_name)->w;
                  *w2 = _net->get_node(n2->_name)->w;
                  *w3 = _net->get_node(n3->_name)->w;
                  if (b->_rot) _model->concretise(*SDPr, id, "SDP" + to_string(id));
                  else _model->concretise(*SDP, id, "SDP" + to_string(id));
                  id++;
              }
//              if (!b->graph_is_chordal()) {
//                  cout << "\nBag graph is non chordal, skipping";
//                  continue;
//              };
          }
#ifdef USE_IGRAPH
#ifdef USE_ARMADILLO
        add_violated_cliques(b);
#else
        cout << "\nHigher dimensional cuts can't be generated without Armadillo";
#endif
#else
        cout << "\nHigher dimensional cuts can't be generated without Igraph";
#endif
    }


    if(sdp_alg==2) {
        for (auto b: *_net->_bagsnew) {
//            cout << "\n\nBag number = " << b->_id << ": ";
            if (b->_need_to_fix && !b->_added) {
//                cout << "\nADDING NEW CUT " << b->_id;
                if (b->size() != 3) continue;

                n1 = b->_n1;
                n2 = b->_n2;
                n3 = b->_n3;
                a12 = _net->get_arc(n1, n2);
                a13 = _net->get_arc(n1, n3);
                a32 = _net->get_arc(n3, n2);

                add_bag_SOCP(b, id);

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
                *w1 = _net->get_node(n1->_name)->w;
                *w2 = _net->get_node(n2->_name)->w;
                *w3 = _net->get_node(n3->_name)->w;
                if (b->_rot) _model->concretise(*SDPr, id, "SDP" + to_string(id));
                else _model->concretise(*SDP, id, "SDP" + to_string(id));
                id++;
            }
        }
    }
    _model->init_functions(_net->_bagsnew->size());
    _model->print_functions();
    cout << "\nNumber of added cuts = " << id << endl;
    return id;
}

void PowerModel::add_SDP_OA_projections(){
    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    int id = 0, skipped = 0;
    double tol = 0.001;
    double wr12, wr13, wr32, wi12, wi13, wi32, w1, w2, w3;
    for (auto b: *_net->_bagsnew){
        cout << "\n\nBag number = " << b->_id << ": ";
        if(b->size()==3) {
            if (!SDP_satisfied_new1(b)) {
                cout << "\nrot = " << b->_rot;
                double* x = new double[_model->get_nb_vars()];
                add_bag_SOCP(b, id);
                n1 = b->_n1;
                n2 = b->_n2;
                n3 = b->_n3;
                a12 = _net->get_arc(n1, n2);
                a13 = _net->get_arc(n1, n3);
                a32 = _net->get_arc(n3, n2);

//                if(a12->wr.get_value()*a12->wr.get_value() + a12->wi.get_value()*a12->wi.get_value()
//                   - _net->get_node(n1->_name)->w.get_value()*_net->get_node(n2->_name)->w.get_value() > tol) continue;
//                if(a13->wr.get_value()*a13->wr.get_value() + a13->wi.get_value()*a13->wi.get_value()
//                   - _net->get_node(n1->_name)->w.get_value()*_net->get_node(n3->_name)->w.get_value() > tol) continue;
//                if(a32->wr.get_value()*a32->wr.get_value() + a32->wi.get_value()*a32->wi.get_value()
//                   - _net->get_node(n3->_name)->w.get_value()*_net->get_node(n2->_name)->w.get_value() > tol) continue;

                Function SDPf;
                if (b->_rot) {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) - a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                else {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) + a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                SDPf *= 2;
                SDPf -= (((a12->wr)^2)+((a12->wi)^2))*(_net->get_node(n3->_name)->w);
                SDPf -= (((a13->wr)^2)+((a13->wi)^2))*(_net->get_node(n2->_name)->w);
                SDPf -= (((a32->wr)^2)+((a32->wi)^2))*(_net->get_node(n1->_name)->w);
                SDPf += (_net->get_node(n1->_name)->w)*(_net->get_node(n2->_name)->w)*(_net->get_node(n3->_name)->w);

//                cout << "\nSDPf: ";
//                SDPf.print(false);

                wr12 = a12->wr.get_value();
                wr13 = a13->wr.get_value();
                wr32 = a32->wr.get_value();
                wi12 = a12->wi.get_value();
                wi13 = a13->wi.get_value();
                wi32 = a32->wi.get_value();
                w1 = _net->get_node(n1->_name)->w.get_value();
                w2 = _net->get_node(n2->_name)->w.get_value();
                w3 = _net->get_node(n3->_name)->w.get_value();

                cout << "\nGenerating cuts at: wr12 = " << wr12 << ", wr13 = " << wr13 << ", wr32 = " << wr32;
                cout << ",\nwi12 = " << wi12 << ", wi13 = " << wi13 << ", wi32 = " << wi32;
                cout << ",\nw1 = " << w1 << ", w2 = " << w2 << ", w3 = " << w3;

                x[a12->wr.get_idx()] = wr12;
                x[a13->wr.get_idx()] = wr13;
                x[a32->wr.get_idx()] = wr32;
                x[a12->wi.get_idx()] = wi12;
                x[a13->wi.get_idx()] = wi13;
                x[a32->wi.get_idx()] = wi32;
                x[_net->get_node(n2->_name)->w.get_idx()] = w2;
                x[_net->get_node(n3->_name)->w.get_idx()] = w3;

                double Dwr12 = 4*(wr32*wr13 - wi32*wi13)*(wr32*wr13 - wi32*wi13) + 4*w3*(2*wi12*(wi32*wr13 + wr32*wi13) - wi12*wi12*w3 - (wr13*wr13+wi13*wi13)*w2 - (wr32*wr32+wi32*wi32)*w1 + w1*w2*w3);
                double Dwi12 = 4*(wi32*wr13 + wr32*wi13)*(wi32*wr13 + wr32*wi13) + 4*w3*(2*wr12*(wr32*wr13 - wi32*wi13) - wr12*wr12*w3 - (wr13*wr13+wi13*wi13)*w2 - (wr32*wr32+wi32*wi32)*w1 + w1*w2*w3);
                cout << "\nDwr12 = " << Dwr12 << ", Dwi12 = " << Dwi12 << endl;

                if(!b->_rot)
                    x[_net->get_node(n1->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) +
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr12*wr12+wi12*wi12)*w3 - (wr13*wr13+wi13*wi13)*w2 ) /
                    ( wr32*wr32 + wi32*wi32 - w2*w3 );
                else
                    x[_net->get_node(n1->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) -
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr12*wr12+wi12*wi12)*w3 - (wr13*wr13+wi13*wi13)*w2 ) /
                    ( wr32*wr32 + wi32*wi32 - w2*w3 );

                Constraint SDPw1("SDPw1");
                SDPw1 += SDPf.outer_approx(x);
                SDPw1 >= 0;
                SDPw1.print();
                if(wr12*wr12+wi12*wi12 <= x[_net->get_node(n1->_name)->w.get_idx()]*w2 + tol
                   && wr13*wr13+wi13*wi13 <= x[_net->get_node(n1->_name)->w.get_idx()]*w3 + tol
                   && wr32*wr32+wi32*wi32 <= w2*w3 + tol) {
                    _model->addConstraint(SDPw1);
                    id++;
                }else skipped++;


                x[_net->get_node(n1->_name)->w.get_idx()] = w1;
                if(!b->_rot)
                    x[_net->get_node(n2->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) +
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr12*wr12+wi12*wi12)*w3 - (wr32*wr32+wi32*wi32)*w1 ) /
                    ( wr13*wr13 + wi13*wi13 - w1*w3 );
                else
                    x[_net->get_node(n2->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) -
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr12*wr12+wi12*wi12)*w3 - (wr32*wr32+wi32*wi32)*w1 ) /
                    ( wr13*wr13 + wi13*wi13 - w1*w3 );

                Constraint SDPw2("SDPw2");
                SDPw2 += SDPf.outer_approx(x);
                SDPw2 >= 0;
                SDPw2.print();
                if(wr12*wr12+wi12*wi12 <= w1*x[_net->get_node(n2->_name)->w.get_idx()] + tol
                   && wr13*wr13+wi13*wi13 <= w1*w3 + tol
                   && wr32*wr32+wi32*wi32 <= x[_net->get_node(n2->_name)->w.get_idx()]*w3 + tol) {
                    _model->addConstraint(SDPw2);
                    id++;
                }else skipped++;


                x[_net->get_node(n2->_name)->w.get_idx()] = w2;
                if(!b->_rot)
                    x[_net->get_node(n3->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) +
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr32*wr32+wi32*wi32)*w1 - (wr13*wr13+wi13*wi13)*w2 ) /
                    ( wr12*wr12 + wi12*wi12 - w1*w2 );
                else
                    x[_net->get_node(n3->_name)->w.get_idx()] = ( 2*wr12*(wr32*wr13 - wi32*wi13) -
                    2*wi12*(wi32*wr13 + wr32*wi13) - (wr32*wr32+wi32*wi32)*w1 - (wr13*wr13+wi13*wi13)*w2 ) /
                    ( wr12*wr12 + wi12*wi12 - w1*w2 ); //what if socp12 is active?
                Constraint SDPw3("SDPw3");
                SDPw3 += SDPf.outer_approx(x);
                SDPw3 >= 0;
                SDPw3.print();
                if(wr12*wr12+wi12*wi12 <= w1*w2 + tol
                   && wr13*wr13+wi13*wi13 <= w1*x[_net->get_node(n3->_name)->w.get_idx()] + tol
                   && wr32*wr32+wi32*wi32 <= w2*x[_net->get_node(n3->_name)->w.get_idx()] + tol) {
                    _model->addConstraint(SDPw3);
                    id++;
                }else skipped++;

                x[_net->get_node(n3->_name)->w.get_idx()] = w3;
                if(Dwi12 > -0.0001) {
                    x[a12->wi.get_idx()] = (wi32*wr13 + wr32*wi13) / w3;
                    Constraint SDPwi12("SDPwi12");
                    SDPwi12 += SDPf.outer_approx(x);
                    SDPwi12 >= 0;
                    SDPwi12.print();
                    if(wr12*wr12+x[a12->wi.get_idx()]*x[a12->wi.get_idx()] <= w1*w2 + tol
                       && wr13*wr13+wi13*wi13 <= w1*w3 + tol
                       && wr32*wr32+wi32*wi32 <= w2*w3 + tol) {
                        _model->addConstraint(SDPwi12);
                        id++;
                    }else skipped++;
                }

//                SDP.print();
                delete[] x;
            }
        }
    }
    cout << "\nNumber of added cuts = " << id << ", skipped cuts = " << skipped << endl;
}

/* Generate a hyperplane orthogonal to x1-x0 such that x0 lies on this hyperplane.
 * res <= 0 will describe a half-space such that x1-x0 points outward */
Function PowerModel::ort_plane(const double *x0, const double *x1, vector<int> *ind) const{
    Function res;
    var<double>* xi;
    for(auto idx: *ind) {
        xi = _model->getVar_<double>(idx);
        res += (x1[idx]-x0[idx])*(*xi);
        res -= (x1[idx]-x0[idx])*x0[idx];
    }
    return res;
}

int PowerModel::add_SDP_OA_closest_point(){
    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    int id = 0;
    double wr12, wr13, wr32, wi12, wi13, wi32, w1, w2, w3;
    for (auto b: *_net->_bagsnew){
//        cout << "\n\nBag number = " << b->_id << ": ";
        if(b->size()==3) {
            if (!SDP_satisfied_new1(b)) {
//                cout << "\nrot = " << b->_rot;
//                double* x = new double[_model->get_nb_vars()];
                add_bag_SOCP(b, id);
                n1 = b->_n1; n2 = b->_n2; n3 = b->_n3;
                a12 = _net->get_arc(n1, n2);
                a13 = _net->get_arc(n1, n3);
                a32 = _net->get_arc(n3, n2);
//                cout << "\nn1 = " << n1->_name << ", n2 = " << n2->_name << ", n3 = " << n3->_name;

                wr12 = a12->wr.get_value(); wr13 = a13->wr.get_value(); wr32 = a32->wr.get_value();
                wi12 = a12->wi.get_value(); wi13 = a13->wi.get_value(); wi32 = a32->wi.get_value();
                w1 = _net->get_node(n1->_name)->w.get_value();
                w2 = _net->get_node(n2->_name)->w.get_value();
                w3 = _net->get_node(n3->_name)->w.get_value();

                double* xstar = new double[_model->get_nb_vars()];
                vector<int>* indices = new vector<int>;

                xstar[a12->wr.get_idx()] = wr12; xstar[a13->wr.get_idx()] = wr13; xstar[a32->wr.get_idx()] = wr32;
                xstar[a12->wi.get_idx()] = wi12; xstar[a13->wi.get_idx()] = wi13; xstar[a32->wi.get_idx()] = wi32;
                xstar[_net->get_node(n1->_name)->w.get_idx()] = w1;
                xstar[_net->get_node(n2->_name)->w.get_idx()] = w2;
                xstar[_net->get_node(n3->_name)->w.get_idx()] = w3;

                indices->push_back(a12->wr.get_idx()); indices->push_back(a13->wr.get_idx()); indices->push_back(a32->wr.get_idx());
                indices->push_back(a12->wi.get_idx()); indices->push_back(a13->wi.get_idx()); indices->push_back(a32->wi.get_idx());
                indices->push_back(_net->get_node(n1->_name)->w.get_idx());
                indices->push_back(_net->get_node(n2->_name)->w.get_idx());
                indices->push_back(_net->get_node(n3->_name)->w.get_idx());

//                cout << "\nSDP is violated for x*: \nwr12 = " << wr12 << ", wr13 = " << wr13 << ", wr32 = " << wr32;
//                cout << ",\nwi12 = " << wi12 << ", wi13 = " << wi13 << ", wi32 = " << wi32;
//                cout << ",\nw1 = " << w1 << ", w2 = " << w2 << ", w3 = " << w3;

                Model *m1;
                m1 = new Model();

                var<> wr12v, wr32v, wr13v, wi12v, wi32v, wi13v, w1v, w2v, w3v;
                wr12v.init("wr12v",a12->wr.get_lb(),a12->wr.get_ub());
                wr32v.init("wr32v",a32->wr.get_lb(),a32->wr.get_ub());
                wr13v.init("wr13v",a13->wr.get_lb(),a13->wr.get_ub());
                wi12v.init("wi12v",a12->wi.get_lb(),a12->wi.get_ub());
                wi32v.init("wi32v",a32->wi.get_lb(),a32->wi.get_ub());
                wi13v.init("wi13v",a13->wi.get_lb(),a13->wi.get_ub());
                w1v.init("w1v",_net->get_node(n1->_name)->w.get_lb(),_net->get_node(n1->_name)->w.get_ub());
                w2v.init("w2v",_net->get_node(n2->_name)->w.get_lb(),_net->get_node(n2->_name)->w.get_ub());
                w3v.init("w3v",_net->get_node(n3->_name)->w.get_lb(),_net->get_node(n3->_name)->w.get_ub());
//                wr12v = wr12; wr32v = wr32; wr13v = wr13;
//                wi12v = wi12; wi32v = wi32; wi13v = wi13;
//                w1v = w1; w2v = w2; w3v = w3;
                wr12v = 1; wr32v = 1; wr13v = 1;
                wi12v = 0; wi32v = 0; wi13v = 0;
                w1v = 1;   w2v = 1;   w3v = 1;
                m1->addVar(wr12v); m1->addVar(wr32v); m1->addVar(wr13v);
                m1->addVar(wi12v); m1->addVar(wi32v); m1->addVar(wi13v);
                m1->addVar(w1v); m1->addVar(w2v); m1->addVar(w3v);

                Function* dist = new Function();
                *dist += (wr12-wr12v)*(wr12-wr12v) + (wr32-wr32v)*(wr32-wr32v) + (wr13-wr13v)*(wr13-wr13v);
                *dist += (wi12-wi12v)*(wi12-wi12v) + (wi32-wi32v)*(wi32-wi32v) + (wi13-wi13v)*(wi13-wi13v);
                *dist += (w1-w1v)*(w1-w1v) + (w2-w2v)*(w2-w2v) + (w3-w3v)*(w3-w3v);
//                cout << "\nObj:";
//                dist->print(false);
//                cout << "\n";
                m1->setObjectiveType(minimize);
                m1->setObjective(dist);

                meta_var* wr12m = new meta_var("wr12", m1);
                meta_var* wr13m = new meta_var("wr13", m1);
                meta_var* wr32m = new meta_var("wr32", m1);
                meta_var* wi12m = new meta_var("wi12", m1);
                meta_var* wi13m = new meta_var("wi13", m1);
                meta_var* wi32m = new meta_var("wi32", m1);
                meta_var* w1m = new meta_var("w1", m1);
                meta_var* w2m = new meta_var("w2", m1);
                meta_var* w3m = new meta_var("w3", m1);
                meta_Constraint* SDPm = new meta_Constraint();
                if(!b->_rot) {
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm += (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }else{
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm -= (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }
                *SDPm *= 2;
                *SDPm -= (((*wr12m)^2)+((*wi12m)^2))*(*w3m);
                *SDPm -= (((*wr13m)^2)+((*wi13m)^2))*(*w2m);
                *SDPm -= (((*wr32m)^2)+((*wi32m)^2))*(*w1m);
                *SDPm += (*w1m)*(*w2m)*(*w3m);
                *SDPm >= 0;
                m1->addMetaConstraint(*SDPm);

                m1->init_functions(1);

                *wr12m = wr12v; *wr32m = wr32v; *wr13m = wr13v;
                *wi12m = wi12v; *wi32m = wi32v; *wi13m = wi13v;
                *w1m = w1v; *w2m = w2v; *w3m = w3v;
                m1->concretise(*SDPm, 0, "SDP");

                Constraint SOCP12("SOCP12");
                SOCP12 = wr12v*wr12v + wi12v*wi12v - w1v*w2v;
                SOCP12 <= 0;
                m1->addConstraint(SOCP12);

                Constraint SOCP32("SOCP32");
                SOCP32 = wr32v*wr32v + wi32v*wi32v - w3v*w2v;
                SOCP32 <= 0;
                m1->addConstraint(SOCP32);

                Constraint SOCP13("SOCP13");
                SOCP13 = wr13v*wr13v + wi13v*wi13v - w1v*w3v;
                SOCP13 <= 0;
                m1->addConstraint(SOCP13);

                PTSolver* s1 = new PTSolver(m1,ipopt);
                s1->conv_tol = 1e-12;
                s1->run(0,false);

                wr12 = wr12v.get_value(); wr13 = wr13v.get_value(); wr32 = wr32v.get_value();
                wi12 = wi12v.get_value(); wi13 = wi13v.get_value(); wi32 = wi32v.get_value();
                w1 = w1v.get_value(); w2 = w2v.get_value(); w3 = w3v.get_value();

                delete s1;
                delete m1;

                /* Generate a cut perpendicular to x*-x^ */

                double* xhat = new double[_model->get_nb_vars()];

                xhat[a12->wr.get_idx()] = wr12; xhat[a13->wr.get_idx()] = wr13; xhat[a32->wr.get_idx()] = wr32;
                xhat[a12->wi.get_idx()] = wi12; xhat[a13->wi.get_idx()] = wi13; xhat[a32->wi.get_idx()] = wi32;
                xhat[_net->get_node(n1->_name)->w.get_idx()] = w1;
                xhat[_net->get_node(n2->_name)->w.get_idx()] = w2;
                xhat[_net->get_node(n3->_name)->w.get_idx()] = w3;

                Constraint p("p");
                p = ort_plane(xhat, xstar, indices);
                p <= 0;
//                p.print();
                _model->addConstraint(p);

                /* ------------------------------------- */

//                cout << "\nNearest point x^ is: \nwr12 = " << wr12 << ", wr13 = " << wr13 << ", wr32 = " << wr32;
//                cout << ",\nwi12 = " << wi12 << ", wi13 = " << wi13 << ", wi32 = " << wi32;
//                cout << ",\nw1 = " << w1 << ", w2 = " << w2 << ", w3 = " << w3;

//                Function SDPf;
//                if (b->_rot) {
//                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) - a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
//                }
//                else {
//                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) + a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
//                }
//                SDPf *= 2;
//                SDPf -= (((a12->wr)^2)+((a12->wi)^2))*(_net->get_node(n3->_name)->w);
//                SDPf -= (((a13->wr)^2)+((a13->wi)^2))*(_net->get_node(n2->_name)->w);
//                SDPf -= (((a32->wr)^2)+((a32->wi)^2))*(_net->get_node(n1->_name)->w);
//                SDPf += (_net->get_node(n1->_name)->w)*(_net->get_node(n2->_name)->w)*(_net->get_node(n3->_name)->w);

//                cout << "\nSDPf: ";
//                SDPf.print(false);

//                x[a12->wr.get_idx()] = wr12; x[a13->wr.get_idx()] = wr13; x[a32->wr.get_idx()] = wr32;
//                x[a12->wi.get_idx()] = wi12; x[a13->wi.get_idx()] = wi13; x[a32->wi.get_idx()] = wi32;
//                x[_net->get_node(n1->_name)->w.get_idx()] = w1;
//                x[_net->get_node(n2->_name)->w.get_idx()] = w2;
//                x[_net->get_node(n3->_name)->w.get_idx()] = w3;

//                cout << "\nSDPf(x*) = " << SDPf.eval(x);

//                Constraint SDP("SDP");
//                SDP += SDPf.outer_approx(x);
//                SDP >= 0;
//                SDP.print();
//                _model->addConstraint(SDP);

//                cout << "\nValue of SDPf at x^ = " << SDPf.eval(x);
//                cout << ", SOCPs: " << wr13*wr13 + wi13*wi13 - w1*w3 << ", " << wr12*wr12 + wi12*wi12 - w1*w2 << ", " << wr32*wr32 + wi32*wi32 - w2*w3;

//                x[a12->wr.get_idx()] = a12->wr.get_value(); x[a13->wr.get_idx()] = a13->wr.get_value(); x[a32->wr.get_idx()] = a32->wr.get_value();
//                x[a12->wi.get_idx()] = a12->wi.get_value(); x[a13->wi.get_idx()] = a13->wi.get_value(); x[a32->wi.get_idx()] = a32->wi.get_value();
//                x[_net->get_node(n1->_name)->w.get_idx()] = _net->get_node(n1->_name)->w.get_value();
//                x[_net->get_node(n1->_name)->w.get_idx()] = _net->get_node(n2->_name)->w.get_value();
//                x[_net->get_node(n1->_name)->w.get_idx()] = _net->get_node(n3->_name)->w.get_value();

//                cout << "\nValue of the cut at x* = " << SDP.eval(x);

//                SDP.print();
//                delete[] x;
                id++;
            }
        }
    }
    cout << "\nNumber of added cuts = " << id;
    return id;
}

void PowerModel::add_SDP_OA_deepest_cut(){
    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    int id = 0;
    double tol = 0.001;
    double wr12, wr13, wr32, wi12, wi13, wi32, w1, w2, w3;
    for (auto b: *_net->_bagsnew){
//        cout << "\n\nBag number = " << b->_id << ": ";
        if(b->size()==3) {
            if (!SDP_satisfied_new1(b)) {
//                cout << "\nrot = " << b->_rot;
                double* x = new double[_model->get_nb_vars()];
                double* x1 = new double[9];
                add_bag_SOCP(b, id);
                n1 = _net->get_node(b->_n1->_name);
                n2 = _net->get_node(b->_n2->_name);
                n3 = _net->get_node(b->_n3->_name);
                cout << "\nn1 = " << n1->_name << ", n2 = " << n2->_name << ", n3 = " << n3->_name;
                a12 = _net->get_arc(n1, n2);
                a13 = _net->get_arc(n1, n3);
                a32 = _net->get_arc(n3, n2);

                /* Get the values from the solution of SOCP */

                wr12 = a12->wr.get_value(); wr13 = a13->wr.get_value(); wr32 = a32->wr.get_value();
                wi12 = a12->wi.get_value(); wi13 = a13->wi.get_value(); wi32 = a32->wi.get_value();
                w1 = n1->w.get_value(); w2 = n2->w.get_value(); w3 = n3->w.get_value();

                cout << "\nSolving the deepest cut problem for x*: \nwr12 = " << wr12 << ", wr13 = " << wr13 << ", wr32 = " << wr32;
                cout << ",\nwi12 = " << wi12 << ", wi13 = " << wi13 << ", wi32 = " << wi32;
                cout << ",\nw1 = " << w1 << ", w2 = " << w2 << ", w3 = " << w3;

//                wr12 = 1.20978; wr13 = 1.20212; wr32 = 1.20224;
//                wi12 = -0.0232485; wi13 = 0.0731976; wi32 = -0.0712145;
//                w1 = 1.21; w2 = 1.21; w3 = 1.19871;

                /* Create a model for finding the deepest cut */

                Model *m1;
                m1 = new Model();

                var<> wr12v, wr32v, wr13v, wi12v, wi32v, wi13v, w1v, w2v, w3v;
                wr12v.init("wr12v",a12->wr.get_lb(),a12->wr.get_ub());
                wr32v.init("wr32v",a32->wr.get_lb(),a32->wr.get_ub());
                wr13v.init("wr13v",a13->wr.get_lb(),a13->wr.get_ub());
                wi12v.init("wi12v",a12->wi.get_lb(),a12->wi.get_ub());
                wi32v.init("wi32v",a32->wi.get_lb(),a32->wi.get_ub());
                wi13v.init("wi13v",a13->wi.get_lb(),a13->wi.get_ub());
                w1v.init("w1v",n1->w.get_lb(),n1->w.get_ub());
                w2v.init("w2v",n2->w.get_lb(),n2->w.get_ub());
                w3v.init("w3v",n3->w.get_lb(),n3->w.get_ub());
//                wr12v.init("wr12v",0.701481,1.21);
//                wr32v.init("wr32v",0.701481,1.21);
//                wr13v.init("wr13v",0.701481,1.21);
//                wi12v.init("wi12v",-0.605,0.605);
//                wi32v.init("wi32v",-0.605,0.605);
//                wi13v.init("wi13v",-0.605,0.605);
//                w1v.init("w1v",0.81,1.21);
//                w2v.init("w2v",0.81,1.21);
//                w3v.init("w3v",0.81,1.21);
                wr12v = wr12; wr32v = wr32; wr13v = wr13;
                wi12v = wi12; wi32v = wi32; wi13v = wi13;
                w1v = w1;     w2v = w2;     w3v = w3;
//                wr12v = 1; wr32v = 1; wr13v = 1;
//                wi12v = 0; wi32v = 0; wi13v = 0;
//                w1v = 1;   w2v = 1;   w3v = 1;
                m1->addVar(wr12v); m1->addVar(wr32v); m1->addVar(wr13v);
                m1->addVar(wi12v); m1->addVar(wi32v); m1->addVar(wi13v);
                m1->addVar(w1v);   m1->addVar(w2v);   m1->addVar(w3v);


                Function obj_f;
                if (b->_rot) {
                    obj_f = wr12v*(wr32v*wr13v - wi32v*wi13v) - wi12v*(wi32v*wr13v + wr32v*wi13v);
                }
                else {
                    obj_f = wr12v*(wr32v*wr13v - wi32v*wi13v) + wi12v*(wi32v*wr13v + wr32v*wi13v);
                }
                obj_f *= 2;
                obj_f -= (((wr12v)^2)+((wi12v)^2))*w3v;
                obj_f -= (((wr13v)^2)+((wi13v)^2))*w2v;
                obj_f -= (((wr32v)^2)+((wi32v)^2))*w1v;
                obj_f += w1v*w2v*w3v;

                x1[wr12v.get_idx()] = wr12; x1[wr13v.get_idx()] = wr13; x1[wr32v.get_idx()] = wr32;
                x1[wi12v.get_idx()] = wi12; x1[wi13v.get_idx()] = wi13; x1[wi32v.get_idx()] = wi32;
                x1[w1v.get_idx()] = w1;     x1[w2v.get_idx()] = w2;     x1[w3v.get_idx()] = w3;

                Function* f = new Function();
                *f += obj_f.tang_param(x1);
                cout << "\nObj:"; f->print(false); cout << "\n";
                m1->setObjectiveType(minimize);
                m1->setObjective(f);

//                x1[wr12v.get_idx()] = 1; x1[wr13v.get_idx()] = 1; x1[wr32v.get_idx()] = 1;
//                x1[wi12v.get_idx()] = 0; x1[wi13v.get_idx()] = 0; x1[wi32v.get_idx()] = 0;
//                x1[w1v.get_idx()] = 1;   x1[w2v.get_idx()] = 1;   x1[w3v.get_idx()] = 1;

                meta_var* wr12m = new meta_var("wr12", m1);
                meta_var* wr13m = new meta_var("wr13", m1);
                meta_var* wr32m = new meta_var("wr32", m1);
                meta_var* wi12m = new meta_var("wi12", m1);
                meta_var* wi13m = new meta_var("wi13", m1);
                meta_var* wi32m = new meta_var("wi32", m1);
                meta_var* w1m = new meta_var("w1", m1);
                meta_var* w2m = new meta_var("w2", m1);
                meta_var* w3m = new meta_var("w3", m1);
                meta_Constraint* SDPm = new meta_Constraint();
                if(!b->_rot) {
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm += (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }else{
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm -= (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }
                *SDPm *= 2;
                *SDPm -= (((*wr12m)^2)+((*wi12m)^2))*(*w3m);
                *SDPm -= (((*wr13m)^2)+((*wi13m)^2))*(*w2m);
                *SDPm -= (((*wr32m)^2)+((*wi32m)^2))*(*w1m);
                *SDPm += (*w1m)*(*w2m)*(*w3m);
                *SDPm <= 0;
                m1->addMetaConstraint(*SDPm);

                m1->init_functions(1);
                m1->print_functions();

                *wr12m = wr12v; *wr32m = wr32v; *wr13m = wr13v;
                *wi12m = wi12v; *wi32m = wi32v; *wi13m = wi13v;
                *w1m = w1v; *w2m = w2v; *w3m = w3v;
                m1->concretise(*SDPm, 0, "SDP");

                Constraint SOCP12("SOCP12");
                SOCP12 = wr12v*wr12v + wi12v*wi12v - w1v*w2v;
                SOCP12 <= 0;
                m1->addConstraint(SOCP12);

                Constraint SOCP32("SOCP32");
                SOCP32 = wr32v*wr32v + wi32v*wi32v - w3v*w2v;
                SOCP32 <= 0;
                m1->addConstraint(SOCP32);

                Constraint SOCP13("SOCP13");
                SOCP13 = wr13v*wr13v + wi13v*wi13v - w1v*w3v;
                SOCP13 <= 0;
                m1->addConstraint(SOCP13);

                PTSolver* s1 = new PTSolver(m1,ipopt);
                s1->run(0,false);

                /* Get values from the solution of the deepest cut problem */

                x[a12->wr.get_idx()] = wr12v.get_value(); x[a13->wr.get_idx()] = wr13v.get_value(); x[a32->wr.get_idx()] = wr32v.get_value();
                x[a12->wi.get_idx()] = wi12v.get_value(); x[a13->wi.get_idx()] = wi13v.get_value(); x[a32->wi.get_idx()] = wi32v.get_value();
                x[n1->w.get_idx()] = w1v.get_value(); x[n2->w.get_idx()] = w2v.get_value(); x[n3->w.get_idx()] = w3v.get_value();

                delete s1;
                delete m1;

                cout << "\nGenerating cuts for x^: \nwr12 = " << wr12v.get_value() << ", wr13 = " << wr13v.get_value() << ", wr32 = " << wr32v.get_value();
                cout << ",\nwi12 = " << wi12v.get_value() << ", wi13 = " << wi13v.get_value() << ", wi32 = " << wi32v.get_value();
                cout << ",\nw1 = " << w1v.get_value() << ", w2 = " << w2v.get_value() << ", w3 = " << w3v.get_value();

                /* Add the linearised cut */

                Function SDPf;
                if (b->_rot) {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) - a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                else {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) + a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                SDPf *= 2;
                SDPf -= (((a12->wr)^2)+((a12->wi)^2))*(_net->get_node(n3->_name)->w);
                SDPf -= (((a13->wr)^2)+((a13->wi)^2))*(_net->get_node(n2->_name)->w);
                SDPf -= (((a32->wr)^2)+((a32->wi)^2))*(_net->get_node(n1->_name)->w);
                SDPf += (n1->w)*(n2->w)*(n3->w);

                cout << "\nSDPf: ";
                SDPf.print(false);
                cout << "\nSDP constraint at x^ = " << SDPf.eval(x);

                Constraint SDP("SDP");
                SDP += SDPf.outer_approx(x);
                SDP >= 0;
                SDP.print();
                _model->addConstraint(SDP);
                id++;

                x[a12->wr.get_idx()] = wr12; x[a13->wr.get_idx()] = wr13; x[a32->wr.get_idx()] = wr32;
                x[a12->wi.get_idx()] = wi12; x[a13->wi.get_idx()] = wi13; x[a32->wi.get_idx()] = wi32;
                x[n1->w.get_idx()] = w1;     x[n2->w.get_idx()] = w2;     x[n3->w.get_idx()] = w3;

                cout << "\nSDP cut at x* = " << SDP.eval(x);

                SDP.print();
                delete[] x;
            }
        }
    }
    cout << "\nNumber of added cuts = " << id;
}

void PowerModel::add_SDP_OA_cp_and_deepcut(){
    Node *n1, *n2, *n3;
    Arc *a12, *a13, *a32;
    int id = 0;
    double tol = 0.001;
    double wr12, wr13, wr32, wi12, wi13, wi32, w1, w2, w3;
    for (auto b: *_net->_bagsnew){
//        cout << "\n\nBag number = " << b->_id << ": ";
        if(b->size()==3) {
            if (!SDP_satisfied_new1(b)) {
//                cout << "\nrot = " << b->_rot;
                double* x = new double[_model->get_nb_vars()];
                double* x1 = new double[9];
                add_bag_SOCP(b, id);
                n1 = _net->get_node(b->_n1->_name);
                n2 = _net->get_node(b->_n2->_name);
                n3 = _net->get_node(b->_n3->_name);
                cout << "\nn1 = " << n1->_name << ", n2 = " << n2->_name << ", n3 = " << n3->_name;
                a12 = _net->get_arc(n1, n2);
                a13 = _net->get_arc(n1, n3);
                a32 = _net->get_arc(n3, n2);

                /* Get the values from the solution of SOCP */

                wr12 = a12->wr.get_value(); wr13 = a13->wr.get_value(); wr32 = a32->wr.get_value();
                wi12 = a12->wi.get_value(); wi13 = a13->wi.get_value(); wi32 = a32->wi.get_value();
                w1 = n1->w.get_value(); w2 = n2->w.get_value(); w3 = n3->w.get_value();

                /* Find the closest point */

                Model *m_cp;
                m_cp = new Model();

                var<> wr12_cp, wr32_cp, wr13_cp, wi12_cp, wi32_cp, wi13_cp, w1_cp, w2_cp, w3_cp;
                wr12_cp.init("wr12v",a12->wr.get_lb(),a12->wr.get_ub());
                wr32_cp.init("wr32v",a32->wr.get_lb(),a32->wr.get_ub());
                wr13_cp.init("wr13v",a13->wr.get_lb(),a13->wr.get_ub());
                wi12_cp.init("wi12v",a12->wi.get_lb(),a12->wi.get_ub());
                wi32_cp.init("wi32v",a32->wi.get_lb(),a32->wi.get_ub());
                wi13_cp.init("wi13v",a13->wi.get_lb(),a13->wi.get_ub());
                w1_cp.init("w1v",_net->get_node(n1->_name)->w.get_lb(),_net->get_node(n1->_name)->w.get_ub());
                w2_cp.init("w2v",_net->get_node(n2->_name)->w.get_lb(),_net->get_node(n2->_name)->w.get_ub());
                w3_cp.init("w3v",_net->get_node(n3->_name)->w.get_lb(),_net->get_node(n3->_name)->w.get_ub());
                wr12_cp = wr12; wr32_cp = wr32; wr13_cp = wr13;
                wi12_cp = wi12; wi32_cp = wi32; wi13_cp = wi13;
                w1_cp = w1; w2_cp = w2; w3_cp = w3;
                m_cp->addVar(wr12_cp); m_cp->addVar(wr32_cp); m_cp->addVar(wr13_cp);
                m_cp->addVar(wi12_cp); m_cp->addVar(wi32_cp); m_cp->addVar(wi13_cp);
                m_cp->addVar(w1_cp); m_cp->addVar(w2_cp); m_cp->addVar(w3_cp);

                Function* dist = new Function();
                *dist += (wr12-wr12_cp)*(wr12-wr12_cp) + (wr32-wr32_cp)*(wr32-wr32_cp) + (wr13-wr13_cp)*(wr13-wr13_cp);
                *dist += (wi12-wi12_cp)*(wi12-wi12_cp) + (wi32-wi32_cp)*(wi32-wi32_cp) + (wi13-wi13_cp)*(wi13-wi13_cp);
                *dist += (w1-w1_cp)*(w1-w1_cp) + (w2-w2_cp)*(w2-w2_cp) + (w3-w3_cp)*(w3-w3_cp);
                m_cp->setObjectiveType(minimize);
                m_cp->setObjective(dist);

                meta_var* wr12_cpm = new meta_var("wr12", m_cp);
                meta_var* wr13_cpm = new meta_var("wr13", m_cp);
                meta_var* wr32_cpm = new meta_var("wr32", m_cp);
                meta_var* wi12_cpm = new meta_var("wi12", m_cp);
                meta_var* wi13_cpm = new meta_var("wi13", m_cp);
                meta_var* wi32_cpm = new meta_var("wi32", m_cp);
                meta_var* w1_cpm = new meta_var("w1", m_cp);
                meta_var* w2_cpm = new meta_var("w2", m_cp);
                meta_var* w3_cpm = new meta_var("w3", m_cp);
                meta_Constraint* SDP_cpm = new meta_Constraint();
                if(!b->_rot) {
                    *SDP_cpm = (*wr12_cpm)*((*wr32_cpm)*(*wr13_cpm) - (*wi32_cpm)*(*wi13_cpm));
                    *SDP_cpm += (*wi12_cpm)*((*wi32_cpm)*(*wr13_cpm) + (*wr32_cpm)*(*wi13_cpm));
                }else{
                    *SDP_cpm = (*wr12_cpm)*((*wr32_cpm)*(*wr13_cpm) - (*wi32_cpm)*(*wi13_cpm));
                    *SDP_cpm -= (*wi12_cpm)*((*wi32_cpm)*(*wr13_cpm) + (*wr32_cpm)*(*wi13_cpm));
                }
                *SDP_cpm *= 2;
                *SDP_cpm -= (((*wr12_cpm)^2)+((*wi12_cpm)^2))*(*w3_cpm);
                *SDP_cpm -= (((*wr13_cpm)^2)+((*wi13_cpm)^2))*(*w2_cpm);
                *SDP_cpm -= (((*wr32_cpm)^2)+((*wi32_cpm)^2))*(*w1_cpm);
                *SDP_cpm += (*w1_cpm)*(*w2_cpm)*(*w3_cpm);
                *SDP_cpm >= 0;
                m_cp->addMetaConstraint(*SDP_cpm);

                m_cp->init_functions(1);
                m_cp->print_functions();

                *wr12_cpm = wr12_cp; *wr32_cpm = wr32_cp; *wr13_cpm = wr13_cp;
                *wi12_cpm = wi12_cp; *wi32_cpm = wi32_cp; *wi13_cpm = wi13_cp;
                *w1_cpm = w1_cp; *w2_cpm = w2_cp; *w3_cpm = w3_cp;
                m_cp->concretise(*SDP_cpm, 0, "SDP");

                Constraint SOCP12_cp("SOCP12");
                SOCP12_cp = wr12_cp*wr12_cp + wi12_cp*wi12_cp - w1_cp*w2_cp;
                SOCP12_cp <= 0;
                m_cp->addConstraint(SOCP12_cp);

                Constraint SOCP32_cp("SOCP32");
                SOCP32_cp = wr32_cp*wr32_cp + wi32_cp*wi32_cp - w3_cp*w2_cp;
                SOCP32_cp <= 0;
                m_cp->addConstraint(SOCP32_cp);

                Constraint SOCP13_cp("SOCP13");
                SOCP13_cp = wr13_cp*wr13_cp + wi13_cp*wi13_cp - w1_cp*w3_cp;
                SOCP13_cp <= 0;
                m_cp->addConstraint(SOCP13_cp);

                PTSolver* s1_cp = new PTSolver(m_cp,ipopt);
                s1_cp->run(0,false);

                wr12 = wr12_cp.get_value(); wr13 = wr13_cp.get_value(); wr32 = wr32_cp.get_value();
                wi12 = wi12_cp.get_value(); wi13 = wi13_cp.get_value(); wi32 = wi32_cp.get_value();
                w1 = w1_cp.get_value(); w2 = w2_cp.get_value(); w3 = w3_cp.get_value();

                delete s1_cp;
                delete m_cp;

                cout << "\nSolving the deepest cut problem for x*: \nwr12 = " << wr12 << ", wr13 = " << wr13 << ", wr32 = " << wr32;
                cout << ",\nwi12 = " << wi12 << ", wi13 = " << wi13 << ", wi32 = " << wi32;
                cout << ",\nw1 = " << w1 << ", w2 = " << w2 << ", w3 = " << w3;

//                wr12 = 1.20978; wr13 = 1.20212; wr32 = 1.20224;
//                wi12 = -0.0232485; wi13 = 0.0731976; wi32 = -0.0712145;
//                w1 = 1.21; w2 = 1.21; w3 = 1.19871;

                /* Create a model for finding the deepest cut */

                Model *m1;
                m1 = new Model();

                var<> wr12v, wr32v, wr13v, wi12v, wi32v, wi13v, w1v, w2v, w3v;
                wr12v.init("wr12v",a12->wr.get_lb(),a12->wr.get_ub());
                wr32v.init("wr32v",a32->wr.get_lb(),a32->wr.get_ub());
                wr13v.init("wr13v",a13->wr.get_lb(),a13->wr.get_ub());
                wi12v.init("wi12v",a12->wi.get_lb(),a12->wi.get_ub());
                wi32v.init("wi32v",a32->wi.get_lb(),a32->wi.get_ub());
                wi13v.init("wi13v",a13->wi.get_lb(),a13->wi.get_ub());
                w1v.init("w1v",n1->w.get_lb(),n1->w.get_ub());
                w2v.init("w2v",n2->w.get_lb(),n2->w.get_ub());
                w3v.init("w3v",n3->w.get_lb(),n3->w.get_ub());
//                wr12v.init("wr12v",0.701481,1.21);
//                wr32v.init("wr32v",0.701481,1.21);
//                wr13v.init("wr13v",0.701481,1.21);
//                wi12v.init("wi12v",-0.605,0.605);
//                wi32v.init("wi32v",-0.605,0.605);
//                wi13v.init("wi13v",-0.605,0.605);
//                w1v.init("w1v",0.81,1.21);
//                w2v.init("w2v",0.81,1.21);
//                w3v.init("w3v",0.81,1.21);
//                wr12v = wr12; wr32v = wr32; wr13v = wr13;
//                wi12v = wi12; wi32v = wi32; wi13v = wi13;
//                w1v = w1;     w2v = w2;     w3v = w3;
                wr12v = 1; wr32v = 1; wr13v = 1;
                wi12v = 0; wi32v = 0; wi13v = 0;
                w1v = 1;   w2v = 1;   w3v = 1;
                m1->addVar(wr12v); m1->addVar(wr32v); m1->addVar(wr13v);
                m1->addVar(wi12v); m1->addVar(wi32v); m1->addVar(wi13v);
                m1->addVar(w1v);   m1->addVar(w2v);   m1->addVar(w3v);


                Function obj_f;
                if (b->_rot) {
                    obj_f = wr12v*(wr32v*wr13v - wi32v*wi13v) - wi12v*(wi32v*wr13v + wr32v*wi13v);
                }
                else {
                    obj_f = wr12v*(wr32v*wr13v - wi32v*wi13v) + wi12v*(wi32v*wr13v + wr32v*wi13v);
                }
                obj_f *= 2;
                obj_f -= (((wr12v)^2)+((wi12v)^2))*w3v;
                obj_f -= (((wr13v)^2)+((wi13v)^2))*w2v;
                obj_f -= (((wr32v)^2)+((wi32v)^2))*w1v;
                obj_f += w1v*w2v*w3v;

                x1[wr12v.get_idx()] = wr12; x1[wr13v.get_idx()] = wr13; x1[wr32v.get_idx()] = wr32;
                x1[wi12v.get_idx()] = wi12; x1[wi13v.get_idx()] = wi13; x1[wi32v.get_idx()] = wi32;
                x1[w1v.get_idx()] = w1;     x1[w2v.get_idx()] = w2;     x1[w3v.get_idx()] = w3;

                Function* f = new Function();
                *f += obj_f.tang_param(x1);
                cout << "\nObj:"; f->print(false); cout << "\n";
                m1->setObjectiveType(minimize);
                m1->setObjective(f);

//                x1[wr12v.get_idx()] = 1; x1[wr13v.get_idx()] = 1; x1[wr32v.get_idx()] = 1;
//                x1[wi12v.get_idx()] = 0; x1[wi13v.get_idx()] = 0; x1[wi32v.get_idx()] = 0;
//                x1[w1v.get_idx()] = 1;   x1[w2v.get_idx()] = 1;   x1[w3v.get_idx()] = 1;

                meta_var* wr12m = new meta_var("wr12", m1);
                meta_var* wr13m = new meta_var("wr13", m1);
                meta_var* wr32m = new meta_var("wr32", m1);
                meta_var* wi12m = new meta_var("wi12", m1);
                meta_var* wi13m = new meta_var("wi13", m1);
                meta_var* wi32m = new meta_var("wi32", m1);
                meta_var* w1m = new meta_var("w1", m1);
                meta_var* w2m = new meta_var("w2", m1);
                meta_var* w3m = new meta_var("w3", m1);
                meta_Constraint* SDPm = new meta_Constraint();
                if(!b->_rot) {
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm += (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }else{
                    *SDPm = (*wr12m)*((*wr32m)*(*wr13m) - (*wi32m)*(*wi13m));
                    *SDPm -= (*wi12m)*((*wi32m)*(*wr13m) + (*wr32m)*(*wi13m));
                }
                *SDPm *= 2;
                *SDPm -= (((*wr12m)^2)+((*wi12m)^2))*(*w3m);
                *SDPm -= (((*wr13m)^2)+((*wi13m)^2))*(*w2m);
                *SDPm -= (((*wr32m)^2)+((*wi32m)^2))*(*w1m);
                *SDPm += (*w1m)*(*w2m)*(*w3m);
                *SDPm <= 0;
                m1->addMetaConstraint(*SDPm);

                m1->init_functions(1);
                m1->print_functions();

                *wr12m = wr12v; *wr32m = wr32v; *wr13m = wr13v;
                *wi12m = wi12v; *wi32m = wi32v; *wi13m = wi13v;
                *w1m = w1v; *w2m = w2v; *w3m = w3v;
                m1->concretise(*SDPm, 0, "SDP");

                Constraint SOCP12("SOCP12");
                SOCP12 = wr12v*wr12v + wi12v*wi12v - w1v*w2v;
                SOCP12 <= 0;
                m1->addConstraint(SOCP12);

                Constraint SOCP32("SOCP32");
                SOCP32 = wr32v*wr32v + wi32v*wi32v - w3v*w2v;
                SOCP32 <= 0;
                m1->addConstraint(SOCP32);

                Constraint SOCP13("SOCP13");
                SOCP13 = wr13v*wr13v + wi13v*wi13v - w1v*w3v;
                SOCP13 <= 0;
                m1->addConstraint(SOCP13);

                PTSolver* s1 = new PTSolver(m1,ipopt);
                s1->run(0,false);

                /* Get values from the solution of the deepest cut problem */

                x[a12->wr.get_idx()] = wr12v.get_value(); x[a13->wr.get_idx()] = wr13v.get_value(); x[a32->wr.get_idx()] = wr32v.get_value();
                x[a12->wi.get_idx()] = wi12v.get_value(); x[a13->wi.get_idx()] = wi13v.get_value(); x[a32->wi.get_idx()] = wi32v.get_value();
                x[n1->w.get_idx()] = w1v.get_value(); x[n2->w.get_idx()] = w2v.get_value(); x[n3->w.get_idx()] = w3v.get_value();

                delete s1;
                delete m1;

                cout << "\nGenerating cuts for x^: \nwr12 = " << wr12v.get_value() << ", wr13 = " << wr13v.get_value() << ", wr32 = " << wr32v.get_value();
                cout << ",\nwi12 = " << wi12v.get_value() << ", wi13 = " << wi13v.get_value() << ", wi32 = " << wi32v.get_value();
                cout << ",\nw1 = " << w1v.get_value() << ", w2 = " << w2v.get_value() << ", w3 = " << w3v.get_value();

                /* Add the linearised cut */

                Function SDPf;
                if (b->_rot) {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) - a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                else {
                    SDPf = a12->wr*(a32->wr*a13->wr - a32->wi*a13->wi) + a12->wi*(a32->wi*a13->wr + a32->wr*a13->wi);
                }
                SDPf *= 2;
                SDPf -= (((a12->wr)^2)+((a12->wi)^2))*(_net->get_node(n3->_name)->w);
                SDPf -= (((a13->wr)^2)+((a13->wi)^2))*(_net->get_node(n2->_name)->w);
                SDPf -= (((a32->wr)^2)+((a32->wi)^2))*(_net->get_node(n1->_name)->w);
                SDPf += (n1->w)*(n2->w)*(n3->w);

                cout << "\nSDPf: ";
                SDPf.print(false);
                cout << "\nSDP constraint at x^ = " << SDPf.eval(x);

                Constraint SDP("SDP");
                SDP += SDPf.outer_approx(x);
                SDP >= 0;
                SDP.print();
                _model->addConstraint(SDP);
                id++;

                x[a12->wr.get_idx()] = wr12; x[a13->wr.get_idx()] = wr13; x[a32->wr.get_idx()] = wr32;
                x[a12->wi.get_idx()] = wi12; x[a13->wi.get_idx()] = wi13; x[a32->wi.get_idx()] = wi32;
                x[n1->w.get_idx()] = w1;     x[n2->w.get_idx()] = w2;     x[n3->w.get_idx()] = w3;

                cout << "\nSDP cut at x* = " << SDP.eval(x);

//                SDP.print();
                delete[] x;
            }
        }
    }
    cout << "\nNumber of added cuts = " << id;
}

int PowerModel::add_SDP_cuts(int dim){
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
    
    meta_Constraint* SDPconstr = new meta_Constraint();
    *SDPconstr = (*wr12)*((*wr32)*(*wr13) - (*wi32)*(*wi13));
    *SDPconstr += (*wi12)*((*wi32)*(*wr13) + (*wr32)*(*wi13));
    *SDPconstr *= 2;
    *SDPconstr -= (((*wr12)^2)+((*wi12)^2))*(*w3);
    *SDPconstr -= (((*wr13)^2)+((*wi13)^2))*(*w2);
    *SDPconstr -= (((*wr32)^2)+((*wi32)^2))*(*w1);
    *SDPconstr += (*w1)*(*w2)*(*w3);
    *SDPconstr >= 0;
    SDPconstr->is_cut = true;
    _model->addMetaConstraint(*SDPconstr);
    
    meta_Constraint* SDPrconstr = new meta_Constraint();// Rotational SDP
    *SDPrconstr = (*wr12)*((*wr23)*(*wr31) - (*wi23)*(*wi31));
    *SDPrconstr -= (*wi12)*((*wi23)*(*wr31) + (*wr23)*(*wi31));
    *SDPrconstr *= 2;
    *SDPrconstr -= (((*wr12)^2)+((*wi12)^2))*(*w3);
    *SDPrconstr -= (((*wr31)^2)+((*wi31)^2))*(*w2);
    *SDPrconstr -= (((*wr23)^2)+((*wi23)^2))*(*w1);
    *SDPrconstr += (*w1)*(*w2)*(*w3);
    *SDPrconstr >= 0;
    SDPrconstr->is_cut = true;
    _model->addMetaConstraint(*SDPrconstr);
    
    Node* n1 = nullptr;
    Node* n2 = nullptr;
    Node* n3 = nullptr;
    Arc* a12 = nullptr;
    Arc* a13 = nullptr;
    Arc* a32 = nullptr;

    _model->init_functions(_net->_bags->size());
    _model->print_functions();
    
    int id = 0;
    int id1 = 0;
    int n_i1, n_i2, n_i3 = 0;
    bool rot_bag;
    for (auto b: *_net->_bags){
        //cout << "\nid = " << id1;
        id1++;
        //cout << "\nSize of bag = " << b->size();
        if (b->size()==dim) {
            rot_bag = _net->rotation_bag(b);
            if (rot_bag) {
                                cout << "rot_bag: ";
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
                cout << "not rot_bag: ";
                n_i1 = get_order(b,1);
                n1 = b->at(n_i1);
                n_i2 = get_order(b,2);
                n2 = b->at(n_i2);
                n_i3 = get_order(b,3);
                n3 = b->at(n_i3);
                a13 = _net->get_arc(n1,n3);
                a32 = _net->get_arc(n3,n2);
            }
            cout << "\nn1 = " << n1->_name << " n2 = " << n2->_name << " n3 = " << n3->_name;
            a12 = _net->get_arc(n1,n2);
            
            if(!a12 || a12->status==0) {
                //cout << "\nadding a12\n";
                if (!a12) {
                    a12 = _net->_clone->get_arc(n1,n2)->clone();
                    a12->status=1;
                    a12->src = _net->get_node(a12->src->_name);
                    a12->dest = _net->get_node(a12->dest->_name);
                    //a12->free = true;
                    a12->connect();
                    _net->add_arc(a12);
                }
                if (a12->tbound.min < 0 && a12->tbound.max > 0)
                    a12->cs.init("cs("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",min(cos(a12->tbound.min), cos(a12->tbound.max)), 1.);
                else
                    a12->cs.init("cs("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",min(cos(a12->tbound.min), cos(a12->tbound.max)), max(cos(a12->tbound.min),cos(a12->tbound.max)));
                a12->vv.init("vv("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->src->vbound.min*a12->dest->vbound.min,a12->src->vbound.max*a12->dest->vbound.max);
                if (_type==QC_SDP) {
                    a12->wr.init("wr("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*a12->cs.get_lb(), a12->vv.get_ub()*a12->cs.get_ub());
                    if(a12->tbound.min < 0 && a12->tbound.max > 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.min >= 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_lb()*sin(a12->tbound.min), a12->vv.get_ub()*sin(a12->tbound.max));
                    if (a12->tbound.max <= 0)
                        a12->wi.init("wi("+a12->_name+","+a12->src->_name+","+a12->dest->_name+")",a12->vv.get_ub()*sin(a12->tbound.min), a12->vv.get_lb()*sin(a12->tbound.max));
                    _model->addVar(a12->wr);
                    a12->wr = 1;
                    _model->addVar(a12->wi);
                }
                else {
                    a12->init_vars(SDP);
                    _model->addVar(a12->wr);
                    _model->addVar(a12->wi);
                }
                Constraint SOCP1("SOCP1");
                SOCP1 += _net->get_node(n1->_name)->w*_net->get_node(n2->_name)->w;
                if (_type==QC_SDP) {
                    SOCP1 -= ((a12->wr)^2);
                    SOCP1 -= ((a12->wi)^2);
                }
                else{
                    SOCP1 -= ((a12->wr)^2);
                    SOCP1 -= ((a12->wi)^2);
                }
                SOCP1 >= 0;
                SOCP1.is_cut = true;
                _model->addConstraint(SOCP1);
                //                _net->_clone->remove_arc(a12);
            }
            if(!a13 || a13->status==0) {
                //cout << "\nadding a13\n";
                if(!a13) {
                    if (rot_bag) {
                        a13 = _net->_clone->get_arc(n3,n1)->clone();
                    }
                    else {
                        a13 = _net->_clone->get_arc(n1,n3)->clone();
                    }
                    a13->status=1;
                    a13->src = _net->get_node(a13->src->_name);
                    a13->dest = _net->get_node(a13->dest->_name);
                    //a13->free = true;
                    a13->connect();
                    _net->add_arc(a13);
                }
                if (a13->tbound.min < 0 && a13->tbound.max > 0)
                    a13->cs.init("cs("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",min(cos(a13->tbound.min), cos(a13->tbound.max)), 1.);
                else
                    a13->cs.init("cs("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",min(cos(a13->tbound.min), cos(a13->tbound.max)), max(cos(a13->tbound.min),cos(a13->tbound.max)));
                a13->vv.init("vv("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->src->vbound.min*a13->dest->vbound.min,a13->src->vbound.max*a13->dest->vbound.max);
                
                if (_type==QC_SDP) {
                    a13->wr.init("wr("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*a13->cs.get_lb(), a13->vv.get_ub()*a13->cs.get_ub());
                    if(a13->tbound.min < 0 && a13->tbound.max > 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.min >= 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_lb()*sin(a13->tbound.min), a13->vv.get_ub()*sin(a13->tbound.max));
                    if (a13->tbound.max <= 0)
                        a13->wi.init("wi("+a13->_name+","+a13->src->_name+","+a13->dest->_name+")",a13->vv.get_ub()*sin(a13->tbound.min), a13->vv.get_lb()*sin(a13->tbound.max));
                    _model->addVar(a13->wr);
                    a13->wr = 1;
                    _model->addVar(a13->wi);
                }
                else {
                    a13->init_vars(SDP);
                    _model->addVar(a13->wr);
                    _model->addVar(a13->wi);
                }
                Constraint SOCP2("SOCP2");
                SOCP2 += _net->get_node(n1->_name)->w*_net->get_node(n3->_name)->w;
                if (_type==QC_SDP) {
                    SOCP2 -= ((a13->wr)^2);
                    SOCP2 -= ((a13->wi)^2);
                }
                else{
                    SOCP2 -= ((a13->wr)^2);
                    SOCP2 -= ((a13->wi)^2);
                }
                SOCP2 >= 0;
                SOCP2.is_cut = true;
                _model->addConstraint(SOCP2);
                //                _net->_clone->remove_arc(a13);
            }
            if(!a32 || a32->status==0) {
                //cout << "\nadding a32\n";
                if (!a32) {
                    if (rot_bag) {
                        a32 = _net->_clone->get_arc(n2,n3)->clone();
                    }
                    else {
                        a32 = _net->_clone->get_arc(n3,n2)->clone();
                    }
                    a32->status=1;
                    a32->src = _net->get_node(a32->src->_name);
                    a32->dest = _net->get_node(a32->dest->_name);
                    //a32->free = true;
                    a32->connect();
                    _net->add_arc(a32);
                }
                if (a32->tbound.min < 0 && a32->tbound.max > 0)
                    a32->cs.init("cs("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",min(cos(a32->tbound.min), cos(a32->tbound.max)), 1.);
                else
                    a32->cs.init("cs("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",min(cos(a32->tbound.min), cos(a32->tbound.max)), max(cos(a32->tbound.min),cos(a32->tbound.max)));
                a32->vv.init("vv("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->src->vbound.min*a32->dest->vbound.min,a32->src->vbound.max*a32->dest->vbound.max);
                
                if (_type==QC_SDP) {
                    a32->wr.init("wr("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*a32->cs.get_lb(), a32->vv.get_ub()*a32->cs.get_ub());
                    if(a32->tbound.min < 0 && a32->tbound.max > 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.min >= 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_lb()*sin(a32->tbound.min), a32->vv.get_ub()*sin(a32->tbound.max));
                    if (a32->tbound.max <= 0)
                        a32->wi.init("wi("+a32->_name+","+a32->src->_name+","+a32->dest->_name+")",a32->vv.get_ub()*sin(a32->tbound.min), a32->vv.get_lb()*sin(a32->tbound.max));
                    _model->addVar(a32->wr);
                    a32->wr = 1;
                    _model->addVar(a32->wi);
                }
                else {
                    a32->init_vars(SDP);
                    _model->addVar(a32->wr);
                    _model->addVar(a32->wi);
                }
                Constraint SOCP3("SOCP3");
                SOCP3 += _net->get_node(n3->_name)->w*_net->get_node(n2->_name)->w;
                if (_type==QC_SDP) {
                    SOCP3 -= ((a32->wr)^2);
                    SOCP3 -= ((a32->wi)^2);
                }
                else{
                    SOCP3 -= ((a32->wr)^2);
                    SOCP3 -= ((a32->wi)^2);
                }
                SOCP3 >= 0;
                SOCP3.is_cut = true;
                _model->addConstraint(SOCP3);
                //                _net->_clone->remove_arc(a32);
            }

            
            if (_type==QC_SDP) {
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
                _model->concretise(*SDPrconstr, id, "SDP"+to_string(id));
                id++;
            }
            else {
                _model->concretise(*SDPconstr, id, "SDP"+to_string(id));
                id++;
            }
        }
    }
    cout << "\ntotal number of 3d SDP cuts added = " << id << endl;
    return id;
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

void PowerModel::post_AC_Ref(){
    Node* src = NULL;
    Node* dest = NULL;

    for (auto n:_net->nodes)
        add_AC_KCL(n, false);

    for (auto a:_net->arcs) {

        src = a->src;
        dest = a->dest;

        Constraint PF1("PF1");
        PF1 += src->w - dest->w;
        PF1 -= 2*(a->r*a->pi + a->x*a->qi);
        PF1 += (a->r*a->r + a->x*a->x)*((a->pi + a->pj + a->qi + a->qj)/(a->r + a->x));
        PF1 = 0;
        _model->addConstraint(PF1);

/*        Constraint PF1ji("PF1ji");
        PF1ji += dest->w - src->w;
        PF1ji -= 2*(a->r*a->pj + a->x*a->qj);
        PF1ji += (a->r*a->r + a->x*a->x)*((a->pi + a->pj + a->qi + a->qj)/(a->r + a->x));
        PF1ji = 0;
        _model->addConstraint(PF1ji);*/

        Constraint PF2("PF2"); // this one is symmetrical
        PF2 +=a->r*(a->qi + a->qj) - a->x*(a->pi + a->pj);
        PF2 = 0;
        _model->addConstraint(PF2);

        Constraint PF3("PF3");
        PF3 += a->pi*src->w + a->pj*src->w + a->qi*src->w + a->qj*src->w;
        PF3 -= (a->r + a->x)*(a->pi*a->pi + a->qi*a->qi);
        PF3 = 0;
        _model->addConstraint(PF3);

/*        Constraint PF3ji("PF3ji");
        PF3ji += a->pi + a->pj + a->qi + a->qj;
        PF3ji -= (a->r + a->x)*(a->pj*a->pj + a->qj*a->qj)*(1/dest->w);
        PF3ji = 0;
        _model->addConstraint(PF3ji);*/

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
            if (a->status==1 && a->src->_type!=4 && a->dest->_type!=4) {
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


