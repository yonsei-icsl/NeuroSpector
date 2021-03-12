#include "optimizer.h"

#define CSV
#define ENERGY_DELAY
//#define DELAY_ENERGY
//#define EDP

static handler_t handler;

/* Optimizer */
optimizer_t::optimizer_t(const std::string& acc_cfg_path_, 
                         const std::string& net_cfg_path_, 
                         const bool is_fixed_, 
                         const unsigned num_threads_) 
    : is_fixed(is_fixed_),
      D_size(static_cast<unsigned>(parameter_t::SIZE)),
      U_size(static_cast<unsigned>(component_t::SIZE)), 
      num_levels(0),
      accelerator(new accelerator_t(acc_cfg_path_)), 
      network_name(""), 
      num_threads(num_threads_), 
      global_total_cnt(0), 
      global_valid_cnt(0) {
    // Component exist bits from MAC to DRAM
    exists.push_back(true); // MAC
    exists.push_back(accelerator->macs_per_pe() * accelerator->mac_width() != 1 ? true : false);
    exists.push_back(accelerator->l1_type() != buffer_type_t::NONE ? true : false);
    exists.push_back(accelerator->s1_size_x() > 1 ? true : false);
    exists.push_back(accelerator->s1_size_y() > 1 ? true : false);
    exists.push_back(accelerator->l2_type() != buffer_type_t::NONE ? true : false);
    exists.push_back(accelerator->s2_size() > 1 ? true : false);
    exists.push_back(true); // DRAM 
    // # of existent levels
    for(unsigned u = 0; u < U_size; u++) {
        if(exists.at(u))
            num_levels++;
    }
    // Mapping tables initialization
    net_cfg_t *net_cfg = new net_cfg_t(net_cfg_path_);
    network_name = net_cfg->network_name;
    for(size_t idx = 0; idx < net_cfg->layers.size(); idx++) {
        mapping_table_t *mapping_table = new mapping_table_t(exists, 
                                                             net_cfg->layers.at(idx).name, 
                                                             net_cfg->layers.at(idx).values,
                                                             net_cfg->layers.at(idx).stride);
        mapping_tables.push_back(mapping_table); 
    }
    delete net_cfg;
}

optimizer_t::~optimizer_t() {
    delete accelerator;
    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
        delete mapping_tables.at(idx);
    }
}

// Optimizer APIs
void optimizer_t::run_brute_force() {
    // TODO
}

