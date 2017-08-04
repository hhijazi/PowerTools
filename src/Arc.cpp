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


Arc::Arc(string name):Line(name), src(NULL), dest(NULL), in_cycle(false), parallel(false), horton_path(NULL){
}


Arc::Arc(Node* s, Node* d){
    src = s;
    dest = d;
    in_cycle = false;
    parallel = false;
    connect();
}


Arc* Arc::clone(){
    Arc* copy = new Arc(_name);
    copy->src = src;
    copy->dest = dest;
    copy->tbound.min = tbound.min;
    copy->tbound.max = tbound.max;
    copy->parallel = parallel;
    copy->in_cycle = in_cycle;
    copy->status = status;
    copy->free = free;
    copy->defining_bags = defining_bags;
    copy->added = added;
    copy->imaginary = imaginary;
    return copy;
}

void Arc::init_vars(PowerModelType t){
    switch(t){
        case SOCP:
        case SDP:
            if (tbound.min < -3.14 && tbound.max > 3.14)
                cs.init("cs("+_name+","+src->_name+","+dest->_name+")",-1., 1.);
            else if (tbound.min < 0 && tbound.max > 0)
                cs.init("cs("+_name+","+src->_name+","+dest->_name+")",min(cos(tbound.min), cos(tbound.max)), 1.);
            else
                cs.init("cs("+_name+","+src->_name+","+dest->_name+")",min(cos(tbound.min), cos(tbound.max)), max(cos(tbound.min),cos(tbound.max)));
            vv.init("vv("+_name+","+src->_name+","+dest->_name+")",src->vbound.min*dest->vbound.min,src->vbound.max*dest->vbound.max);
            wr.init("wr("+_name+","+src->_name+","+dest->_name+")",vv.get_lb()*cs.get_lb(), vv.get_ub()*cs.get_ub());

            if(tbound.min < -1.57 && tbound.max > 1.57)
                sn.init("sn("+_name+","+src->_name+","+dest->_name+")",-1., 1.);
            else
                sn.init("sn("+_name+","+src->_name+","+dest->_name+")",sin(tbound.min), sin(tbound.max));

            if(tbound.min < 0 && tbound.max > 0)
                wi.init("wi(" + _name + "," + src->_name + "," + dest->_name + ")", vv.get_ub() * sn.get_lb(), vv.get_ub() * sn.get_ub());
            if (tbound.min >= 0)
                wi.init("wi("+_name+","+src->_name+","+dest->_name+")",vv.get_lb()*sn.get_lb(), vv.get_ub()*sn.get_ub());
            if (tbound.max <= 0)
                wi.init("wi("+_name+","+src->_name+","+dest->_name+")",vv.get_ub()*sn.get_lb(), vv.get_lb()*sn.get_ub());
            wr = 1;
            break;
        default:
            break;
    }
}


/* Connects the current arc to its source and destination, adding itself to the list of branches in these nodes */
void Arc::connect(){
    src->update_fill_in(dest);
    dest->update_fill_in(src);
    Node* common = nullptr;
    if(!src->is_connected(dest)) {
        for (auto a:src->branches) {
            common = a->neighbour(src);
            if (common->is_connected(dest)) {
                common->fill_in--;
                assert(common->fill_in >= 0);
            }
        }
    }
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
    std::cout << "( " << src->_name << ", " << dest->_name << " ): ";
    std::cout << "limit = " << limit;
    std::cout << "; charge = " << ch;
    std::cout << "; as = " << as;
    std::cout << "; tr = " << tr;
    std::cout << "; cc = " << cc;
    std::cout << "; dd = " << dd;
    std::cout << "; r = " << r;
    std::cout << "; x = " << x << std::endl;
}