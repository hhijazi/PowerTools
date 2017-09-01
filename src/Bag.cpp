//
// Created by Ksenia on 12/07/17.
//

#include "PowerTools++/Bag.h"
#include <igraph.h>

Bag::Bag(int id){
    _id = id;
    _nodes = new vector<Node*>();
    _clone_arcs = new vector<Arc*>();
}

Bag::~Bag(){
    delete _nodes;
    delete _clone_arcs;
    delete graph;
}

int Bag::size() const{
    return _nodes->size();
}

Node* Bag::get_node(int i) const{
    return _nodes->at(i);
}

vector<Bag*>* Bag::split_into_3d(int id) const{
    assert(size()>3);
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

bool Bag::graph_is_chordal() const{
    igraph_bool_t res;
    igraph_is_chordal(graph,nullptr,nullptr,&res,nullptr,nullptr);
    return res;
}

bool Bag::is_complete() const{
    for(auto a: *_clone_arcs){
        if(a->free) return false;
    }
    return true;
}

void Bag::add_node(Node* n){
    _nodes->push_back(n);
}

void Bag::create_graph(){
//    FILE* f;
//    string fname = "graph" + to_string(_id) + ".txt";
//    cout << "\n" << fname;
//    f = fopen (fname.c_str(), "w");
    Node* ni, *nj;
    graph = new igraph_t;
    igraph_empty(graph, _nodes->size(), IGRAPH_UNDIRECTED);
    for (int i = 0; i <= _nodes->size() - 2; ++i) {
        for (int j = i + 1; j <= _nodes->size() - 1; ++j) {
            ni = _nodes->at(i);
            nj = _nodes->at(j);
//                cout << "\nni = " << ni->_name << " nj = " << nj->_name;
            if (ni->is_connected_fixed(nj)) {
//                    cout << "\nadding edge";
                igraph_add_edge(graph, i, j);
            }
        }
    }
//    igraph_write_graph_edgelist(&graph,f);
}