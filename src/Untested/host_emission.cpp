#include "host_energy.h"
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/energy.h>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <simgrid/simcall.hpp>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <xbt/log.h>
#include <xbt/log.hpp>


//#include <simgrid/s4u.hpp>
#include <iostream>

SIMGRID_REGISTER_PLUGIN(host_emission, "Cpu CO2 emisson besed on energy consumption.", &sg_host_emission_plugin_init)

class HostEmissions{
    const int EMISSION_FR = 42; // g CO2/kWh in France on the 11/03/2025

    public:
    static simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> EXTENSION_ID;
        static double host_get_emission(simgrid::s4u::Host* host){
            //ToDo
            double energy = sg_host_get_consumed_energy(host);
        
            return 0;
        }

        void update() {
            // Implementation of the update method
            // ToDo: Add the logic to update emissions
        }
};

/* **************************** events  callback *************************** */
extern void (*on_creation_ptr)(simgrid::s4u::Host*);
static void on_creation(simgrid::s4u::Host& host)
{
    on_creation_ptr(&host);
}

extern void (*on_action_state_change_ptr)(simgrid::kernel::resource::CpuAction const&, simgrid::kernel::resource::Action::State);
static void on_action_state_change(simgrid::kernel::resource::CpuAction const& action,
                                   simgrid::kernel::resource::Action::State /*previous*/)
{
    on_action_state_change_ptr(action, /*ignored*/ action.get_state());
}

extern void (*on_host_change_ptr)(simgrid::s4u::Host const&);
/* This callback is fired either when the host changes its state (on/off) ("onStateChange") or its speed
 * (because the user changed the pstate, or because of external trace events) ("onSpeedChange") */
static void on_host_change(simgrid::s4u::Host const& h)
{
  on_host_change_ptr(h);
}

static void on_host_destruction(simgrid::s4u::Host const& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // Ignore virtual machines
    return;

    XBT_INFO("Emission of C02 of host %s: %f g", host.get_cname(), host.extension<HostEmissions>()->host_get_emission());
}


static void on_simulation_end()
{
  double total_energy      = 0.0; // Total energy consumption (whole platform)
  double used_hosts_energy = 0.0; // Energy consumed by hosts that computed something
  for (simgrid::s4u::Host const* host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    if (host && dynamic_cast<const simgrid::s4u::VirtualMachine*>(host) == nullptr) { // Ignore virtual machines
      double energy = host->extension<HostEmissions>()->host_get_emission();
      total_energy += energy;
      if (host->extension<HostEmissions>()->host_was_used_)
        used_hosts_energy += energy;
    }
  }
  XBT_INFO("Total energy consumption: %f Joules (used hosts: %f Joules; unused/idle hosts: %f)", total_energy,
           used_hosts_energy, total_energy - used_hosts_energy);
}

extern void (*on_activity_suspend_resume_ptr)(simgrid::s4u::Activity const&);
static void on_activity_suspend_resume(simgrid::s4u::Activity const& activity)
{
    on_activity_suspend_resume_ptr(activity);
}


/* **************************** Public interface *************************** */

/** @ingroup plugin_host_energy
 * @brief Enable host energy plugin
 * @details Enable energy plugin to get joules consumption of each cpu. Call this function before loading your platform.
 */

void sg_host_emission_plugin_init()
{
    if (HostEmissions::EXTENSION_ID.valid())
        return;

    HostEmissions::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostEmissions>();
    sg_host_energy_plugin_init();
}

static void ensure_plugin_inited()
{
  if (not HostEmissions::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError("The Emission plugin is not active. Please call sg_host_emission_plugin_init() "
                                            "before calling any function related to that plugin.");
}

void sg_host_get_emission() {
    ensure_plugin_inited();
}
