#include "stats.h"

static handler_t handler;
static energy_ref_t energy_ref;
static cycle_ref_t cycle_ref;

/* Stats */
stats_t::stats_t(const accelerator_t *accelerator_, const mapping_table_t *mapping_table_)
    : accelerator(accelerator_), mapping_table(mapping_table_) {
}

stats_t::~stats_t() {
}

void stats_t::print_stats() const {
    handler.print_line(30, "*");
    std::cout << "L1 TILE SIZE (I): " << l1_input_tile_size << "\n"
              << "L1 TILE SIZE (F): " << l1_filter_tile_size << "\n"
              << "L1 TILE SIZE (O): " << l1_output_tile_size << std::endl;
    std::cout << "L2 TILE SIZE (I): " << l2_input_tile_size << "\n"
              << "L2 TILE SIZE (F): " << l2_filter_tile_size << "\n"
              << "L2 TILE SIZE (O): " << l2_output_tile_size << std::endl;
    handler.print_line(30, "*");
    std::cout << "L1 ITERATION (I): " << l1_iteration.input_rd_it << "\n"
              << "L1 ITERATION (F): " << l1_iteration.filter_rd_it << "\n"
              << "L1 ITERATION (O): " << l1_iteration.output_rd_it + l1_iteration.output_wt_it << std::endl;
    std::cout << "L2 ITERATION (I): " << l2_iteration.input_rd_it << "\n"
              << "L2 ITERATION (F): " << l2_iteration.filter_rd_it << "\n"
              << "L2 ITERATION (O): " << l2_iteration.output_rd_it + l2_iteration.output_wt_it << std::endl;
    std::cout << "DRAM ITERATION (I): " << dram_iteration.input_rd_it << "\n"
              << "DRAM ITERATION (F): " << dram_iteration.filter_rd_it << "\n"
              << "DRAM ITERATION (O): " << dram_iteration.output_rd_it + dram_iteration.output_wt_it << std::endl;
    handler.print_line(30, "*");
    std::cout << "# OF S0 HOSTS (I): " << num_s0_input_hosts << "\n"
              << "# OF S0 HOSTS (F): " << num_s0_filter_hosts << "\n"
              << "# OF S0 HOSTS (O): " << num_s0_output_hosts << std::endl;
    std::cout << "# OF S1 HOSTS (I): " << num_s1_input_hosts << "\n"
              << "# OF S1 HOSTS (F): " << num_s1_filter_hosts << "\n"
              << "# OF S1 HOSTS (O): " << num_s1_output_hosts << std::endl;
    handler.print_line(30, "*");
    std::cout << "  MAC ENERGY: " << mac_energy << " (" << float(mac_energy) / total_energy * 100 << "%)" << "\n"
              << "   L1 ENERGY: " << l1_energy << " (" << float(l1_energy) / total_energy * 100 << "%)" << "\n"
              << "   L2 ENERGY: " << l2_energy << " (" << float(l2_energy) / total_energy * 100 << "%)" << "\n"
              << " DRAM ENERGY: " << dram_energy << " (" << float(dram_energy) / total_energy * 100 << "%)" << "\n"
              << "TOTAL ENERGY: " << total_energy << std::endl;
    handler.print_line(30, "*");
    std::cout << "# OF ACTIVE PEs : " << num_active_pes << "\n"
              << "# L1 UTILIZATION: " << l1_utilization << " %" << "\n" 
              << "# S1 UTILIZATION: " << s1_utilization << " %" << "\n"
              << "# L2 UTILIZATION: " << l2_utilization << " %" << std::endl;
    handler.print_line(30, "*");
    std::cout << "#   MAC CYCLE: " << mac_cycle << " (" << float(mac_cycle) / total_cycle * 100 << "%)" << "\n"
              << "#    L1 CYCLE: " << l1_cycle << " (" << float(l1_cycle) / total_cycle * 100 << "%)" << "\n"
              << "#    L2 CYCLE: " << l2_cycle << " (" << float(l2_cycle) / total_cycle * 100 << "%)" << "\n"
              << "#  DRAM CYCLE: " << dram_cycle << " (" << float(dram_cycle) / total_cycle * 100 << "%)" << "\n"
              << "# TOTAL CYCLE: " << total_cycle << std::endl;
    handler.print_line(30, "*");
    std::cout << "# TOTAL EDP (J x CYCLE) : " << total_edp << std::endl;
    return;
}

