//
//  Net.h
//  Cycle_Basis_PF
//
//  Created by Sumiran on 16/06/2014.
//  Copyright (c) 2014 NICTA. All rights reserved.
//

#ifndef Cycle_Basis_PF_Net_h
#define Cycle_Basis_PF_Net_h

#include <map>
#include <math.h>
#include <PowerTools++/Arc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <PowerTools++/Node.h>
#include <PowerTools++/Path.h>
#include <PowerTools++/PowerModel.h>
#include <assert.h>


class Net{
public:

    int _bus_count; // buses are i
//    int _time_count; //time instances are j


    double bMVA;
    

    /** PV capacity in kW */
    double PV_CAP = -1;
    
    /** PV efficiency */
    double PV_EFF = -1;

    /** PV capacity in kW */
    double BATT_CAP = -1;
    
    /** PV efficiency */
    double BATT_EFF = -1;
    
    double              _peak_rate = -1;
    double              _demand_growth = -1;
    int                 _nb_years = -1, _nb_samples = -1;
    int                 _demand_nb_years = -1, _irrad_nb_years = -1;
    double              _pv_cost = -1;// $ per Watt for PV rooftop installation
    double              _price_inflation = -1;
    double              _metering_charges = -1;
    int                 _nb_meters = -1; /*< Number of meters installed */
    int                 _nb_connect_points = -1; /*< Number of connection points to the Utility */
    double              _supply_charges = -1;
    double              _uncert_perc = -1;
    
    std::vector<double> weekday_cost;
    std::vector<double> weekend_cost;
    double              min_cost_week = MAXFLOAT;/*< Smallest Time of Use rate for weekdays */
    double              min_cost_weekend = MAXFLOAT;/*< Smallest Time of Use rate for weekdays */
    
//    var<> max_kVa;//Max demand for the last 13 months
    
    vector<var<>> max_kVas_month; // Max demands for each billing period
    vector<var<>> max_kVas_year; // Max demands for the last 13 months
    vector<var<>> pv_gen; // Global PV generation in network, time dependant
    vector<var<>> batt; // Global Energy stored in Batteries, time dependant
    vector<var<>> gen; // Global generation from utility, time dependant
    
    std::map<string, int> busmap;  //mapping between building names in load file and bus ids.
    
    string _name;
    
    /** Set of nodes */
    std::vector<Node*> nodes;
    
    /** Set of arcs */
    std::vector<Arc*> arcs;
    
    /** Set of generators */
    std::vector<Gen*> gens;
    
    /**radiation limit time dependent*/
    std::vector<double>      _radiation;
    
    /**Power factor time dependent*/
    std::vector<double>      _power_factor;
    
    /**Overall apparent power demand, time dependent*/
    std::vector<double>      _load_kVA;
    
    /**Overall real power demand, time dependent*/
    std::vector<double>      _load_kW;
    
    /**Date represented as a tuple <Year,Month,Day,hour>*/
    vector<tuple<int,int,int,int>>           _date;

    
    /* tag the weekdays */
    vector<int> tag;


    
    /** Mapping the arcs to their source-destination */
    std::map<std::string, std::set<Arc*>*> lineID;
    
    /** Mapping the node id to its position in the vector, key = node id */
    std::map<std::string, Node*> nodeID;
    
    /** Vector of cycles forming a cycle basis */
    std::vector<Path*> cycle_basis;
    
    /** Horton subnetwork */
    Net* horton_net;

    /** Clone network */
    Net* _clone;
    
    /** Tree decomposition bags */
    std::vector<std::vector<Node*>*>* _bags;

    /** Compute the tree decomposition bags **/
    void get_tree_decomp_bags();
    
    double m_theta_ub;
    double m_theta_lb;
    
    /** structure for a new constraint */
    struct constraint{
        int cycle_index;
        /** it stors the gen node, the id of link tp next node, next node, id of link to prev node, prev node */
        int arr[5];
        double cnst;
    };
    
    /** vector of constraints bounding ratio of current magnitudes */
    std::vector<constraint> new_constraints;

    
    /* Constructors */
    
