//
// Created by Ksenia on 12/07/17.
//

#ifndef POWERTOOLS_BAG_H
#define POWERTOOLS_BAG_H

#include <PowerTools++/Arc.h>
#include <PowerTools++/Node.h>
#include <igraph.h>

class Arc;

class Bag {
public:
    bool added = false;
    vector<Node*>* _nodes;
    vector<Arc*>* _clone_arcs=nullptr;
    Node* _n1;
    Node* _n2;
    Node* _n3;
    bool _rot;
    int _id;
    igraph_t* graph = nullptr;
    bool _need_to_fix = false;
    bool _added = false;

    Bag(int id);
    ~Bag();

    /** Returns the number of nodes in the bag */
    int size() const;

    Node* get_node(int i) const;

    /** For a bag with size > 3, returns a vector of its subsets of 3 nodes */
    vector<Bag*>* split_into_3d(int id) const;
    bool graph_is_chordal() const;
    bool is_complete() const;

    void add_node(Node* n);

    /** Creates the graph, adds nodes as vertices and fixed lines as edges */
    void create_graph();
};

#endif //POWERTOOLS_BAG_H
