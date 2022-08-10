#ifndef __SCHEDULING_TABLE_H__
#define __SCHEDULING_TABLE_H__

#include <cassert>

#include "accelerator.h"
#include "network.h"

class scheduling_table_t {
public:
    scheduling_table_t();
    scheduling_table_t(accelerator_t *accelerator_,
                       network_t *network_);
    scheduling_table_t(accelerator_t *accelerator_,
                       network_t *network_,
                       const std::string& scheduling_table_pth_);
    ~scheduling_table_t();
    scheduling_table_t(const scheduling_table_t &scheduling_table_);      // Copy constructor
    void init();                                                    // Init. Scheduling table
    void init_table_rows();                                         // Init. # table rows
    void init_mapping_values();                                     // Init. Mapping values
    void print_stats();                                             // Print scheduling table

    unsigned get_above_buffer_pos(unsigned pos_) const;             // Get buffer index one level above 
    unsigned get_below_buffer_pos(unsigned pos_) const;          // Get lower temporal level's index
    unsigned get_num_rows() const;                                  // Get # rows of scheduling table
    unsigned get_correlation_product(int idx_, 
                                     correlation_type_t correlation_);
    std::string get_component_name(unsigned idx_) const;            // Get target component name 
    component_type_t get_component_type(unsigned idx_) const;       // Get reuse type of target component 
    unsigned get_component_index(unsigned idx_) const;              // Get accelerator index of component
    std::vector<unsigned> get_row_values(unsigned idx_) const;      // Get mapping values of a component level
    unsigned get_mapping_value(unsigned row_, unsigned col_) const; // Get mapping value at (row, col) 
    unsigned get_layer_index();                                     // Get current layer's index
    std::vector<unsigned> get_layer_parameters();                                     // Get current layer's index

    float get_num_mac_operations();                               // Get total number of MAC operations in a layer 

    bool is_virtual(unsigned idx_);
    
    void load_dnn_layer(unsigned idx_);                             // Update DRAM mapping values to layer parameters
    void update_set_of_rows(unsigned begin_, unsigned end_, 
                            std::vector<std::vector<unsigned>> mapping_values_set_);
    void update_dataflow(std::vector<dataflow_t>);
    void fill_out_mapping_values(const parser_t parser_);
    unsigned get_column_wise_product(parameter_t param_, 
                                     unsigned begin_, unsigned end_);
    
    bool operator!=(const scheduling_table_t& scheduling_table_);

private:
    void add_virtual_component(component_type_t component_type_); 
    void add_virtual_component(component_type_t component_type_,
                               unsigned component_idx_); 
    void update_row(unsigned component_pos_, 
                    std::vector<unsigned> mapping_values_);
    void update_mapping_value(unsigned dst_, unsigned val_); 

    accelerator_t         *accelerator;                             // Target accelerator
    network_t             *network;                                 // Target network

    std::string           layer_name; 
    unsigned              layer_index;
    std::vector<unsigned> layer_parameters;
    float                 num_mac_operations;                      // Total number of MAC operations in a layer

    unsigned              num_table_rows;                           // # table rows
    unsigned              num_table_cols;                           // # table cols
    std::vector<unsigned> mapping_values;                           // Mapping_values
    std::vector<std::string> row_names;                             // row's names
    std::vector<component_type_t> row_types;                        // row's types 
    std::vector<unsigned>    row_index;                             // row's determined
};

struct PartitioningInfo {
    float    cost = 0.0;
    bool     is_parallelized    = true;
    unsigned input_tile_size    = 0;
    unsigned input_access_count = 0;
    unsigned num_assigned_chips = 1;
    scheduling_table_t scheduling_table;
};

#endif 