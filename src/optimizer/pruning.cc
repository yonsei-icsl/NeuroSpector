#include "optimizer.h"

static handler_t handler;

/* Pruning (s-t or t-s) */
pruning_t::pruning_t(const std::string& acc_cfg_path_, 
                     const std::string& net_cfg_path_, 
                     const opt_type_t opt_type_, 
                     const unsigned num_threads_,
                     const bool is_fixed_)
    : optimizer_t(acc_cfg_path_, net_cfg_path_, is_fixed_),
      opt_type(opt_type_),
      num_threads(num_threads_),
      num_spatial(0),
      num_temporal(0),
      final_best_stat(DBL_MAX) {
    // if(exists.at(static_cast<unsigned>(component_t::S0)) || exists.at(static_cast<unsigned>(component_t::S2))) {
    //     handler.print_err(err_type_t::INVAILD, "s-t or t-s pruning optimizer does not support scaling");
    //     exit(1);
    // }
    for(unsigned row = 1; row < U_size; row++) {
        if((row == static_cast<unsigned>(component_t::L1) || row == static_cast<unsigned>(component_t::L2)) && exists.at(row))
            num_temporal++;
        if((row == static_cast<unsigned>(component_t::S0)   ||
            row == static_cast<unsigned>(component_t::S1_X) ||
            row == static_cast<unsigned>(component_t::S1_Y) ||
            row == static_cast<unsigned>(component_t::S2)) && exists.at(row))
            num_spatial++;
    }
}

pruning_t::~pruning_t() {

}

// Optimizer APIs
void pruning_t::run() {
    for(unsigned idx = 0; idx < mappings.size(); idx++) {
        final_best_stat = DBL_MAX;
        final_best_mappings.clear();
        final_best_dataflows.clear();
        run(idx + 1);
    }
    return;
}

void pruning_t::run(const unsigned idx_) {
    handler.print_line(50, "*");
    if(opt_type == opt_type_t::S_T)
        std::cout << "# PRUNING (S-T) OPTIMIZATION" << std::endl;
    else 
        std::cout << "# PRUNING (T-S) OPTIMIZATION" << std::endl;
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# LAYER      : " << mappings.at(idx_ - 1).get_layer_name() << std::endl;
    handler.print_line(50, "*");
    for(unsigned l1_df = 0; l1_df < l1_dataflows.size(); l1_df++) {
        for(unsigned l2_df = 0; l2_df < l2_dataflows.size(); l2_df++) {
            // Reset variables & containers
            reset(idx_);
            // Run the engine
            engine(idx_,
                   static_cast<dataflow_t>(l1_dataflows.at(l1_df)),
                   static_cast<dataflow_t>(l2_dataflows.at(l2_df)));
            // Sync and Update
            sync_and_update(static_cast<dataflow_t>(l1_dataflows.at(l1_df)),
                            static_cast<dataflow_t>(l2_dataflows.at(l2_df)));
            // TODO: print stats per dataflow
        }
    }
    // Print stats
#ifdef CSV
    print_csv();
#else
    print_stats();
#endif
    return;
}


void pruning_t::print_stats() {
    std::string df_str("IWO");
    handler.print_line(50, "*");
    std::cout << "# SPATIAL"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(0) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(0) << std::endl;
    std::cout << "# TEMPORAL"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(1) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(1) << std::endl;
    handler.print_line(50, "*");
    return;
}

