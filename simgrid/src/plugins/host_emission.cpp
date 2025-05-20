#include <simgrid/Exception.hpp>
#include <simgrid/plugins/emission.h>
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

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <list>
#include <ctime>

SIMGRID_REGISTER_PLUGIN(host_emission, "Cpu CO2 emisson besed on energy consumption.", &sg_host_emission_plugin_init)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host_emission, kernel, "Logging specific to the host emission plugin");

static void on_simulation_end();

namespace simgrid::plugin {


class HostEmissions {
    simgrid::s4u::Host* host_ = nullptr;

    double emission_value = 42.0; // g CO2/kWh in France on the 11/03/2025 used as default value
    double total_emissions_  = 0.0;
    double total_conso_ = 0.0; // Total energy consumption in KWh
    std::list<double> total_emissions_list_ = {}; 
    double last_updated_  = simgrid::s4u::Engine::get_clock(); /*< Timestamp of the last energy update event*/
    time_t last_update_time_ = time(nullptr); // Last update time in seconds since epoch
    std::string country = "No data";
    int type_of_csv =-1; // -1: No CSV, 0: Monthly, 1: Daily, 2: Hourly, 3: Yearly
    std::list<double> list_emission_value = {}; // List of emission values
    bool host_was_used_ = false; // Tracks whether the host was used
    std::list<std::string> date_utc_list = {}; // List of dates in UTC format

    std::list<std::list<double>> emission_export_list = {}; // List of emissions for export

    friend void ::on_simulation_end(); // For access to host_was_used_
    double JouleTokWattH(double energy) { return (energy / 3600.0)*0.001; }

  public:    
    static simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> EXTENSION_ID;
    explicit HostEmissions(simgrid::s4u::Host* ptr);
    ~HostEmissions();
    