void stats_t::print_csv() const {
    std::cout << "L1 TILE SIZE,I,F,O\n" 
              << "," << l1_input_tile_size << "," << l1_filter_tile_size << ","  << l1_output_tile_size << "\n"
              << "L2 TILE SIZE,I,F,O\n" 
              << "," << l2_input_tile_size << "," << l2_filter_tile_size << "," << l2_output_tile_size << "\n"
              << "L1 ITERATION,I,F,O\n"
              << "," << l1_iteration.input_rd_it << "," << l1_iteration.filter_rd_it << "," << l1_iteration.output_rd_it + l1_iteration.output_wt_it << "\n"
              << "L2 ITERATION,I,F,O\n"
              << "," << l2_iteration.input_rd_it << "," << l2_iteration.filter_rd_it << "," << l2_iteration.output_rd_it + l2_iteration.output_wt_it << "\n"
              << "DRAM ITERATION,I,F,O\n"
              << "," << dram_iteration.input_rd_it << "," << dram_iteration.filter_rd_it << "," << dram_iteration.output_rd_it + dram_iteration.output_wt_it << "\n"
              << "# OF S0 HOSTS,I,F,O\n" 
              << "," << num_s0_input_hosts << "," << num_s0_filter_hosts << "," << num_s0_output_hosts << "\n"
              << "# OF S1 HOSTS,I,F,O\n" 
              << "," << num_s1_input_hosts << "," << num_s1_filter_hosts << "," << num_s1_output_hosts << "\n"
              << "MAC ENERGY,L1 ENERGY,L2 ENERGY,DRAM ENERGY,TOTAL ENERGY,MAC CYCLE,L1 CYCLE,L2 CYCLE,DRAM CYCLE,TOTAL CYCLE,TOTAL EDP (JxCYCLE),L1 UTILIZATION,S1 UTILIZATION,L2 UTILIZATION\n" 
              << std::fixed << std::setprecision(1) << mac_energy << "," << std::fixed << std::setprecision(1) << l1_energy << "," << std::fixed << std::setprecision(1) << l2_energy << "," << std::fixed << std::setprecision(1) << dram_energy << "," << std::fixed << std::setprecision(1) << total_energy << ","
              << mac_cycle << "," << l1_cycle << "," << l2_cycle << "," << dram_cycle << "," << total_cycle << "," << total_edp << ","
              << l1_utilization << "," << s1_utilization << "," << l2_utilization << std::endl;
    return;
}

void stats_t::update_stats() { 
    update_tile_size();
    update_iteration();
    update_active_components();
    update_noc();
    update_energy();
    update_utilization();
    update_cycle();
    update_edp();
    return;
}

void stats_t::update_tile_size() {
    // Temporal tile sizes
    mac_input_tile_size = mapping_table->get_input_tile_size(component_t::MAC);     // Size: 1
    mac_filter_tile_size = mapping_table->get_filter_tile_size(component_t::MAC);   // Size: 1
    mac_output_tile_size = mapping_table->get_filter_tile_size(component_t::MAC);   // Size: 1
    l1_input_tile_size = mapping_table->get_input_tile_size(component_t::L1);
    l1_filter_tile_size = mapping_table->get_filter_tile_size(component_t::L1);
    l1_output_tile_size = mapping_table->get_output_tile_size(component_t::L1);
    l2_input_tile_size = mapping_table->get_input_tile_size(component_t::L2);
    l2_filter_tile_size = mapping_table->get_filter_tile_size(component_t::L2);
    l2_output_tile_size = mapping_table->get_output_tile_size(component_t::L2);
    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l1_input_tile_size = mac_input_tile_size; 
    if(accelerator->l1_filter_bypass())
        l1_filter_tile_size = mac_filter_tile_size; 
    if(accelerator->l1_output_bypass()) 
        l1_output_tile_size = mac_output_tile_size; 
    if(accelerator->l2_input_bypass())
        l2_input_tile_size = l1_input_tile_size; 
    if(accelerator->l2_filter_bypass())
        l2_filter_tile_size = l1_filter_tile_size; 
    if(accelerator->l2_output_bypass()) 
        l2_output_tile_size = l1_output_tile_size; 
}

