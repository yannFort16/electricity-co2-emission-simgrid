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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host_energy, kernel, "Logging specific to the host energy plugin");

static void on_simulation_end();

namespace simgrid::plugin {

class HostEmissions {
    simgrid::s4u::Host* host_ = nullptr;
    const int EMISSION_FR = 42; // g CO2/kWh in France on the 11/03/2025
    double total_emissions_  = 0.0;
    double last_updated_  = simgrid::s4u::Engine::get_clock(); /*< Timestamp of the last energy update event*/
    bool host_was_used_ = false;

    friend void ::on_simulation_end(); // For access to host_was_used_
    double JouleToWattH(double energy) { return energy / 3600.0; }
public:    
    static simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> EXTENSION_ID;
    explicit HostEmissions(simgrid::s4u::Host* ptr) : host_(ptr) {}
    explicit HostEmissions(simgrid::s4u::Host* ptr);
    ~HostEmissions();
    double get_emission();
    double get_last_update_time() const { return last_updated_; }
    void update();  
};

    static simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> EXTENSION_ID;
    
    double get_emission(simgrid::s4u::Host* host){
            //ToDo
            double energy = sg_host_get_consumed_energy(host);
        
            return 0;
        }

void HostEmissions::update() {
    double start_time  = last_updated_;
    double finish_time = simgrid::s4u::Engine::get_clock();
    //
    // We may have start == finish if the past consumption was updated since the simcall was started
    // for example if 2 actors requested to update the same host's consumption in a given scheduling round.
    //
    // Even in this case, we need to save the pstate for the next call (after this if),
    // which may have changed since that recent update.
    if (start_time < finish_time) {
      double previous_emissions = total_emissions_;
      
      double power_consumption = sg_host_get_current_consumption(host_);

      double emissions_this_step = JouleToWattH(power_consumption) * EMISSION_FR;
          
      // TODO Trace: Trace energy_this_step from start_time to finish_time in host->getName()
          
      total_emissions_ = previous_emissions + emissions_this_step;
      last_updated_ = finish_time;
          
      XBT_DEBUG("[update_emission of %s] period=[%.8f-%.8f]; total emission before: %.8f gCO2 -> added now: %.8f gCO2",
        host_->get_cname(), start_time, finish_time, previous_emissions, emissions_this_step);
    }
          
}

HostEmissions::~HostEmissions() = default;

double HostEmissions::get_emission() {
    //TODO
}

}

using simgrid::plugin::HostEmissions;
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

    XBT_INFO("Emission of C02 of host %s: %f g", host.get_cname(), host.extension<HostEmissions>()->get_emission());
}


static void on_simulation_end()
{
  double total_energy      = 0.0; // Total energy consumption (whole platform)
  //double used_hosts_energy = 0.0; // Energy consumed by hosts that computed something
  for (simgrid::s4u::Host const* host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    if (host && dynamic_cast<const simgrid::s4u::VirtualMachine*>(host) == nullptr) { // Ignore virtual machines
      double energy = host->extension<HostEmissions>()->get_emission();
      total_energy += energy;
      /*if (host->extension<HostEmissions>()->host_was_used_)
        used_hosts_energy += energy;*/
    }
  }
  XBT_INFO("Total energy consumption: %f Joules ", total_energy);//(used hosts: %f Joules; unused/idle hosts: %f) used_hosts_energy, total_energy - used_hosts_energy
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

void sg_host_emission_update_all()
{
  simgrid::kernel::actor::simcall_answered([]() {
    std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::get_instance()->get_all_hosts();
    for (auto const& host : list)
      if (dynamic_cast<simgrid::s4u::VirtualMachine*>(host) == nullptr) { // Ignore virtual machines
        xbt_assert(host != nullptr);
        host->extension<HostEmissions>()->update();
      }
  });
}

static void ensure_plugin_inited()
{
  if (not HostEmissions::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError("The Emission plugin is not active. Please call sg_host_emission_plugin_init() "
                                            "before calling any function related to that plugin.");
}

void sg_host_get_emission(const_sg_host_t host) {
    ensure_plugin_inited();

    //TODO
}
