//
//  Arc.h
//  Cycle_Basis_PF
//
//  Created by Sumiran on 18/06/2014.
//  Copyright (c) 2014 NICTA. All rights reserved.
//

#ifndef Cycle_Basis_PF_Arc_h
#define Cycle_Basis_PF_Arc_h
#include <PowerTools++/Bag.h>
#include <PowerTools++/Node.h>
#include <PowerTools++/Path.h>
#include <PowerTools++/Line.h>
#include <PowerTools++/Type.h>

class Bag;

class Arc : public Line{
public:
    bool free = false;
    bool imaginary = false;
    bool added = false; // indicates if a SOCP constraint for this line has been added to the model
    Node* src;
    Node* dest;
    bool in_cycle;
    bool parallel;
    Path* horton_path;
    double weight;

    /** List of bags that are used to fix this line */
    vector<Bag*>* defining_bags = nullptr;
    
    /** @brief Phase angle difference bounds */
    Bound   tbound;
    
    /* @brief Returns the neighbour of n if n is a node of the arc, null otherwise */
    Node* neighbour(Node* n);
    
    Arc(std::string name);
    
    ~Arc();
    
    Arc(Node* s, Node* d);
    
    Arc* clone();

    void init_vars(PowerModelType t);
    
    /* Connects the current arc to its source and destination, adding itself to the list of branches in these nodes */
    void connect();
    
    void print();
};

#endif