void stats_t::update_iteration() {
    // Iteration without dataflow 
    size_t l1_iteration_tmp = mapping_table->get_iteration(component_t::L1);
    size_t l2_iteration_tmp = mapping_table->get_iteration(component_t::L2);
    size_t dram_iteration_tmp = mapping_table->get_iteration(component_t::DRAM);
    size_t h_upper = 0;
    size_t h_lower = 0;
    size_t w_upper = 0;
    size_t w_lower = 0;
    // L1 iteration with MAC dataflow
    l1_iteration.input_rd_it = l1_iteration_tmp;
    l1_iteration.filter_rd_it = l1_iteration_tmp;
    l1_iteration.output_rd_it = l1_iteration_tmp;
    l1_iteration.output_wt_it = l1_iteration_tmp;
    switch(accelerator->mac_dataflow()) {
        case dataflow_t::IS: 
            // l1_iteration.input_rd_it /= mapping_table->get_degree(parameter_t::K, component_t::L1); 
            h_upper = (mapping_table->get_product(parameter_t::P, component_t::MAC) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::MAC);
            h_lower = (mapping_table->get_product(parameter_t::P, component_t::L1) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::L1);
            w_upper = (mapping_table->get_product(parameter_t::Q, component_t::MAC) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::MAC);
            w_lower = (mapping_table->get_product(parameter_t::Q, component_t::L1) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::L1);
            l1_iteration.input_rd_it = mapping_table->get_degree(parameter_t::B, component_t::L1)
                                     * mapping_table->get_degree(parameter_t::C, component_t::L1)
                                     * ((h_lower - h_upper) / mapping_table->get_stride() + 1)
                                     * ((w_lower - w_upper) / mapping_table->get_stride() + 1);
            break;
        case dataflow_t::WS: 
            l1_iteration.filter_rd_it /= (mapping_table->get_degree(parameter_t::B, component_t::L1) 
                                          * mapping_table->get_degree(parameter_t::P, component_t::L1) 
                                          * mapping_table->get_degree(parameter_t::Q, component_t::L1));
            break;
        case dataflow_t::NONE: // Nothing to do
            break;
        default: handler.print_err(err_type_t::INVAILD, "MAC DATAFLOW"); break;
    }
    // L2 iteration with L1 dataflow 
    l2_iteration.input_rd_it = l2_iteration_tmp;
    l2_iteration.filter_rd_it = l2_iteration_tmp;
    l2_iteration.output_rd_it = l2_iteration_tmp;
    l2_iteration.output_wt_it = l2_iteration_tmp;
    switch(accelerator->l1_dataflow()) {
        case dataflow_t::IS: 
            //l2_iteration.input_rd_it /= mapping_table->get_degree(parameter_t::K, component_t::L2);
            h_upper = (mapping_table->get_product(parameter_t::P, component_t::L1) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::L1);
            h_lower = (mapping_table->get_product(parameter_t::P, component_t::L2) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::L2);
            w_upper = (mapping_table->get_product(parameter_t::Q, component_t::L1) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::L1);
            w_lower = (mapping_table->get_product(parameter_t::Q, component_t::L2) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::L2);
            l2_iteration.input_rd_it = mapping_table->get_degree(parameter_t::B, component_t::L2)
                                     * mapping_table->get_degree(parameter_t::C, component_t::L2)
                                     * ((h_lower - h_upper) / mapping_table->get_stride() + 1)
                                     * ((w_lower - w_upper) / mapping_table->get_stride() + 1);
            break;
        case dataflow_t::WS: 
            l2_iteration.filter_rd_it /= (mapping_table->get_degree(parameter_t::B, component_t::L2) 
                                          * mapping_table->get_degree(parameter_t::P, component_t::L2) 
                                          * mapping_table->get_degree(parameter_t::Q, component_t::L2));
            break;
        case dataflow_t::OS: 
            l2_iteration.output_rd_it = 0;
            l2_iteration.output_wt_it /= (mapping_table->get_degree(parameter_t::C, component_t::L2) 
                                          * mapping_table->get_degree(parameter_t::S, component_t::L2) 
                                          * mapping_table->get_degree(parameter_t::R, component_t::L2));
            break;
        default: handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW"); break;
    }
    // DRAM iteration with L2 dataflow 
    dram_iteration.input_rd_it = dram_iteration_tmp;
    dram_iteration.filter_rd_it = dram_iteration_tmp;
    dram_iteration.output_rd_it = dram_iteration_tmp;
    dram_iteration.output_wt_it = dram_iteration_tmp;
    switch(accelerator->l2_dataflow()) {
        case dataflow_t::IS: 
            //dram_iteration.input_rd_it /= mapping_table->get_degree(parameter_t::K, component_t::DRAM);
            h_upper = (mapping_table->get_product(parameter_t::P, component_t::L2) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::L2);
            h_lower = (mapping_table->get_product(parameter_t::P, component_t::DRAM) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::S, component_t::DRAM);
            w_upper = (mapping_table->get_product(parameter_t::Q, component_t::L2) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::L2);
            w_lower = (mapping_table->get_product(parameter_t::Q, component_t::DRAM) - 1) * mapping_table->get_stride() + mapping_table->get_product(parameter_t::R, component_t::DRAM);
            dram_iteration.input_rd_it = mapping_table->get_degree(parameter_t::B, component_t::DRAM)
                                       * mapping_table->get_degree(parameter_t::C, component_t::DRAM)
                                       * ((h_lower - h_upper) / mapping_table->get_stride() + 1)
                                       * ((w_lower - w_upper) / mapping_table->get_stride() + 1);
            break;
        case dataflow_t::WS: 
            dram_iteration.filter_rd_it /= (mapping_table->get_degree(parameter_t::B, component_t::DRAM) 
                                            * mapping_table->get_degree(parameter_t::P, component_t::DRAM) 
                                            * mapping_table->get_degree(parameter_t::Q, component_t::DRAM));
            break;
        case dataflow_t::OS: 
            dram_iteration.output_rd_it = 0;
            dram_iteration.output_wt_it /= (mapping_table->get_degree(parameter_t::C, component_t::DRAM) 
                                            * mapping_table->get_degree(parameter_t::S, component_t::DRAM) 
                                            * mapping_table->get_degree(parameter_t::R, component_t::DRAM));
            break;
        default: handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW"); break;
    }
    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l2_iteration.input_rd_it = l1_iteration.input_rd_it;
    if(accelerator->l1_filter_bypass())
        l2_iteration.filter_rd_it = l1_iteration.filter_rd_it;
    if(accelerator->l1_output_bypass()) {
        l2_iteration.output_rd_it = l1_iteration.output_rd_it;
        l2_iteration.output_wt_it = l1_iteration.output_wt_it;
    }
    if(accelerator->l2_input_bypass())
        dram_iteration.input_rd_it = l2_iteration.input_rd_it;
    if(accelerator->l2_filter_bypass())
        dram_iteration.filter_rd_it = l2_iteration.filter_rd_it;
    if(accelerator->l2_output_bypass()) {
        dram_iteration.output_rd_it = l2_iteration.output_rd_it;
        dram_iteration.output_wt_it = l2_iteration.output_wt_it;
    }
    return;
}

