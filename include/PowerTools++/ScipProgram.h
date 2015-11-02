#ifndef SCIPPROGRAM_H
#define SCIPPROGRAM_H

#include "PowerTools++/Model.h"
#include "objscip/objscip.h"

class ScipProgram {
private:
    Model *model;
    SCIP* scip;
    SCIP_VAR** svars;

    SCIP_RETCODE create_nlin_expr(Function* f, vector<SCIP_VAR*>& varlist, SCIP_EXPR**);
public:
    ScipProgram();
    ~ScipProgram();
    void set_model(Model* m);

    SCIP_RETCODE solve();
    SCIP_RETCODE prepare_model();

    SCIP_RETCODE create_scip_vars();
    SCIP_RETCODE create_scip_constraints();
    SCIP_RETCODE set_scip_objective();
};

#endif // SCIPPROGRAM_H