    double get_emission();
    double get_last_update_time() const { return last_updated_; }
    void update(); 
    void setCO2(int newCO2){emission_value = newCO2;};
    double getCO2(){return emission_value;};
    std::string getCountry(){return country;};
    void export_emission_list();
private :
    double read_emission_file(std::filesystem::path emission_file);
    int set_emission_file_type(std::string emission_file);
    //void set_export_list_type(std::string export_type_str);
    std::list<double> fill_emission_list(double last_value, int type_of_csv = -1);
    //std::list<double> HostEmissions::init_emission_export_list();
    void print_emission_list(std::list<double> list_double);
    int get_index_time(double time);
    void add_emission_to_list(double emission, double start_time, double current_time);
    void add_emission_list_to_export();
    std::string get_CSV_Type();
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostEmissions> HostEmissions::EXTENSION_ID;

void HostEmissions::print_emission_list(std::list<double> list_double){
  std::cout << "Emission list: ";
  for (const auto& value : list_double) {
      std::cout << value << " ";
  }
  std::cout << std::endl;
}

double HostEmissions::read_emission_file(std::filesystem::path emission_file){
  //Read the emission file and set the emission_value variable
  
  double newEmissionValue = -1.0;
  std::ifstream file(emission_file);
  if (!file.is_open()) {
    XBT_INFO("Could not open CSV file %s/%s", std::filesystem::current_path().string().c_str(), emission_file.string().c_str());
    return newEmissionValue;

  }
    
  std::string line;
  std::getline(file, line); // Skip header line

  while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string token;

      std::getline(ss, token, ','); // Skip datetime
      date_utc_list.push_back(token);
      std::getline(ss, token, ','); // Get country
      country = std::string(token);
      std::getline(ss, token, ','); // Skip ZoneName
      std::getline(ss, token, ','); // Skip ZoneId
      std::getline(ss, token, ','); // Get gCO2/kWh
      try {
        token.erase(0, token.find_first_not_of(" \t\n\r")); // Trim leading whitespace
        token.erase(token.find_last_not_of(" \t\n\r") + 1); // Trim trailing whitespace
        newEmissionValue = std::stod(token);
      } catch (const std::invalid_argument& e) {
        XBT_INFO("Invalid emission value in CSV file: '%s'", token.c_str());
        return newEmissionValue;
      }
      list_emission_value.push_back(newEmissionValue);
      // Read until the end of the line
      while (std::getline(ss, token, '\n')) {
        // Skip all remaining columns
      }
  }
  file.close();
  return newEmissionValue;
}

std::list<double> HostEmissions::fill_emission_list(double last_value, int type_of_csv) {
    std::list<double> new_list;
    int target_size = 0;
    if (type_of_csv == 0) target_size = 12;
    else if (type_of_csv == 1) target_size = 366;
    else if (type_of_csv == 2) target_size = 24;
    else if (type_of_csv == 3) target_size = 1;
    else return new_list;

    for (int i = 0; i < target_size; ++i) {
        new_list.push_back(last_value);
    }
    return new_list;
}

/*
std::list<double> HostEmissions::init_emission_export_list(){
  std::list<double> current_list = {};
  // Handle all combinations of export_type and type_of_csv
  // For simplicity, only the structure is created; you may want to store or use it as needed

  // Monthly export
  if (export_type == 0) {
    if (type_of_csv == 0) { // Monthly CSV
      current_list = fill_emission_list(0.0, 0); // 12 months
    } else if (type_of_csv == 1) { // Daily CSV
      // 12 months, each with 31 days (max)
      // Not implemented: would require list<list<double>>
      return {};
    } else if (type_of_csv == 2) { // Hourly CSV
      // 12 months, each with 31 days, each with 24 hours
      // Not implemented: would require list<list<list<double>>>
      return {};
    }
  }

  // Daily export
  if (export_type == 1) {
    if (type_of_csv == 0) { // Monthly CSV
      // 365 days, each with 1 value (from monthly)
      // Not implemented: would require mapping months to days
      return {};
    } else if (type_of_csv == 1) { // Daily CSV
      current_list = fill_emission_list(0.0, 1); // 366 days
    } else if (type_of_csv == 2) { // Hourly CSV
      // 366 days, each with 24 hours
      // Not implemented: would require list<list<double>>
      return {};
    }
  }

  // Hourly export
  if (export_type == 2) {
    if (type_of_csv == 0) { // Monthly CSV
      // 24 hours, each with 1 value (from monthly)
      // Not implemented: would require mapping months to hours
      return {};
    } else if (type_of_csv == 1) { // Daily CSV
      // 24 hours, each with 1 value (from daily)
      // Not implemented: would require mapping days to hours
      return {};
    } else if (type_of_csv == 2) { // Hourly CSV
      current_list = fill_emission_list(0.0, 2); // 24 hours
    }
  }

  // Yearly export
  if (export_type == 3) {
    if (type_of_csv == 0) { // Monthly CSV
      // 1 year, 12 months
      current_list = fill_emission_list(0.0, 0);
    } else if (type_of_csv == 1) { // Daily CSV
      // 1 year, 366 days
      current_list = fill_emission_list(0.0, 1);
    } else if (type_of_csv == 2) { // Hourly CSV
      // 1 year, 365 days, each with 24 hours
      std::list<std::list<double>> yearly_hourly_emissions;
      for (int day = 0; day < 365; ++day) {
        yearly_hourly_emissions.push_back(fill_emission_list(0.0, 2)); // 24 hours per day
      }
      // Not returning this structure, just as example
      return {};
    }
  }
}
*/

//TODO : Hide prints
int HostEmissions::set_emission_file_type(std::string emission_file){
  if (emission_file.find("monthly") != std::string::npos){
    type_of_csv = 0;
    XBT_INFO("CSV file Monthly: '%s'", emission_file.c_str());
  }else if (emission_file.find("daily") != std::string::npos){
    type_of_csv = 1;
    XBT_INFO("CSV file Daily: '%s'", emission_file.c_str());
  }else if (emission_file.find("hourly") != std::string::npos){
    type_of_csv = 2;
    XBT_INFO("CSV file Hourly: '%s'", emission_file.c_str());
  }else if (emission_file.find("yearly") != std::string::npos){
    type_of_csv = 3;
    XBT_INFO("CSV file Yearly: '%s'", emission_file.c_str());
  }
  return 0;
}

/*
void HostEmissions::set_export_list_type(std::string export_type_str){
  if (export_type_str == "yearly") {
    export_type = 3;
  } else if (export_type_str == "monthly") {
    export_type = 0;
  } else if (export_type_str == "daily") {
    export_type = 1;
  } else if (export_type_str == "hourly") {
    export_type = 2;
  } else {
    XBT_INFO("Invalid export type: %s. Defaulting to monthly.", export_type_str.c_str());
    export_type = 0; // Default to monthly
  }
}
*/

std::string HostEmissions::get_CSV_Type(){
  if (type_of_csv == 0) return "m";
  else if (type_of_csv == 1) return "d";
  else if (type_of_csv == 2) return "h";
  else if (type_of_csv == 3) return "y";
  else return "unknown";
}

void HostEmissions::export_emission_list(){
  //print_emission_list(total_emissions_list_);
  add_emission_list_to_export();
  // Export the emission_export_list to a CSV file
  std::string export_type_str = get_CSV_Type();
  std::string filename = "emission_export_list_" + host_->get_name() + ".csv";
  std::ofstream outfile(filename);
  if (!outfile.is_open()) {
    XBT_ERROR("Could not open file %s for writing emission export list.", filename.c_str());
    return;
  }
  outfile << "ExportIndex,Index,Emission (gCO2)\n";
  auto it = emission_export_list.begin();
  std::advance(it, 0);
  for (int i = 0; i < emission_export_list.size(); ++i) {
    std::list<double> list_emission_values = *it;
    auto it_nested = list_emission_values.begin();
    std::advance(it_nested, 0);
    for (int j = 0; j < list_emission_values.size(); ++j) {
      outfile << i << "," << j+1 << export_type_str << "," << (*it_nested) << "\n"; //TODO :  day, month, year
      ++it_nested;
    }
    ++it;
  }

  /*
  int export_idx = 0;

  for (const auto& emission_list : emission_export_list) {
    int idx = 0;
    for (const auto& emission : emission_list) {
      outfile << export_idx << "," << idx << "," << emission << "\n";
      ++idx;
    }
    ++export_idx;
  }*/
  outfile.close();
  XBT_INFO("Emission export list exported to %s", filename.c_str());
}

int HostEmissions::get_index_time(double current_time){
  struct tm date_time;
  time_t sim_time = static_cast<time_t>(current_time); // Convert simulation time to time_t
  localtime_r(&sim_time, &date_time);
  //struct tm date_time = *localtime(&last_update_time_);
  if (type_of_csv == 0){
    // Monthly
    return date_time.tm_mon;
  } else if (type_of_csv == 1){
    // Daily
    return date_time.tm_yday;
  } else if (type_of_csv == 2){
    // Hourly
    return date_time.tm_hour;
  } else if (type_of_csv == 3){
    // Yearly
    return 0; // Only one value per year
  }
  return -1; // Invalid type
}

void HostEmissions::add_emission_list_to_export(){
  //Add the emission list to the export list
  std::list<double> current_list = {};
  for (const auto& emission : total_emissions_list_) {
    current_list.push_back(emission);
  }
  //std::cout << "Current list :" << std::endl;
  //print_emission_list(current_list);
  emission_export_list.push_back(current_list);
  //std::cout << "Emission list is full!!!!!!!\n" << std::endl;
  //print_emission_list(total_emissions_list_);
  //XBT_INFO("Total Emission list is full. It will be added to the result CSV file at the end");
  //Clear the total emissions list
  total_emissions_list_.clear();
  total_emissions_list_ = fill_emission_list(0.0, type_of_csv);
}

void HostEmissions::add_emission_to_list(double conso_this_step, double start_time, double current_time) {
    int N = static_cast<int>(total_emissions_list_.size());
    
    if (N == 0 || start_time >= current_time)
        return;
    // Total duration
    double total_duration = current_time - start_time;
    if (total_duration < 0)
        return;
    //std::cout << "Total duration :" << total_duration << "s"<< std::endl;
    // For each bin, compute its time window and overlap with [start_time, current_time]
    double nb_lists  = 0;
    if (type_of_csv == 0) nb_lists = total_duration / (30 * 86400 * 24); // Monthly
    else if (type_of_csv == 1) nb_lists = total_duration / (86400 * 24); // Daily
    else if (type_of_csv == 2) nb_lists = total_duration / (3600 * 24); // Hourly
    else if (type_of_csv == 3) nb_lists = total_duration / (365 * 86400 * 24); // Yearly
    else return;
    int nb_lists_int = std::floor(nb_lists);
    std::cout << "Nb de list:" <<nb_lists_int << std::endl;

    double conso_per_step = conso_this_step / (current_time - start_time + 1);
    int start_index = get_index_time(start_time);
    int end_index = get_index_time(current_time);
    for(int j=0; j<nb_lists_int; ++j){
      auto it = total_emissions_list_.begin();
      std::advance(it, start_index);

      auto emission_it = list_emission_value.begin();
      std::advance(emission_it, start_index);
      for (int i = 0; i < N; ++i) {
        *it += conso_per_step * (*emission_it);
        ++it;
        ++emission_it;
      }
      start_index = 0;
      add_emission_list_to_export();
    }
    auto it = total_emissions_list_.begin();
    std::advance(it, start_index);

    auto emission_it = list_emission_value.begin();
    std::advance(emission_it, start_index);
    for (int i = 0; i <= end_index; ++i) {
      *it += conso_per_step * (*emission_it);
      ++it;
      ++emission_it;
    }
}


void HostEmissions::update() {
  double start_time  = last_updated_;
  double finish_time = simgrid::s4u::Engine::get_clock();
  if (start_time < finish_time) {
    double previous_conso = total_conso_;
    total_conso_ = JouleTokWattH(sg_host_get_consumed_energy(host_));
    double previous_emissions = total_emissions_;
    total_emissions_ = total_conso_ * emission_value;
    
    double emissions_this_step = total_emissions_ - previous_emissions;
    double energy_this_step = total_conso_ - previous_conso;
    // TODO Trace: Trace energy_this_step from start_time to finish_time in host->getName()
    last_updated_ = finish_time;
        
    XBT_DEBUG("[update_emission of %s] period=[%.8f-%.8f]; total emission before: %.8f gCO2 -> added now: %.8f gCO2",
      host_->get_cname(), start_time, finish_time, previous_emissions, emissions_this_step);
    host_was_used_ = true; // Mark the host as used

    // Update the emission list
    if (type_of_csv != -1) {
      double current_time = simgrid::s4u::Engine::get_clock(); // Get simulation time
      add_emission_to_list(energy_this_step, start_time, current_time);
    }
    }

}


HostEmissions::HostEmissions(simgrid::s4u::Host* ptr) : host_(ptr), last_update_time_(simgrid::s4u::Engine::get_clock()) {
      const char* emission_file = host_->get_property("emission_file");

      std::filesystem::path p = emission_file;

      std::filesystem::path t = std::filesystem::current_path() / emission_file;
      int type = set_emission_file_type(emission_file);
      int list_size = 0;
      if (type_of_csv == 0) list_size = 12;
      else if (type_of_csv == 1) list_size = 366;
      else if (type_of_csv == 2) list_size = 24;
      else list_size = 0;

      total_emissions_list_.clear();
      for (int i = 0; i < list_size; ++i)
          total_emissions_list_.push_back(0.0);

      int v = read_emission_file(p);
      if (v != -1) {
          emission_value = v;
          //print_emission_list(list_emission_value);
      } else {
          XBT_INFO("Emission file not found or invalid %s, using default value: %f gCO2/kWh", t.string().c_str(), emission_value);
      }
}


HostEmissions::~HostEmissions() = default;

double HostEmissions::get_emission(){
  //double energy = sg_host_get_consumed_energy(host_);
      
  if (last_updated_ < simgrid::s4u::Engine::get_clock()) // We need to simcall this as it modifies the environment
      simgrid::kernel::actor::simcall_answered(std::bind(&HostEmissions::update, this));

  return total_emissions_;
}

}