void optimizer_t::run_brute_force(const unsigned idx_) {
    accelerator->print_stats();
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# NUM THREADS: " << num_threads << std::endl;
    // Global initialization 
    global_min_energy.assign(num_threads, -1);
    global_best_mapping_table.assign(num_threads, mapping_table_t(exists,
                                                                  mapping_tables.at(idx_ - 1)->get_layer_name(),
                                                                  mapping_tables.at(idx_ - 1)->get_layer_values(),
                                                                  mapping_tables.at(idx_ - 1)->get_stride()));
    std::vector<mapping_table_t> tmp(1, mapping_table_t(exists,
                                                        mapping_tables.at(idx_ - 1)->get_layer_name(),
                                                        mapping_tables.at(idx_ - 1)->get_layer_values(),
                                                        mapping_tables.at(idx_ - 1)->get_stride()));
    global_similar_mapping_tables.assign(num_threads, tmp);
    // Mapping space generation
    mapping_space_t mapping_space(num_levels - 1, mapping_tables.at(idx_ - 1)->get_layer_values());
    // Threads initialization
    std::vector<std::thread> workers;
    std::mutex m;
    for(unsigned tid = 0; tid < num_threads; tid++) {
        workers.push_back(std::thread(&optimizer_t::mapping_worker, 
                                      this, 
                                      idx_, 
                                      tid, 
                                      std::ref(mapping_space), 
                                      component_t::MAC, 
                                      component_t::DRAM, 
                                      std::ref(m)));
    }
    for(unsigned tid = 0; tid < num_threads; tid++) {
        workers.at(tid).join();
    }
    // Sync
    unsigned best_tid = 0;
    size_t best_energy = -1;
    for(unsigned tid = 0; tid < num_threads; tid++) {
        if(best_energy > global_min_energy.at(tid)) {
            best_tid = tid;
            best_energy = global_min_energy.at(tid);
        }
        else if(best_energy == global_min_energy.at(tid)) {
            global_similar_mapping_tables.at(best_tid).push_back(global_best_mapping_table.at(tid));
            for(size_t i = 0; i < global_similar_mapping_tables.at(tid).size(); i++)
                global_similar_mapping_tables.at(best_tid).push_back(global_similar_mapping_tables.at(tid).at(i));
        }
    }
    // Print stats
    handler.print_line(60, "*");
#ifdef ENERGY_DELAY
    std::cout << "# ENERGY-DELAY OPTIMIZATION" << std::endl;
#endif
#ifdef DELAY_ENERGY
    std::cout << "# DELAY-ENERGY OPTIMIZATION" << std::endl;
#endif
#ifdef EDP
    std::cout << "# EDP OPTIMIZATION" << std::endl;
#endif
    handler.print_line(60, "*");
#ifdef CSV
    std::cout << "# OF VALID MAPPINGS,# OF INVALID MAPPINGS,TOTAL # OF MAPPINGS,# OF BEST MAPPINGS\n"  
              << global_valid_cnt << "," << global_total_cnt - global_valid_cnt << "," << mapping_space.get_num_permutations() << "," << global_similar_mapping_tables.at(best_tid).size() + 1 << std::endl;
    handler.print_line(60, "*");
#else
    std::cout << "# NUM OF VALID MAPPINGS  : " << std::setw(15) << global_valid_cnt << " (" << std::fixed << std::setprecision(2) << float(global_valid_cnt) / global_total_cnt * 100 << "%)" << std::endl; 
    std::cout << "# NUM OF INVALID MAPPINGS: " << std::setw(15) << global_total_cnt - global_valid_cnt << " (" << std::fixed << std::setprecision(2) << float(global_total_cnt - global_valid_cnt) / global_total_cnt * 100 << "%)" << std::endl; 
    std::cout << "# TOTAL NUM OF MAPPINGS  : " << std::setw(15) << mapping_space.get_num_permutations() << " (100%)" << std::endl;
    std::cout << "# NUM OF BEST MAPPINGS   : " << std::setw(15) << global_similar_mapping_tables.at(best_tid).size() + 1 << std::endl;
    handler.print_line(60, "*");
#endif
    std::cout << "\n# BEST MAPPING TABLE 0" << std::endl;
#ifdef CSV
    global_best_mapping_table.at(best_tid).print_csv();
#else
    global_best_mapping_table.at(best_tid).print_stats();
#endif
    mapping_table_t *for_stats = new mapping_table_t(exists, 
                                                     mapping_tables.at(idx_ - 1)->get_layer_name(),
                                                     mapping_tables.at(idx_ - 1)->get_layer_values(),
                                                     mapping_tables.at(idx_ - 1)->get_stride());
    for_stats->swap_degrees(global_best_mapping_table.at(best_tid).get_degrees());
    stats_t stats(accelerator, for_stats);
    stats.update_stats();
#ifdef CSV
    stats.print_csv();
#else
    stats.print_stats();
#endif
    delete for_stats;
    for(size_t i = 0; i < global_similar_mapping_tables.at(best_tid).size(); i++) {
        std::cout << "\n# BEST MAPPING TABLE " << i + 1 << std::endl;
#ifdef CSV
        global_similar_mapping_tables.at(best_tid).at(i).print_csv();
#else
        global_similar_mapping_tables.at(best_tid).at(i).print_stats();
#endif
        mapping_table_t *for_stats = new mapping_table_t(exists, 
                                                         mapping_tables.at(idx_ - 1)->get_layer_name(),
                                                         mapping_tables.at(idx_ - 1)->get_layer_values(),
                                                         mapping_tables.at(idx_ - 1)->get_stride());
        for_stats->swap_degrees(global_best_mapping_table.at(best_tid).get_degrees());
        stats_t stats(accelerator, for_stats);
        stats.update_stats();
#ifdef CSV
        stats.print_csv();
#else
        stats.print_stats();
#endif
        delete for_stats;
    }
}

