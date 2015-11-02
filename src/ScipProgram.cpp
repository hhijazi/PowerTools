#include "PowerTools++/ScipProgram.h"
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip_exception.hpp"
#include <vector>

ScipProgram::ScipProgram() {
    try {
        SCIP_CALL_EXC( SCIPcreate(&scip) );
        SCIPprintVersion(scip, NULL);
        SCIP_CALL_EXC( SCIPincludeDefaultPlugins(scip) );
    }catch(SCIPException & exec){
        cerr << exec.what() << endl;
        exit(exec.getRetcode());
    }
    model = NULL;
}

void ScipProgram::set_model(Model* m) {
    model = m;
    svars = new SCIP_VAR*[m->get_nb_vars()];
}

SCIP_RETCODE ScipProgram::solve(){
    SCIP_CALL( SCIPsolve(scip) );
}

SCIP_RETCODE ScipProgram::prepare_model() {
    if(SCIPfileExists("scipmip.set")) {
        SCIP_CALL( SCIPreadParams(scip, "scipmip.set") );
    }
    else cout << "\nParameter file scipmip.set not found - using default parameters";

    SCIP_CALL( SCIPcreateProbBasic(scip, "model") );
    SCIP_CALL( create_scip_vars() );

    SCIP_CALL( create_scip_constraints() );
    SCIP_CALL( set_scip_objective() );
    SCIP_CALL( SCIPprintOrigProblem(scip, NULL, "cip", FALSE) );

    for(int i = 0; i < model->get_nb_vars(); i++)
        SCIP_CALL( SCIPreleaseVar(scip, &svars[i]) );
    return SCIP_OKAY;
}

SCIP_RETCODE ScipProgram::create_scip_vars(){
    var<bool>* vb = NULL;
    var<double>* vr = NULL;
    var<int>* vi = NULL;
    double lb, ub;

    for (auto& it: model->_vars){
        switch (it->get_type()) {
            case binary:
                vb =  (var<bool>*)it;
                SCIP_CALL( SCIPcreateVarBasic(scip, &svars[it->get_idx()], vb->_name.c_str(), vb->get_lb(), vb->get_ub(), 0.0, SCIP_VARTYPE_CONTINUOUS) );
                SCIP_CALL( SCIPaddVar(scip, svars[it->get_idx()]) );
                break;
            case integ:
                vi =  (var<int>*)it;
                SCIP_CALL( SCIPcreateVarBasic(scip, &svars[it->get_idx()], vi->_name.c_str(), vi->get_lb(), vi->get_ub(), 0.0, SCIP_VARTYPE_INTEGER) );
                SCIP_CALL( SCIPaddVar(scip, svars[it->get_idx()]) );
                break;
            case real:
            case longreal:
                vr = (var<double>*)it;
                lb = min(vr->get_lb(), vr->get_lb_off());
                ub = max(vr->get_ub(), vr->get_ub_off());
                SCIP_CALL( SCIPcreateVarBasic(scip, &svars[it->get_idx()], vr->_name.c_str(), lb, ub, 0.0, SCIP_VARTYPE_CONTINUOUS) );
                SCIP_CALL( SCIPaddVar(scip, svars[it->get_idx()]) );
                break;
            default:
                cerr << "\nUnknown variable type.";
                break;
        }
    }

//    FILE* f;
//    f = fopen ("scipvars.txt" , "w");
//    for(int i = 0; i < model->get_nb_vars(); i++)
//        SCIPprintVar(scip, svars[i], f);

    return SCIP_OKAY;
}

