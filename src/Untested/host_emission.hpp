#ifndef HOST_EMISSION_H
#define HOST_EMISSION_H

#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <xbt/log.h>
#include <xbt/log.hpp>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the host emission plugin
void sg_host_emission_plugin_init();
void sg_host_emission_update_all();

// Get the CO2 emission of a host
double sg_host_get_emission(const simgrid::s4u::Host* host);

#ifdef __cplusplus
}
#endif

namespace simgrid::plugin {

class HostEmissions {
private:
    simgrid::s4u::Host* host_ = nullptr;
    static constexpr int EMISSION_FR = 42; // g CO2/kWh in France on 11/03/2025
    double total_emissions_ = 0.0;
    double last_updated_ = simgrid::s4u::Engine::get_clock();
    
    double JouleToWattH(double energy) { return energy / 3600.0; }

public:
    static simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> EXTENSION_ID;

    explicit HostEmissions(simgrid::s4u::Host* host);
    ~HostEmissions();

    double get_emission();
    double get_last_update_time() const { return last_updated_; }
    void update();
};

} // namespace simgrid::plugin

#endif // HOST_EMISSION_H