void optimizer_t::run_2level_by_2level(const unsigned idx_) {
    accelerator->print_stats();
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# NUM THREADS: " << num_threads << std::endl;

    bool df_fixed = true;
    unsigned used_levels = 0; 
    // Mapping space generation
    for(unsigned i = 0; i < 3; i++) {
        // L2 & S2
        if(i == 0) {
            if(accelerator->s2_size() > 1) used_levels++;
            if(accelerator->l2_type() != buffer_type_t::NONE) used_levels++;
            if(used_levels == 0) continue;
            mapping_space_t mapping_space(used_levels + 1, mapping_tables.at(idx_ - 1)->get_layer_values());

            if(df_fixed) {

            }
            else {

            }
        }
        // L1 & S1
        else if(i == 1) {
            if(accelerator->s1_size_y() > 1) used_levels++;
            if(accelerator->s1_size_y() > 1) used_levels++;
            if(accelerator->l1_type() != buffer_type_t::NONE) used_levels++;
            if(used_levels == 0) continue;

        }
        // MAC & S0
        else {
            if(accelerator->macs_per_pe() * accelerator->mac_width() > 1) used_levels++;
            if(used_levels == 0) continue;

        }
        used_levels = 0;
    }
}

// Mapping worker
void optimizer_t::mapping_worker(const unsigned idx_, 
                                 const unsigned tid_, 
                                 const mapping_space_t& mapping_space_,
                                 const component_t start_,
                                 const component_t end_,
                                 std::mutex& m_) {
    mapping_table_t *current_mapping_table = new mapping_table_t(exists, 
                                                                 mapping_tables.at(idx_ - 1)->get_layer_name(),
                                                                 mapping_tables.at(idx_ - 1)->get_layer_values(),
                                                                 mapping_tables.at(idx_ - 1)->get_stride());
    mapping_table_t *local_best_mapping_table = new mapping_table_t(exists, 
                                                                    mapping_tables.at(idx_ - 1)->get_layer_name(), 
                                                                    mapping_tables.at(idx_ - 1)->get_layer_values(), 
                                                                    mapping_tables.at(idx_ - 1)->get_stride());
    stats_t *current_stats = new stats_t(accelerator, current_mapping_table);
    size_t min_energy = -1;
    size_t min_cycle = -1;
#ifdef EDP
    double min_edp = DBL_MAX;
#endif
    size_t match_cnt = 0;
    uint64_t total_cnt = 0;
    uint64_t valid_cnt = 0;
    std::vector<mapping_table_t> similar_mapping_tables;
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations(), std::ref(m_));
    for(size_t k = range.start_k; k < range.end_k; k++) {
        current_mapping_table->put_column_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k), start_, end_);
        for(size_t b = range.start_b; b < range.end_b; b++) {
            current_mapping_table->put_column_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b), start_, end_);
            for(size_t p = range.start_p; p < range.end_p; p++) {
                current_mapping_table->put_column_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p), start_, end_);
                for(size_t q = range.start_q; q < range.end_q; q++) {
                    current_mapping_table->put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q), start_, end_);
                    for(size_t c = range.start_c; c < range.end_c; c++) {
                        current_mapping_table->put_column_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c), start_, end_);
                        for(size_t s = range.start_s; s < range.end_s; s++) {
                            current_mapping_table->put_column_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s), start_, end_);
                            for(size_t r = range.start_r; r < range.end_r; r++) {
                                current_mapping_table->put_column_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r), start_, end_);
                                // Find the best mapping table
                                if(check_validity(current_mapping_table)) {
                                    valid_cnt++;
                                    current_stats->update_stats();
#ifdef ENERGY_DELAY
                                    if(min_energy > current_stats->get_total_energy()) {
                                        min_energy = current_stats->get_total_energy();
                                        min_cycle = current_stats->get_total_cycle();
                                        local_best_mapping_table->swap_degrees(current_mapping_table->get_degrees());
                                        match_cnt = 0;
                                    }
                                    else if(min_energy == current_stats->get_total_energy()) {
                                        if(match_cnt == 0)
                                            similar_mapping_tables.clear();
                                        if(min_cycle > current_stats->get_total_cycle()) {
                                            min_cycle = current_stats->get_total_cycle();
                                            local_best_mapping_table->swap_degrees(current_mapping_table->get_degrees());
                                            match_cnt = 0;
                                        }
                                        else if(min_cycle == current_stats->get_total_cycle()) {
                                            mapping_table_t similar_mapping_table(exists, 
                                                                                  mapping_tables.at(idx_ - 1)->get_layer_name(), 
                                                                                  mapping_tables.at(idx_ - 1)->get_layer_values(), 
                                                                                  mapping_tables.at(idx_ - 1)->get_stride());
                                            similar_mapping_table.swap_degrees(current_mapping_table->get_degrees());
                                            similar_mapping_tables.push_back(similar_mapping_table);
                                            match_cnt++;
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                    }
                                    else {
                                        // Nothing to do
                                    }
#endif
#ifdef DELAY_ENERGY
                                    if(min_cycle > current_stats->get_total_cycle()) {
                                        min_energy = current_stats->get_total_energy();
                                        min_cycle = current_stats->get_total_cycle();
                                        local_best_mapping_table->swap_degrees(current_mapping_table->get_degrees());
                                        match_cnt = 0;
                                    }
                                    else if(min_cycle == current_stats->get_total_cycle()) {
                                        if(match_cnt == 0)
                                            similar_mapping_tables.clear();
                                        if(min_energy > current_stats->get_total_energy()) {
                                            min_energy = current_stats->get_total_energy();
                                            local_best_mapping_table->swap_degrees(current_mapping_table->get_degrees());
                                            match_cnt = 0;
                                        }
                                        else if(min_energy == current_stats->get_total_energy()) {
                                            mapping_table_t similar_mapping_table(exists, 
                                                                                  mapping_tables.at(idx_ - 1)->get_layer_name(), 
                                                                                  mapping_tables.at(idx_ - 1)->get_layer_values(), 
                                                                                  mapping_tables.at(idx_ - 1)->get_stride());
                                            similar_mapping_table.swap_degrees(current_mapping_table->get_degrees());
                                            similar_mapping_tables.push_back(similar_mapping_table);
                                            match_cnt++;
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                    }
                                    else {
                                        // Nothing to do
                                    }
#endif
#ifdef EDP
                                    if(min_edp > current_stats->get_total_edp()) {
                                        min_energy = current_stats->get_total_energy();
                                        min_cycle = current_stats->get_total_cycle();
                                        min_edp = current_stats->get_total_edp();
                                        local_best_mapping_table->swap_degrees(current_mapping_table->get_degrees());
                                        match_cnt = 0;
                                    }
                                    else if(min_edp == current_stats->get_total_edp()) {
                                        if(match_cnt == 0)
                                            similar_mapping_tables.clear();
                                        mapping_table_t similar_mapping_table(exists, 
                                                                              mapping_tables.at(idx_ - 1)->get_layer_name(), 
                                                                              mapping_tables.at(idx_ - 1)->get_layer_values(), 
                                                                              mapping_tables.at(idx_ - 1)->get_stride());
                                        similar_mapping_table.swap_degrees(current_mapping_table->get_degrees());
                                        similar_mapping_tables.push_back(similar_mapping_table);
                                        match_cnt++;
                                    }
                                    else {
                                        // Nothing to do
                                    }
#endif
                                }
                                total_cnt++;
                            }
                        }
                    }
                }
            }
        }
    }
    // Sync
    m_.lock();
    global_total_cnt += total_cnt;
    global_valid_cnt += valid_cnt;
    global_min_energy.at(tid_) = min_energy;
    global_best_mapping_table.at(tid_).swap_degrees(local_best_mapping_table->get_degrees());
    global_similar_mapping_tables.at(tid_).assign(similar_mapping_tables.begin(), similar_mapping_tables.end());
    m_.unlock();
    // Delete
    delete current_stats;
    delete local_best_mapping_table;
    delete current_mapping_table;
    return;
}