void pruning_t::print_csv() {
    std::string df_str("IWO");
    handler.print_line(50, "*");
    std::cout << "# SPATIAL"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(0) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(0) << std::endl;
    std::cout << "# TEMPORAL"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(1) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(1) << std::endl;
    std::cout << "# VALID MAPPINGS,TOTAL MAPPINGS,SIMILAR MAPPINGS\n"  
              << valid_cnt.at(0) + valid_cnt.at(1) << "," 
              << total_cnt.at(0) + total_cnt.at(1) << "," 
              << final_best_mappings.size() << std::endl;
    std::cout << "TOTAL SEARCH SPACE, " << total_cnt.at(0) + total_cnt.at(1) << std::endl;
    handler.print_line(50, "*");
    for(unsigned i = 0; i < final_best_mappings.size(); i++) {
        std::cout << "# BEST MAPPING TABLE " << i + 1 << std::endl;
        std::cout << "# DATAFLOWS (L1-L2): " 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(0))) 
                  << "S-" 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(1))) 
                  << "S" << std::endl; 
        final_best_mappings.at(i).print_csv();
        stats_t stats_tmp(accelerator, final_best_mappings.at(i), final_best_dataflows.at(0), final_best_dataflows.at(1));
        stats_tmp.update_stats();
        stats_tmp.print_csv();
        handler.print_line(50, "-");
    }
    return;
}

// Optimizer private functions
void pruning_t::reset(const unsigned idx_) {
    valid_cnt.clear();
    total_cnt.clear();
    valid_cnt.assign(2, 0);
    total_cnt.assign(2, 0);
    best_mappings.clear();
    best_mappings.assign(num_threads, std::make_pair(DBL_MAX, mapping_table_t(mappings.at(idx_ - 1))));
    // Reset similar_mappings
    similar_mappings.clear();
    std::vector<mapping_table_t> mapping_tmp(1, mapping_table_t(mappings.at(idx_ - 1)));
    similar_mappings.assign(num_threads, mapping_tmp);
    return;
}

void pruning_t::engine(const unsigned idx_,
                       const dataflow_t l1_dataflow_,
                       const dataflow_t l2_dataflow_) {
    // Mapping space generation
    static unsigned prev_idx = -1;
    static mapping_space_t spatial_mapping_space;
    static mapping_space_t mapping_space; //FOR BRUTEFORCE

    mapping_space = mapping_space_t(num_levels - 1, mappings.at(idx_ - 1).get_layer_values());
    std::cout << "BRUTEFORCE, " << mapping_space.get_num_permutations() << std::endl;
    if(prev_idx != idx_) {
        spatial_mapping_space = mapping_space_t(num_spatial + 1, mappings.at(idx_ - 1).get_layer_values());
        total_cnt.at(0) = spatial_mapping_space.get_num_permutations();
    }
    prev_idx = idx_;
    // Threads initialization
    std::vector<std::thread> workers;
    std::mutex m;
    // Spatial mapping space generation 
    // Spatial mapping first
    for(unsigned tid = 0; tid < num_threads; tid++) {
        workers.push_back(std::thread(&pruning_t::spatial_first_worker, 
                                      this, 
                                      tid, 
                                      std::ref(mappings.at(idx_ - 1)),
                                      std::ref(spatial_mapping_space),
                                      l1_dataflow_,
                                      l2_dataflow_,
                                      std::ref(m)));
    }
    // Thread join
    for(unsigned tid = 0; tid < num_threads; tid++) 
        workers.at(tid).join();
    workers.clear();
    // TODO : Additional Options for various optimizing strategies  
    return;
}

