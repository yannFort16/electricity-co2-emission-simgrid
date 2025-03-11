#ifndef HOST_ENERGY_H
#define HOST_ENERGY_H

#include <simgrid/s4u/Host.hpp>

#ifdef __cplusplus
extern "C" {
#endif

void sg_host_energy_plugin_init();
void sg_host_energy_update_all();

double sg_host_get_consumed_energy(const simgrid::s4u::Host* host);
double sg_host_get_idle_consumption(const simgrid::s4u::Host* host);
double sg_host_get_idle_consumption_at(const simgrid::s4u::Host* host, int pstate);
double sg_host_get_wattmin_at(const simgrid::s4u::Host* host, int pstate);
double sg_host_get_wattmax_at(const simgrid::s4u::Host* host, int pstate);
double sg_host_get_power_range_slope_at(const simgrid::s4u::Host* host, int pstate);
double sg_host_get_current_consumption(const simgrid::s4u::Host* host);

#ifdef __cplusplus
}
#endif

#endif // HOST_ENERGY_H
