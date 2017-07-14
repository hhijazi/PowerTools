//
// Created by Ksenia on 12/07/17.
//

#ifndef POWERTOOLS_BAG_H
#define POWERTOOLS_BAG_H

#include <PowerTools++/Arc.h>
#include <PowerTools++/Node.h>

class Bag {
public:
    vector<Node*>* bagsvec;
    Node* _n1;
    Node* _n2;
    Node* _n3;
    bool _rot;
    int _id;

    Bag(int id);
    ~Bag();

    int size();
    Node* get_node(int i);

    void add_node(Node* n);
    vector<Bag*>* split_into_3d(int id);
};

#endif //POWERTOOLS_BAG_H