void pruning_t::sync_and_update(const dataflow_t l1_dataflow_,
                                const dataflow_t l2_dataflow_) {
    double local_min_energy = DBL_MAX;
    double local_min_cycle = DBL_MAX;
    double local_min_edp = DBL_MAX;
    std::vector<mapping_table_t> local_final_best_mappings;
    
    for(unsigned tid = 0; tid < num_threads; tid++) {
        if(local_min_energy >  best_mappings.at(tid).first) {
            local_min_energy = best_mappings.at(tid).first;
            stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
            stats_tmp.update_stats();
            local_min_cycle = stats_tmp.get_total_cycle();
            local_final_best_mappings.clear();
            local_final_best_mappings.push_back(best_mappings.at(tid).second);
            for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                local_final_best_mappings.push_back(*it);
        }
        else if(local_min_energy == best_mappings.at(tid).first) {
            stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
            stats_tmp.update_stats();
            // If energy values are equal, then compare cycle
            if(local_min_cycle > stats_tmp.get_total_cycle()) {
                local_min_cycle = stats_tmp.get_total_cycle();
                local_final_best_mappings.clear();
                local_final_best_mappings.push_back(best_mappings.at(tid).second);
                for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                    local_final_best_mappings.push_back(*it);
            }
            else if(local_min_cycle == stats_tmp.get_total_cycle()) {
                local_final_best_mappings.push_back(best_mappings.at(tid).second);
                for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                    local_final_best_mappings.push_back(*it);
            }
            else { /* Nothing to do */ }
        }
        else { /* Nothing to do */ }
    }
    // Update
    if(final_best_stat > local_min_energy) {
        final_best_stat = local_min_energy;
        final_best_mappings.clear();
        final_best_mappings.assign(local_final_best_mappings.begin(), local_final_best_mappings.end());
        final_best_dataflows.clear();
        final_best_dataflows.push_back(l1_dataflow_);
        final_best_dataflows.push_back(l2_dataflow_);
    }
    return;
}