void stats_t::update_active_components() {
    num_active_macs = mapping_table->get_degree(parameter_t::K, component_t::S0)
                    * mapping_table->get_degree(parameter_t::B, component_t::S0)
                    * mapping_table->get_degree(parameter_t::P, component_t::S0)
                    * mapping_table->get_degree(parameter_t::Q, component_t::S0)
                    * mapping_table->get_degree(parameter_t::C, component_t::S0)
                    * mapping_table->get_degree(parameter_t::S, component_t::S0)
                    * mapping_table->get_degree(parameter_t::R, component_t::S0);
    num_active_pes = mapping_table->get_degree(parameter_t::K, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::B, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::P, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::Q, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::C, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::S, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::R, component_t::S1_X)
                   * mapping_table->get_degree(parameter_t::K, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::B, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::P, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::Q, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::C, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::S, component_t::S1_Y)
                   * mapping_table->get_degree(parameter_t::R, component_t::S1_Y);
    num_active_accelerators = mapping_table->get_degree(parameter_t::K, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::B, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::P, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::Q, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::C, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::S, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::R, component_t::S2_X)
                            * mapping_table->get_degree(parameter_t::K, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::B, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::P, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::Q, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::C, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::S, component_t::S2_Y)
                            * mapping_table->get_degree(parameter_t::R, component_t::S2_Y);
}