using simgrid::plugin::HostEmissions;
/* **************************** events  callback *************************** */
static void on_creation(simgrid::s4u::Host& host)
{
    //on_creation_ptr(&host);

    if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

    host.extension_set(new HostEmissions(&host));
}

static void on_action_state_change(simgrid::kernel::resource::CpuAction const& action,
                                   simgrid::kernel::resource::Action::State /*previous*/)
{
    //on_action_state_change_ptr(action, /*ignored*/ action.get_state());
    for (simgrid::kernel::resource::CpuImpl const* cpu : action.cpus()) {
        simgrid::s4u::Host* host = cpu->get_iface();
        if (host != nullptr) {
            // If it's a VM, take the corresponding PM
            if (const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host))
                host = vm->get_pm();

            // Get the host_emission extension for the relevant host
            auto* host_emission = host->extension<HostEmissions>();

            if (host_emission->get_last_update_time() < simgrid::s4u::Engine::get_clock())
                host_emission->update();
        }
    }
}


/* This callback is fired either when the host changes its state (on/off) ("onStateChange") or its speed
 * (because the user changed the pstate, or because of external trace events) ("onSpeedChange") */
static void on_host_change(simgrid::s4u::Host const& h)
{
  //on_host_change_ptr(h);
  const auto* host = &h;
  if (const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)) // Take the PM of virtual machines
    host = vm->get_pm();
  host->extension<HostEmissions>()->update();
}