    Net();
    
    /* Destructors */
    
    ~Net();
    
    /** Erase Horton network */
    void clear_horton_net();
    
    /** Modifiers */
    
    void add_node(Node* n);
    
    /**  @brief Remove node and all incident arcs from the network
        @note Does not remove the incident arcs from the list of arcs in the network!
        @return the id of the node removed
     */
    std::string remove_end_node();
    
    bool add_arc(Arc* a);
    
    /** Returns true if the cycle in the bag is rotated in one direction only */
    bool rotation_bag(vector<Node*>* b);
    
    void remove_arc(Arc* a);

    /** Sort nodes in decreasing degree */
    void orderNodes(Net* net);
    
    /** Sort arcs in decreasing weight */
    void orderArcs(Net* net);
    
    void add_horton_nodes(Net* net);
    
    void add_horton_branches(Net* net);
    
    /* Accessors */
    
    Node* get_node(std::string id);

    /* Input Output */
    
    int readFile(std::string fname);
    //int readFile_direct(std::string fname);
    //vector<int> weekday_tag();
    int readload(std::string ffname, int _timesteps);
    
    int read_agg_load(std::string fname); // Read aggregated load
    int read_agg_rad(std::string fname); // Read aggregated radiation
    int read_agg_rad_all(std::string path); // Read aggregated radiation form all files stored in path
    
    int readpvmax(std::string ffname);
    int readcost(std::string ffname, int _timesteps);
    int readparams(std::string ffname);
    int choosetime();
    int readrad(std::string fname, int _timesteps);
    int readmap(std::string ffname, int timesteps);
    
   
    void writeDAT(std::string name);
    
    /* Algorithms */
    
    /** Reset all distances to max value */
    void resetDistance();
    
    /** Computes and returns the shortest path between src and dest in net */
    Path* Dijkstra(Node* src, Node* dest, Net* net);
    
    /** Computes a spanning tree of minimal weight in horton's network, then adds the corresponding cycles to the original network by connecting to src
        @note Implements Kruskal’s greedy algorithm
     */
    void minimal_spanning_tree(Node* src, Net* net);

    /** Combines the src node with the path p to form a cycle */
    void combine(Node* src, Path* p);
    
    void Fast_Horton();
    
    void Fast_Horton(Net *net);
    
    /** adds decimal to an integer for precision */
    void precise(std::ofstream &myfile, float f);
    
    /** Cloning */
    Net* clone();
    
    /** finds arc parallel to the given arc and prints the self loop */
    int self_loops(std::ofstream &myfile, Arc* arc, int cycle_index);
    
    /** returns the arc formed by node ids n1 and n2 */
    Arc* get_arc(Node* n1, Node* n2);

    /** returns the arc formed by node ids n1 and n2 */
    Arc* get_arc(int n1, int n2);

    
    /** returns true if the arc formed by node ids n1 and n2 exists */
    bool has_arc(int n1, int n2);

    bool has_directed_arc(Node* n1, Node* n2);

    /** writes path p on myfile */
    void write(std::ofstream &myfile, Path* p, int cycle_index);
    
    /** returns true if an arc is already present between the given nodes */
    bool duplicate(int n1, int n2, int id1);
    
    /** identifies the generator bus in the cycle*/
    void s_flowIn_cycle(Path* p);
    
    /** checks neighbours of a gen bus if present in any cycle*/
    bool check_neighbour(Node* n);
    
    /** forms constraint for each cycle in the cycle basis */
    void Constraint();
    
    /** computes the constant for each gen node in each cycle */
    double compute_constant(Path* cycle);
    
    /** rearranges cycle according to gen node */
    Path* form_cycle(Node* n, Path* p);
    
    /** adds constraint to vector new_constraints */
    void addConstraint(int c, Path* cycle);
    
    /** sets the cycle member fo all Nodes to be false */
    void reset_nodeCycle();
    
    /** sets all nodes to unexplored */
    void reset_nodeExplored();
};
#endif