// Check each mapping table with the accelerator
bool optimizer_t::check_validity(const mapping_table_t *mapping_table_) const {
    bool rtn = mac_validity(mapping_table_) 
             & s0_validity(mapping_table_) 
             & l1_validity(mapping_table_)
             & s1_x_validity(mapping_table_)
             & s1_y_validity(mapping_table_)
             & l2_validity(mapping_table_)
             & s2_validity(mapping_table_);
    return rtn;
}

bool optimizer_t::mac_validity(const mapping_table_t *mapping_table_) const {
    return true;
}     

bool optimizer_t::s0_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned macs_per_pe_val = 1;
    unsigned mac_width_val = 1;
    macs_per_pe_val *= mapping_table_->get_degree(parameter_t::K, component_t::S0)
                     * mapping_table_->get_degree(parameter_t::B, component_t::S0)
                     * mapping_table_->get_degree(parameter_t::P, component_t::S0)
                     * mapping_table_->get_degree(parameter_t::Q, component_t::S0);
    mac_width_val *= mapping_table_->get_degree(parameter_t::C, component_t::S0)
                   * mapping_table_->get_degree(parameter_t::R, component_t::S0)
                   * mapping_table_->get_degree(parameter_t::S, component_t::S0);
    if(macs_per_pe_val > accelerator->macs_per_pe() || mac_width_val > accelerator->mac_width()) 
        validity = false;
    return validity;
}

