//
// Created by Ksenia on 12/07/17.
//

#include "PowerTools++/Bag.h"

Bag::Bag(int id){
    _id = id;
    bagsvec = new vector<Node*>();
}

Bag::~Bag(){
    delete bagsvec;
}

int Bag::size(){
    return bagsvec->size();
}

Node* Bag::get_node(int i){
    return bagsvec->at(i);
}

void Bag::add_node(Node* n){
    bagsvec->push_back(n);
}

vector<Bag*>* Bag::split_into_3d(int id){
    vector<Bag*>* bags3 = new vector<Bag*>();
    for (int j = 0; j<size()-2; j++) {
        auto bag3 = new Bag(id);
        bag3->add_node(get_node(j));
        bag3->add_node(get_node(j+1));
        bag3->add_node(get_node(j+2));
        bags3->push_back(bag3);
        id++;
    }
    return bags3;
}