static void on_host_destruction(simgrid::s4u::Host const& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // Ignore virtual machines
    return;

    XBT_INFO("Emission of C02 of host %s: %f g", host.get_cname(), host.extension<HostEmissions>()->get_emission());
}


static void on_simulation_end()
{
  double total_emission = 0.0; // Total energy consumption (whole platform)
  double used_hosts_emission = 0.0; // Energy consumed by hosts that computed something
  for (simgrid::s4u::Host const* host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    if (host && dynamic_cast<const simgrid::s4u::VirtualMachine*>(host) == nullptr) // Ignore virtual machines
      {
      double emission = host->extension<HostEmissions>()->get_emission();
      total_emission += emission;
      if (host->extension<HostEmissions>()->host_was_used_)
        used_hosts_emission += emission;
      // Export the emission list to a CSV file
      host->extension<HostEmissions>()->export_emission_list();
    }
  }
  XBT_INFO("Total CO2 emission: %f g (used hosts: %f g; unused/idle hosts: %f g)",
     total_emission, used_hosts_emission, total_emission- used_hosts_emission);
  
 
}

static void on_activity_suspend_resume(simgrid::s4u::Activity const& activity)
{
  if (const auto* action = dynamic_cast<simgrid::kernel::resource::CpuAction*>(activity.get_impl()->model_action_))
  on_action_state_change(*action, /*ignored*/ action->get_state());
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
    simgrid::s4u::Host::on_creation_cb(&on_creation);
    simgrid::s4u::Host::on_onoff_cb(&on_host_change);
    simgrid::s4u::Host::on_speed_change_cb(&on_host_change);
    simgrid::s4u::Host::on_destruction_cb(&on_host_destruction);
    simgrid::s4u::Host::on_exec_state_change_cb(&on_action_state_change);
    simgrid::s4u::VirtualMachine::on_suspend_cb(&on_host_change);
    simgrid::s4u::VirtualMachine::on_resume_cb(&on_host_change);
    simgrid::s4u::Exec::on_suspend_cb(on_activity_suspend_resume);
    simgrid::s4u::Exec::on_resume_cb(on_activity_suspend_resume);
    simgrid::s4u::Engine::on_simulation_end_cb(&on_simulation_end);

    simgrid::s4u::Exec::on_start_cb([](simgrid::s4u::Exec const& activity) {
      if (activity.get_host_number() == 1) { // We only run on one host
        simgrid::s4u::Host* host = activity.get_host();
        if (const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host))
          host = vm->get_pm();
        xbt_assert(host != nullptr);
        host->extension<HostEmissions>()->update();
      }
    });
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

double sg_host_get_emission(const_sg_host_t host) {
    ensure_plugin_inited();
    return host->extension<HostEmissions>()->get_emission();
}

void sg_host_setCO2(const_sg_host_t host, double newCO2){
  host->extension<HostEmissions>()->setCO2(newCO2);
}

double sg_host_get_CO2(const_sg_host_t host){
  ensure_plugin_inited();
  return host->extension<HostEmissions>()->getCO2();
}

std::string sg_host_get_country(const_sg_host_t host){
  ensure_plugin_inited();
  return host->extension<HostEmissions>()->getCountry();
}

void sg_host_export_emission_list(const_sg_host_t host){
  ensure_plugin_inited();
  host->extension<HostEmissions>()->export_emission_list();
}