bool optimizer_t::l1_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l1_type()) {
        case buffer_type_t::NONE: break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l1_input_bypass() && mapping_table_->get_input_tile_size(component_t::L1) > accelerator->l1_input_size())
                validity = false;
            if(!accelerator->l1_filter_bypass() && mapping_table_->get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size())
                validity = false;
            if(!accelerator->l1_output_bypass() && mapping_table_->get_output_tile_size(component_t::L1) > accelerator->l1_output_size())
                validity = false;
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L1) 
                             + mapping_table_->get_filter_tile_size(component_t::L1)
                             + mapping_table_->get_output_tile_size(component_t::L1);
            if(shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L1) 
                             + mapping_table_->get_filter_tile_size(component_t::L1);
            if((!accelerator->l1_output_bypass() && mapping_table_->get_output_tile_size(component_t::L1) > accelerator->l1_output_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_table_->get_filter_tile_size(component_t::L1) 
                             + mapping_table_->get_output_tile_size(component_t::L1);
            if((!accelerator->l1_input_bypass() && mapping_table_->get_input_tile_size(component_t::L1) > accelerator->l1_input_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L1) 
                             + mapping_table_->get_output_tile_size(component_t::L1);
            if((!accelerator->l1_filter_bypass() && mapping_table_->get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L1 TYPE"); break;
    } 
    return validity;
}     

bool optimizer_t::s1_x_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned s1_size_x_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_x_val *= mapping_table_->get_degree(static_cast<parameter_t>(column), component_t::S1_X);
    if(s1_size_x_val > accelerator->s1_size_x()) 
        validity = false;
    return validity;
}     

bool optimizer_t::s1_y_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned s1_size_y_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_y_val *= mapping_table_->get_degree(static_cast<parameter_t>(column), component_t::S1_Y);
    if(s1_size_y_val > accelerator->s1_size_y()) 
        validity = false;
    return validity;
}     

bool optimizer_t::l2_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l2_type()) {
        case buffer_type_t::NONE: break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l2_input_bypass() && mapping_table_->get_input_tile_size(component_t::L2) > accelerator->l2_input_size())
                validity = false;
            if(!accelerator->l2_filter_bypass() && mapping_table_->get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size())
                validity = false;
            if(!accelerator->l2_output_bypass() && mapping_table_->get_output_tile_size(component_t::L2) > accelerator->l2_output_size())
                validity = false;
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L2) 
                             + mapping_table_->get_filter_tile_size(component_t::L2)
                             + mapping_table_->get_output_tile_size(component_t::L2);
            if(shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L2) 
                             + mapping_table_->get_filter_tile_size(component_t::L2);
            if((!accelerator->l2_output_bypass() && mapping_table_->get_output_tile_size(component_t::L2) > accelerator->l2_output_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_table_->get_filter_tile_size(component_t::L2) 
                             + mapping_table_->get_output_tile_size(component_t::L2);
            if((!accelerator->l2_input_bypass() && mapping_table_->get_input_tile_size(component_t::L2) > accelerator->l2_input_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_table_->get_input_tile_size(component_t::L2) 
                             + mapping_table_->get_output_tile_size(component_t::L2);
            if((!accelerator->l2_filter_bypass() && mapping_table_->get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L2 TYPE"); break;
    } 
    return validity;
}     

bool optimizer_t::s2_validity(const mapping_table_t *mapping_table_) const {
    bool validity = true;
    unsigned s2_size_val = 1;
    for(unsigned column = 0; column < D_size; column++) {
        // Only K, B, P, and Q
        if(column == 0 || column == 1 || column == 2 || column == 3)
            s2_size_val *= mapping_table_->get_degree(static_cast<parameter_t>(column), component_t::S2);
        else {
            if(mapping_table_->get_degree(static_cast<parameter_t>(column), component_t::S2) > 1) {
                validity = false;
                break;
            }
        }
    }
    if(s2_size_val > accelerator->s2_size()) 
        validity = false;
    return validity;
}     