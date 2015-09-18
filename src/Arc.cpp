//
//  Arc.cpp
//  Cycle_Basis_PF
//
//  Created by Sumiran on 18/06/2014.
//  Copyright (c) 2014 NICTA. All rights reserved.
//

#include "PowerTools++/Arc.h"
#include <iostream>

using namespace std;

/* Only horton path needs to be deleted, src and dest are deleted at the network level */
Arc::~Arc(){
    if (horton_path) {
        delete horton_path;
    }
}


Arc::Arc(string name):Line(name), src(NULL), dest(NULL), in_cycle(false), horton_path(NULL){
}


Arc::Arc(Node* s, Node* d){
    src = s;
    dest = d;
    in_cycle = false;
    connect();
}


Arc* Arc::clone(){
    Arc* copy = new Arc(to_string(id));
    copy->src = src;
    copy->dest = dest;
    return copy;
}


/* Connects the current arc to its source and destination, adding itself to the list of branches in these nodes */
void Arc::connect(){
    src->addArc(this);
    dest->addArc(this);
}

/* @brief Returns the neighbour of n if n is a node of the arc, null otherwise */
Node* Arc::neighbour(Node* n){
    Node* neigh = NULL;
    if (src == n)
        neigh = dest;
    if (dest == n)
        neigh = src;
    return neigh;
}

void Arc::print(){
    std::cout << "( " << src->ID << ", " << dest->ID << " ): ";
    std::cout << "limit = " << limit;
    std::cout << "; charge = " << ch;
    std::cout << "; as = " << as;
    std::cout << "; tr = " << tr;
    std::cout << "; cc = " << cc;
    std::cout << "; dd = " << dd;
    std::cout << "; r = " << r;
    std::cout << "; x = " << x << std::endl;
}