void stats_t::update_noc() {
    // S0
    num_s0_input_hosts = mapping_table->get_degree(parameter_t::B, component_t::S0)
                       * mapping_table->get_degree(parameter_t::C, component_t::S0);
    
    size_t P_s0 = mapping_table->get_degree(parameter_t::P, component_t::S0);
    size_t S_s0 = mapping_table->get_degree(parameter_t::S, component_t::S0);
    size_t Q_s0 = mapping_table->get_degree(parameter_t::Q, component_t::S0);
    size_t R_s0 = mapping_table->get_degree(parameter_t::R, component_t::S0);

    if(P_s0 > 1 && S_s0 > 1) 
        num_s0_input_hosts *= ((P_s0 - 1) * mapping_table->get_stride() + S_s0);
    else 
        num_s0_input_hosts *= (P_s0 * S_s0);

    if(Q_s0 > 1 && R_s0 > 1) 
        num_s0_input_hosts *= ((Q_s0 - 1) * mapping_table->get_stride() + R_s0);
    else 
        num_s0_input_hosts *= (Q_s0 * R_s0);

    num_s0_filter_hosts = mapping_table->get_degree(parameter_t::K, component_t::S0)
                        * mapping_table->get_degree(parameter_t::C, component_t::S0)
                        * mapping_table->get_degree(parameter_t::S, component_t::S0)
                        * mapping_table->get_degree(parameter_t::R, component_t::S0);
    num_s0_output_hosts = mapping_table->get_degree(parameter_t::K, component_t::S0)
                        * mapping_table->get_degree(parameter_t::B, component_t::S0)
                        * mapping_table->get_degree(parameter_t::P, component_t::S0)
                        * mapping_table->get_degree(parameter_t::Q, component_t::S0);
    // S1_X & S1_Y
    num_s1_input_hosts = mapping_table->get_degree(parameter_t::B, component_t::S1_X)
                       * mapping_table->get_degree(parameter_t::C, component_t::S1_X)
                       * mapping_table->get_degree(parameter_t::B, component_t::S1_Y)
                       * mapping_table->get_degree(parameter_t::C, component_t::S1_Y);
    
    size_t P_s1 = mapping_table->get_degree(parameter_t::P, component_t::S1_X)
                * mapping_table->get_degree(parameter_t::P, component_t::S1_Y);
    size_t S_s1 = mapping_table->get_degree(parameter_t::S, component_t::S1_X)
                * mapping_table->get_degree(parameter_t::S, component_t::S1_Y);
    size_t Q_s1 = mapping_table->get_degree(parameter_t::Q, component_t::S1_X)
                * mapping_table->get_degree(parameter_t::Q, component_t::S1_Y);
    size_t R_s1 = mapping_table->get_degree(parameter_t::R, component_t::S1_X)
                * mapping_table->get_degree(parameter_t::R, component_t::S1_Y);
    
    if(P_s1 > 1 && S_s1 > 1) 
        num_s1_input_hosts *= ((P_s1 - 1) * mapping_table->get_stride() + S_s1);
    else 
        num_s1_input_hosts *= (P_s1 * S_s1);

    if(Q_s1 > 1 && R_s1 > 1) 
        num_s1_input_hosts *= ((Q_s1 - 1) * mapping_table->get_stride() + R_s1);
    else 
        num_s1_input_hosts *= (Q_s1 * R_s1);

    num_s1_filter_hosts = mapping_table->get_degree(parameter_t::K, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::C, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::S, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::R, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::K, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::C, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::S, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::R, component_t::S1_Y);
    num_s1_output_hosts = mapping_table->get_degree(parameter_t::K, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::B, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::P, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::Q, component_t::S1_X)
                        * mapping_table->get_degree(parameter_t::K, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::B, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::P, component_t::S1_Y)
                        * mapping_table->get_degree(parameter_t::Q, component_t::S1_Y);
    // TODO: S2_X & S2_Y
}