SCIP_RETCODE ScipProgram::create_nlin_expr(Function* f, vector<SCIP_VAR*>& varlist, SCIP_EXPR** se) {
    const Quadratic* q;
    int nquadterms, idx, idx1, i = 0, nchildren;
    SCIP_EXPR** children;
    SCIP_Real *nonlincoefs;
    SCIP_EXPR *varexpr, *varexpr1, *result, *lexpr, *rexpr;

    q = f->get_quad();
    nquadterms = 0;
    for(auto& it: q->_qmatrix)
        nquadterms += (*(it.second)).size();
    nchildren = q->_coefs.size() + nquadterms;
    if(f->_ftype == nlin_) nchildren++;

    children = new SCIP_EXPR*[nchildren];
    nonlincoefs = new SCIP_Real[nchildren];


    for (auto &it1: q->_qmatrix) {
        idx = find(varlist.begin(), varlist.end(), svars[it1.first]) - varlist.begin();
        if(idx==varlist.size()) varlist.push_back(svars[it1.first]);
        SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &varexpr, SCIP_EXPR_VARIDX, idx) );
        for (auto &it2: *(it1.second)) {
            idx1 = find(varlist.begin(), varlist.end(), svars[it2.first]) - varlist.begin();
            if(idx1==varlist.size()) varlist.push_back(svars[it2.first]);
            SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &varexpr1, SCIP_EXPR_VARIDX, idx1) );
            SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[i], SCIP_EXPR_MUL, varexpr, varexpr1) );
            nonlincoefs[i] = it2.second;
            i++;
        }
    }

    for (auto& it1: q->_coefs) {
        idx = find(varlist.begin(), varlist.end(), svars[it1.first]) - varlist.begin();
        if(idx==varlist.size()) varlist.push_back(svars[it1.first]);
        SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[i], SCIP_EXPR_VARIDX, idx) );
        nonlincoefs[i] = it1.second;
        i++;
    }

    if (f->_ftype <= quad_) {
        SCIP_CALL( SCIPexprCreateLinear(SCIPblkmem(scip), se, nchildren, children, nonlincoefs, q->get_const()) );
        return SCIP_OKAY;
    }

    Function* lparent = f->_lparent.get();
    create_nlin_expr(lparent, varlist, &lexpr);

    if(!f->_rparent) {
        switch(f->get_otype()){
            case cos_:
                SCIP_CALL( SCIPexprMulConstant(SCIPblkmem(scip), &children[nchildren-1], lexpr, 1) );
                //SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_COS, lexpr) );
                cerr << "\nSCIP does not support trigonometric functions!";
                break;
            case sin_:
                SCIP_CALL( SCIPexprMulConstant(SCIPblkmem(scip), &children[nchildren-1], lexpr, 1) );
                //SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_SIN, lexpr) );
                cerr << "\nSCIP does not support trigonometric functions!";
                break;
            default:
                std::cerr << "unsupported operation";
                exit(-1);
        }
    }
    else{
        create_nlin_expr(f->_rparent.get(), varlist, &rexpr);
        switch(f->get_otype()){
            case plus_:
                SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_PLUS, lexpr, rexpr) );
                break;
            case minus_:
                SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_MINUS, lexpr, rexpr) );
                break;
            case product_:
                SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_MUL, lexpr, rexpr) );
                break;
            case div_:
                SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_DIV, lexpr, rexpr) );
                break;
            case power_:
                SCIP_CALL( SCIPexprCreate(SCIPblkmem(scip), &children[nchildren-1], SCIP_EXPR_REALPOWER, lexpr, rexpr) ); //1 operand!
                break;
            default:
                std::cerr << "unsupported operation";
                exit(-1);
        }
    }
    nonlincoefs[nchildren-1] = f->get_fcoeff();
    nonlincoefs[nchildren-1] = 1;
    SCIP_CALL( SCIPexprCreateLinear(SCIPblkmem(scip), se, nchildren, children, nonlincoefs, q->get_const()) );
    return SCIP_OKAY;
}