void pruning_t::spatial_first_worker(const unsigned tid_,
                                     const mapping_table_t& init_mapping_,
                                     const mapping_space_t& mapping_space_, 
                                     const dataflow_t l1_dataflow_,
                                     const dataflow_t l2_dataflow_,
                                     std::mutex& m_) {
    // Initialize local variables & containers
    double local_min_energy = DBL_MAX;
    double local_min_cycle = DBL_MAX;
    uint64_t local_spatial_valid_cnt = 0;
    uint64_t local_temporal_valid_cnt = 0;
    uint64_t local_temporal_total_cnt = 0;
    mapping_table_t local_best_mapping(init_mapping_);
    std::vector<mapping_table_t> local_similar_mappings;
    // Current spatial mapping table
    mapping_table_t curr_spatial_mapping(init_mapping_);
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Start spatial mapping first
    for(size_t g = range.start_g; g < range.end_g; g++) {
        curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g));
        for(size_t k = range.start_k; k < range.end_k; k++) {
            curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k));
            for(size_t b = range.start_b; b < range.end_b; b++) {
                curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b));
                for(size_t p = range.start_p; p < range.end_p; p++) {
                    curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p));
                    for(size_t q = range.start_q; q < range.end_q; q++) {
                        curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q));
                        for(size_t c = range.start_c; c < range.end_c; c++) {
                            curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c));
                            for(size_t s = range.start_s; s < range.end_s; s++) {
                                curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s));
                                for(size_t r = range.start_r; r < range.end_r; r++) {
                                    curr_spatial_mapping.put_column_spatial_first_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r));
                                    // Spatial validity check
                                    if(check_validity(component_t::S0, curr_spatial_mapping)   &&
                                       check_validity(component_t::S1_X, curr_spatial_mapping) &&
                                       check_validity(component_t::S2, curr_spatial_mapping)) {
                                        local_spatial_valid_cnt++;
                                        // Temporal mapping space generation
                                        mapping_space_t temporal_mapping_space(num_temporal + 1, curr_spatial_mapping.get_row_degrees(component_t::DRAM));
                                        local_temporal_total_cnt += temporal_mapping_space.get_num_permutations();
                                        mapping_table_t curr_temporal_mapping(curr_spatial_mapping);
                                        // Start temporal mapping second
                                        for(size_t g = 0; g < temporal_mapping_space.get_permutations(0).size(); g++) {
                                            curr_temporal_mapping.put_column_temporal_degrees(parameter_t::G, temporal_mapping_space.get_permutations(0).at(g));
                                            for(size_t k = 0; k < temporal_mapping_space.get_permutations(1).size(); k++) {
                                                curr_temporal_mapping.put_column_temporal_degrees(parameter_t::K, temporal_mapping_space.get_permutations(1).at(k));
                                                for(size_t b = 0; b < temporal_mapping_space.get_permutations(2).size(); b++) {
                                                    curr_temporal_mapping.put_column_temporal_degrees(parameter_t::B, temporal_mapping_space.get_permutations(2).at(b));
                                                    for(size_t p = 0; p < temporal_mapping_space.get_permutations(3).size(); p++) {
                                                        curr_temporal_mapping.put_column_temporal_degrees(parameter_t::P, temporal_mapping_space.get_permutations(3).at(p));
                                                        for(size_t q = 0; q < temporal_mapping_space.get_permutations(4).size(); q++) {
                                                            curr_temporal_mapping.put_column_temporal_degrees(parameter_t::Q, temporal_mapping_space.get_permutations(4).at(q));
                                                            for(size_t c = 0; c < temporal_mapping_space.get_permutations(5).size(); c++) {
                                                                curr_temporal_mapping.put_column_temporal_degrees(parameter_t::C, temporal_mapping_space.get_permutations(5).at(c));
                                                                for(size_t s = 0; s < temporal_mapping_space.get_permutations(6).size(); s++) {
                                                                    curr_temporal_mapping.put_column_temporal_degrees(parameter_t::S, temporal_mapping_space.get_permutations(6).at(s));
                                                                    for(size_t r = 0; r < temporal_mapping_space.get_permutations(7).size(); r++) {
                                                                        curr_temporal_mapping.put_column_temporal_degrees(parameter_t::R, temporal_mapping_space.get_permutations(7).at(r));
                                                                        // Temporal validity check
                                                                        if(check_validity(component_t::L1, curr_temporal_mapping) && check_validity(component_t::L2, curr_temporal_mapping)) {
                                                                            stats_t curr_stats(accelerator, curr_temporal_mapping, l1_dataflow_, l2_dataflow_);
                                                                            curr_stats.update_stats();
                                                                            if(local_min_energy > curr_stats.get_total_energy()) {
                                                                                local_min_energy = curr_stats.get_total_energy();
                                                                                local_min_cycle = curr_stats.get_total_cycle();
                                                                                local_best_mapping.swap_degrees(curr_temporal_mapping.get_degrees());
                                                                                local_similar_mappings.clear();
                                                                            }
                                                                            else if(local_min_energy == curr_stats.get_total_energy()) {
                                                                                if(local_min_cycle > curr_stats.get_total_cycle()) {
                                                                                    local_min_cycle = curr_stats.get_total_cycle();
                                                                                    local_best_mapping.swap_degrees(curr_temporal_mapping.get_degrees());
                                                                                    local_similar_mappings.clear();
                                                                                }
                                                                                else if(local_min_cycle == curr_stats.get_total_cycle()) {
                                                                                    mapping_table_t similar_mapping_tmp(curr_temporal_mapping);
                                                                                    local_similar_mappings.push_back(similar_mapping_tmp);
                                                                                }
                                                                                else {
                                                                                    // Nothing to do
                                                                                }
                                                                            }
                                                                            else {
                                                                                // Nothing to do
                                                                            }
                                                                            local_temporal_valid_cnt++;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(local_spatial_valid_cnt > 0) {
        m_.lock();
        valid_cnt.at(0) += local_spatial_valid_cnt;
        valid_cnt.at(1) += local_temporal_valid_cnt;
        total_cnt.at(1) += local_temporal_total_cnt;
        m_.unlock();
        best_mappings.at(tid_).first = local_min_energy;
        best_mappings.at(tid_).second.swap_degrees(local_best_mapping.get_degrees());
        similar_mappings.at(tid_).assign(local_similar_mappings.begin(), local_similar_mappings.end());
    }
    return;
}

void pruning_t::temporal_first_worker(const unsigned tid_,
                                      const mapping_table_t& init_mapping_,
                                      const mapping_space_t& mapping_space_, 
                                      std::mutex& m_) {
    uint64_t local_spatial_valid_cnt = 0;
    uint64_t local_spatial_total_cnt = 0;
    uint64_t local_temporal_valid_cnt = 0;
    //std::vector<mapping_table_t> local_best_mappings;
    // Current temporal mapping table
    mapping_table_t curr_temporal_mapping(init_mapping_);
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Start temporal mapping first
    for(size_t g = range.start_g; g < range.end_g; g++) {
        curr_temporal_mapping.put_column_temporal_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g));
        for(size_t k = range.start_k; k < range.end_k; k++) {
            curr_temporal_mapping.put_column_temporal_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k));
            for(size_t b = range.start_b; b < range.end_b; b++) {
                curr_temporal_mapping.put_column_temporal_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b));
                for(size_t p = range.start_p; p < range.end_p; p++) {
                    curr_temporal_mapping.put_column_temporal_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p));
                    for(size_t q = range.start_q; q < range.end_q; q++) {
                        curr_temporal_mapping.put_column_temporal_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q));
                        for(size_t c = range.start_c; c < range.end_c; c++) {
                            curr_temporal_mapping.put_column_temporal_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c));
                            for(size_t s = range.start_s; s < range.end_s; s++) {
                                curr_temporal_mapping.put_column_temporal_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s));
                                for(size_t r = range.start_r; r < range.end_r; r++) {
                                    curr_temporal_mapping.put_column_temporal_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r));
                                    // Tempor validity check
                                    if(check_validity(component_t::L1, curr_temporal_mapping) && check_validity(component_t::L2, curr_temporal_mapping)) {
                                        local_temporal_valid_cnt++;
                                        // Spatial mapping space generation
                                        mapping_space_t spatial_mapping_space(num_spatial + 1, curr_temporal_mapping.get_row_degrees(component_t::L2));
                                        local_spatial_total_cnt += spatial_mapping_space.get_num_permutations();
                                        mapping_table_t curr_spatial_mapping(curr_temporal_mapping);
                                        // Start spatial mapping second
                                        for(size_t g = 0; g < spatial_mapping_space.get_permutations(0).size(); g++) {
                                            curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::G, spatial_mapping_space.get_permutations(0).at(g));
                                            for(size_t k = 0; k < spatial_mapping_space.get_permutations(1).size(); k++) {
                                                curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::K, spatial_mapping_space.get_permutations(1).at(k));
                                                for(size_t b = 0; b < spatial_mapping_space.get_permutations(2).size(); b++) {
                                                    curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::B, spatial_mapping_space.get_permutations(2).at(b));
                                                    for(size_t p = 0; p < spatial_mapping_space.get_permutations(3).size(); p++) {
                                                        curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::P, spatial_mapping_space.get_permutations(3).at(p));
                                                        for(size_t q = 0; q < spatial_mapping_space.get_permutations(4).size(); q++) {
                                                            curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::Q, spatial_mapping_space.get_permutations(4).at(q));
                                                            for(size_t c = 0; c < spatial_mapping_space.get_permutations(5).size(); c++) {
                                                                curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::C, spatial_mapping_space.get_permutations(5).at(c));
                                                                for(size_t s = 0; s < spatial_mapping_space.get_permutations(6).size(); s++) {
                                                                    curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::S, spatial_mapping_space.get_permutations(6).at(s));
                                                                    for(size_t r = 0; r < spatial_mapping_space.get_permutations(7).size(); r++) {
                                                                        curr_spatial_mapping.put_column_spatial_later_degrees(parameter_t::R, spatial_mapping_space.get_permutations(7).at(r));
                                                                        // Spatial validity check
                                                                        if(check_validity(component_t::S1_X, curr_spatial_mapping)) {
                                                                            local_spatial_valid_cnt++;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(local_temporal_valid_cnt > 0) {
        m_.lock();
        valid_cnt.at(0) += local_spatial_valid_cnt;
        valid_cnt.at(1) += local_temporal_valid_cnt;
        total_cnt.at(0) += local_spatial_total_cnt;
        m_.unlock();
    }
    return;
}