void stats_t::update_energy() {
    // Between MAC and L1 with 'l1 iteration' and S0 NoC
    mac_energy = mapping_table->get_num_macs() * energy_ref.mac_operation;
    l1_energy = mac_input_tile_size * l1_iteration.input_rd_it * num_s0_input_hosts * energy_ref.l1_input_egress
              + mac_filter_tile_size * l1_iteration.filter_rd_it * num_s0_filter_hosts * energy_ref.l1_filter_egress
              + mac_output_tile_size * l1_iteration.output_rd_it * num_s0_output_hosts * energy_ref.l1_output_egress
              + mac_output_tile_size * l1_iteration.output_wt_it * num_s0_output_hosts * energy_ref.l1_output_ingress;
    // Between L1 and L2 with 'l2_iteration' and S1 NoC
    l1_energy += l1_input_tile_size * l2_iteration.input_rd_it * energy_ref.l1_input_ingress 
              + l1_filter_tile_size * l2_iteration.filter_rd_it * energy_ref.l1_filter_ingress
              + l1_output_tile_size * l2_iteration.output_rd_it * energy_ref.l1_output_ingress
              + l1_output_tile_size * l2_iteration.output_wt_it * energy_ref.l1_output_egress;
    l1_energy *= num_active_pes;
    l2_energy = l1_input_tile_size * l2_iteration.input_rd_it * num_s1_input_hosts * energy_ref.l2_input_egress 
              + l1_filter_tile_size * l2_iteration.filter_rd_it * num_s1_filter_hosts * energy_ref.l2_filter_egress
              + l1_output_tile_size * l2_iteration.output_rd_it * num_s1_output_hosts * energy_ref.l2_output_egress
              + l1_output_tile_size * l2_iteration.output_wt_it * num_s1_output_hosts * energy_ref.l2_output_ingress; 
    // Between L2 and DRAM with 'dram_iteration' and S2 NoC
    l2_energy += l2_input_tile_size * dram_iteration.input_rd_it * energy_ref.l2_input_ingress
               + l2_filter_tile_size * dram_iteration.filter_rd_it * energy_ref.l2_filter_ingress
               + l2_output_tile_size * dram_iteration.output_rd_it * energy_ref.l2_output_ingress
               + l2_output_tile_size * dram_iteration.output_wt_it * energy_ref.l2_output_egress;
    dram_energy = l2_input_tile_size * dram_iteration.input_rd_it * energy_ref.dram_egress
                + l2_filter_tile_size * dram_iteration.filter_rd_it * energy_ref.dram_egress
                + l2_output_tile_size * dram_iteration.output_rd_it * energy_ref.dram_egress
                + l2_output_tile_size * dram_iteration.output_wt_it * energy_ref.dram_ingress;
    // TODO: broadcast (or mesh) energy & reduction energy
    s0_noc_energy = 0; 
    s1_noc_energy = 0;
    s2_noc_energy = 0;
    total_energy = mac_energy + l1_energy + l2_energy + dram_energy + s0_noc_energy + s1_noc_energy + s2_noc_energy;
    return;
}