SCIP_RETCODE ScipProgram::create_scip_constraints(){
    SCIP_Real lhs, rhs;
    int i, nquadterms;
    SCIP_CONS* sconstr;
    const Quadratic* q;
    //FILE* fc;
    //fc = fopen ("scipconstrs.txt" , "w");
    SCIP_VAR** linvars, **quadvars1, **quadvars2;
    SCIP_Real* lincoefs, *quadcoefs;
    SCIP_EXPRTREE** exprtrees;
    SCIP_EXPR* nlinexpr;
    std::vector<SCIP_VAR*> varlist;

    for(auto& c: model->get_cons()){
        switch(c->get_type()) {
            case geq:
                lhs = c->get_rhs();
                rhs = SCIPinfinity(scip);
                break;
            case leq:
                lhs = -SCIPinfinity(scip);
                rhs = c->get_rhs();
                break;
            case eq:
                lhs = c->get_rhs();
                rhs = c->get_rhs();
                break;
            default:
                cerr << "Unknown constraint type.";
                break;
        }
        switch(c->get_ftype()) {
            case lin_:
                q = c->get_quad();
                linvars = new SCIP_VAR*[q->_coefs.size()];
                lincoefs = new SCIP_Real[q->_coefs.size()];
                i = 0;
                for (auto& it1: q->_coefs) {
                    linvars[i] = svars[it1.first];
                    lincoefs[i] = it1.second;
                    i++;
                }
                if (lhs != -SCIPinfinity(scip)) lhs -= q->get_const();
                if (rhs != SCIPinfinity(scip)) rhs -= q->get_const();
                SCIP_CALL( SCIPcreateConsBasicLinear(scip, &sconstr, c->_name.c_str(), q->_coefs.size(), linvars, lincoefs, lhs, rhs) );
                SCIP_CALL( SCIPaddCons(scip,sconstr) );
                SCIP_CALL( SCIPreleaseCons(scip,&sconstr) );
                delete[] linvars; delete[] lincoefs;
                break;
            case quad_:
                q = c->get_quad();
                linvars = new SCIP_VAR*[q->_coefs.size()];
                lincoefs = new SCIP_Real[q->_coefs.size()];
                i = 0;
                for (auto& it1: q->_coefs) {
                    linvars[i] = svars[it1.first];
                    lincoefs[i] = it1.second;
                    i++;
                }
                if (lhs != -SCIPinfinity(scip)) lhs -= q->get_const();
                if (rhs != SCIPinfinity(scip)) rhs -= q->get_const();
                nquadterms = 0;
                for(auto& it: q->_qmatrix) {
                    nquadterms += (*(it.second)).size();
                }
                quadvars1 = new SCIP_VAR*[nquadterms];
                quadvars2 = new SCIP_VAR*[nquadterms];
                quadcoefs = new SCIP_Real[nquadterms];

                i = 0;
                for (auto& it1: q->_qmatrix) {
                    for (auto& it2: *(it1.second)) {
                        quadvars1[i] = svars[it1.first];
                        quadvars2[i] = svars[it2.first];
                        quadcoefs[i] = it2.second;
                        i++;
                    }
                }
                SCIP_CALL( SCIPcreateConsBasicQuadratic(scip, &sconstr, c->_name.c_str(), q->_coefs.size(), linvars, lincoefs,
                                                        nquadterms, quadvars1, quadvars2, quadcoefs, lhs, rhs ) );
                SCIP_CALL( SCIPaddCons(scip,sconstr) );
                SCIP_CALL( SCIPreleaseCons(scip,&sconstr) );
                delete[] linvars; delete[] lincoefs;
                delete[] quadvars1; delete[] quadvars2;
                delete[] quadcoefs;
                break;
            case nlin_:
                exprtrees = new SCIP_EXPRTREE*[1];
                create_nlin_expr(c, varlist, &nlinexpr);
                SCIP_CALL( SCIPexprtreeCreate(SCIPblkmem(scip), &exprtrees[0], nlinexpr, varlist.size(), 0, NULL) );
                SCIP_CALL( SCIPexprtreeSetVars(exprtrees[0], varlist.size(), varlist.data()) ); // set the vars for the lin and quad exprtree
                SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &sconstr, c->_name.c_str(), 0, NULL, NULL,
                                                        1, exprtrees, NULL, lhs, rhs) );
                SCIP_CALL( SCIPaddCons(scip,sconstr) );
                SCIP_CALL( SCIPreleaseCons(scip,&sconstr) );
                SCIP_CALL( SCIPexprtreeFree(&exprtrees[0]) );
                delete[] exprtrees;
                varlist.clear();
                break;
            default:
                break;
        }
    }

    return SCIP_OKAY;
}

SCIP_RETCODE ScipProgram::set_scip_objective(){
    const Quadratic* q = model->_obj->get_quad();
    if (model->_obj->_ftype == lin_) {
        for (auto &it1: q->_coefs) {
            SCIP_CALL(SCIPchgVarObj(scip, svars[it1.first], it1.second));
        }
    }
    else{
        // nonlinear objective...
    }

    return SCIP_OKAY;
}

ScipProgram::~ScipProgram() {
    try{
        SCIP_CALL_EXC( SCIPfreeTransform(scip) );
        SCIP_CALL_EXC( SCIPfree(&scip) );
    }catch(SCIPException & exec){
        cerr << exec.what() << endl;
        exit(exec.getRetcode());
    }
}