void stats_t::update_utilization() {
    //s0_utilization = 
    l1_utilization = float(l1_input_tile_size + l1_filter_tile_size + l1_output_tile_size) 
                   / (accelerator->l1_input_size() + accelerator->l1_filter_size() + accelerator->l1_output_size() + accelerator->l1_shared_size()) * 100;
    s1_utilization = float(num_active_pes) / (accelerator->s1_size_x() * accelerator->s1_size_y()) * 100;
    l2_utilization = float(l2_input_tile_size + l2_filter_tile_size + l2_output_tile_size) 
                   / (accelerator->l2_input_size() + accelerator->l2_filter_size() + accelerator->l2_output_size() + accelerator->l2_shared_size()) * 100;
    //s2_utilization =
    return;
}

void stats_t::update_cycle() {
    mac_cycle = mapping_table->get_iteration(component_t::L1) * cycle_ref.mac_operation;
    l1_cycle = (l1_iteration.input_rd_it + l1_iteration.filter_rd_it + l1_iteration.output_rd_it + l1_iteration.output_wt_it) * cycle_ref.l1_access;
    l2_cycle = (l2_iteration.input_rd_it + l2_iteration.filter_rd_it + l2_iteration.output_rd_it + l2_iteration.output_wt_it) * cycle_ref.l2_access;
    dram_cycle = (dram_iteration.input_rd_it + dram_iteration.filter_rd_it + dram_iteration.output_rd_it + dram_iteration.output_wt_it) * cycle_ref.dram_access;
    // Separated buffer adjustment
    if(accelerator->l1_type() == buffer_type_t::SEPARATED) {
        if(l1_iteration.input_rd_it >= l1_iteration.filter_rd_it)
            l1_cycle -= l1_iteration.filter_rd_it * cycle_ref.l1_access;
        else
            l1_cycle -= l1_iteration.input_rd_it * cycle_ref.l1_access;
    }
    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l2_cycle -= l2_iteration.input_rd_it * cycle_ref.l2_access;
    if(accelerator->l1_filter_bypass())
        l2_cycle -= l2_iteration.filter_rd_it * cycle_ref.l2_access;
    if(accelerator->l1_output_bypass()) 
        l2_cycle -= (l2_iteration.output_rd_it + l2_iteration.output_wt_it) * cycle_ref.l2_access;
    if(accelerator->l2_input_bypass())
        dram_cycle -= dram_iteration.input_rd_it * cycle_ref.dram_access;
    if(accelerator->l2_filter_bypass())
        dram_cycle -= dram_iteration.filter_rd_it * cycle_ref.dram_access;
    if(accelerator->l2_output_bypass()) 
        dram_cycle -= (dram_iteration.output_rd_it + dram_iteration.output_wt_it) * cycle_ref.dram_access;
    total_cycle = mac_cycle + l1_cycle + l2_cycle + dram_cycle;
    return;
}

void stats_t::update_edp() {
    total_edp = total_energy / 1000000000000; // pJ -> J
    total_edp *= total_cycle;
}

/* Energy reference */
energy_ref_t::energy_ref_t() {
}

energy_ref_t::~energy_ref_t() {
}

/* Cycle reference */
cycle_ref_t::cycle_ref_t() {
}

cycle_ref_t::~cycle_ref_t() {
}
