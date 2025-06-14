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

SIMGRID_REGISTER_PLUGIN(host_emission, "Cpu CO2 emisson based on energy consumption.", &sg_host_emission_plugin_init)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host_emission, kernel, "Logging specific to the host emission plugin");

static void on_simulation_end();

namespace simgrid::plugin {


class HostEmissions {
    simgrid::s4u::Host* host_ = nullptr;

    double emission_value = 42.0; // g CO2/kWh in France on the 11/03/2025 used as default value
    double total_emissions_  = 0.0; // Total CO2 emission in g
    double conso_total_joule = 0.0; // Total energy consumption in J
    std::list<double> total_emissions_list_ = {}; 
    double last_updated_  = simgrid::s4u::Engine::get_clock(); /*< Timestamp of the last energy update event*/
    time_t last_update_time_ = time(nullptr); // Last update time in seconds since epoch
    std::string country = "No data";
    int type_of_csv =-1; // -1: No CSV, 0: Monthly, 1: Daily, 2: Hourly, 3: Yearly
    std::list<double> list_emission_value = {}; // List of emission values
    bool host_was_used_ = false; // Tracks whether the host was used
    std::list<std::string> date_utc_list = {}; // List of dates in UTC format
    double time_left = 0.0;

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
    std::list<double> fill_emission_list(double last_value, int type_of_csv = -1);
    void print_emission_list(std::list<double> list_double);
    int get_index_time(double time);
    double add_emission_to_list(double emission, double start_time, double current_time);
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
  //print_emission_list(list_emission_value);
  return newEmissionValue;
}

std::list<double> HostEmissions::fill_emission_list(double last_value, int type_of_csv) {
    std::list<double> new_list;
    int target_size = 0;
    if (type_of_csv == 0) target_size = 12;
    else if (type_of_csv == 1) target_size = 366;
    else if (type_of_csv == 2) target_size = 24; // 24 hours * 366 days
    else if (type_of_csv == 3) target_size = 1;
    else return new_list;

    for (int i = 0; i < target_size; i++) {
        new_list.push_back(last_value);
    }
    return new_list;
}

int HostEmissions::set_emission_file_type(std::string emission_file){
  if (emission_file.find("monthly") != std::string::npos){
    type_of_csv = 0;
    time_left = 86400.0 * 31; // 1 month in seconds Start with 31 days : january
    //XBT_INFO("CSV file Monthly: '%s'", emission_file.c_str());
  }else if (emission_file.find("daily") != std::string::npos){
    type_of_csv = 1;
    time_left = 86400.0; // 1 day in seconds
    //XBT_INFO("CSV file Daily: '%s'", emission_file.c_str());
  }else if (emission_file.find("hourly") != std::string::npos){
    type_of_csv = 2;
    time_left = 3600.0; // 1 hour in seconds
    //XBT_INFO("CSV file Hourly: '%s'", emission_file.c_str());
  }else if (emission_file.find("yearly") != std::string::npos){
    type_of_csv = 3;
    time_left = 365 * 86400.0; // 1 year in seconds
    //XBT_INFO("CSV file Yearly: '%s'", emission_file.c_str());
  }
  return 0;
}

std::string HostEmissions::get_CSV_Type(){
  if (type_of_csv == 0) return "m";
  else if (type_of_csv == 1) return "d";
  else if (type_of_csv == 2) return "h";
  else if (type_of_csv == 3) return "y";
  else return "unknown";
}

void HostEmissions::export_emission_list(){
  //print_emission_list(total_emissions_list_);
  // Add the current emission list to the export list
  add_emission_list_to_export();
  
  // Export the emission_export_list to a CSV file
  std::string export_type_str = get_CSV_Type();
  std::string folder = "../ressources/exports";
  std::filesystem::create_directories(folder); // Ensure the folder exists
  std::string filename = folder + "/emission_" + host_->get_name() + "_" + this->country + ".csv";
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
      outfile << i << "," << j+1 << export_type_str << "," << (*it_nested) << "\n";
      ++it_nested;
    }
    ++it;
  }
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
    return date_time.tm_hour-1;
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
  //std::cout << "Total Emission list :\n" << std::endl;
  //print_emission_list(total_emissions_list_);
  //XBT_INFO("Total Emission list is full. It will be added to the result CSV file at the end");
  //Clear the total emissions list
  total_emissions_list_.clear();
  total_emissions_list_ = fill_emission_list(0.0, type_of_csv);
}


double HostEmissions::add_emission_to_list(double conso_this_step, double start_time, double current_time) {
  //Return emission added to the list
    //int N = static_cast<int>(total_emissions_list_.size());
    int N = static_cast<int>(list_emission_value.size());
    if (N == 0 || start_time >= current_time){
        XBT_ERROR("No emissions to add or invalid time range.");
        return 0.0;
    }
    
    
    //std::cout << "N :" << N << std::endl;
     
    // Total duration
    double total_duration = current_time - start_time;
    int start_index = 0;
    int index_emission = 0;
    if(type_of_csv == 2){ 
      index_emission = static_cast<int>(start_time / 3600.0);
      start_index = index_emission % 24; // Hourly
    }
    else start_index = get_index_time(start_time);  
    //std::cout << "Total duration :" << total_duration << "s"<< std::endl;
    //std::cout << "Conso :" << conso_this_step << " kWh"<< std::endl;
    // For each type_csv, calculate the number of lists and the number of steps to add the full consumption
    double unite = 0.0;
    //Only used for type_of_csv = 0
    std::list<int> list_size_monthly = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // Days in each month
    auto it_jour_mois = list_size_monthly.begin();
    std::advance(it_jour_mois, start_index);

    if (type_of_csv == 0) unite = 86400 * (*it_jour_mois); // 1 month
    else if (type_of_csv == 1) unite = 86400; // 1 day
    else if (type_of_csv == 2) unite = 3600; // 1 hour
    else if (type_of_csv == 3) unite = 365 * 86400; // 1 year
    else{
      XBT_ERROR("Invalid type_of_csv");
      return 0.0;
    } 
    int step = 0;
    int n = time_left - total_duration;
    int new_time_left = time_left;
    if (n < 0){
      int k = std::abs(n);
      double total_duration_temp = total_duration; // Subtract the first part duration
      step = 2; // Start with 2 steps, one for the first part and one for the last part

      
      //Bad condition, not using time left
      if (type_of_csv == 0){
        if (it_jour_mois != list_size_monthly.end()) {
          ++it_jour_mois; // Move to the next month
        } else {
          it_jour_mois = list_size_monthly.begin(); // Reset to the first month
        }
        unite = (*it_jour_mois) * 86400; // Days in the month * 86400 seconds
      }

      while (k > unite) {
        //std::cout << "Unite :" << unite <<"s"<< std::endl;
        step += 1;
        total_duration_temp -= unite;
        k -= unite; // Subtract the full unite from k
        if (type_of_csv == 0){
          if (it_jour_mois != list_size_monthly.end()) {
            ++it_jour_mois; // Move to the next month
          } else {
            it_jour_mois = list_size_monthly.begin(); // Reset to the first month
          }
          unite = (*it_jour_mois) * 86400; // Days in the month * 86400 seconds
        }
      }

      
      //total_duration_temp -= k; // Subtract the remaining time

      //step = std::ceil(k / unite) + 1;
      //new_time_left = unite - total_duration_temp;
      new_time_left = unite - k; // Update new_time_left with the remaining time
    }else{
      step = 1;
      new_time_left -= total_duration;
    }

    int nb_lists = N>1 ? std::div(step, N).quot : step-1;
    
    /*/Prints debug : 
    std::cout << "Nb de list:" <<nb_lists << std::endl;
    std::cout << "Step:" <<step << std::endl;
    std::cout << "Time left Before Computation: " <<time_left << "(s)" << std::endl;
    std::cout << "Time left After Computation: " <<new_time_left << "(s)" << std::endl;
    std::cout << "Start Index:" <<start_index << std::endl;*/
    

    double conso_per_second = 0.0;
    if (step == 0) return 0.0;
    else{
      conso_per_second = conso_this_step / total_duration;
      //std::cout << "Conso per second:" <<conso_per_second << std::endl;
    }
    
    //Start adding emissions
    double emission_added_total = 0.0;
    double emission_per_step = 0.0;
    int current_index = start_index;
    int duration_per_step = 1;
    //int steps_remaining = step;


    auto it = total_emissions_list_.begin();
    std::advance(it, current_index);
    auto emission_it = list_emission_value.begin();
    if (type_of_csv == 2) std::advance(emission_it, index_emission);
    else std::advance(emission_it, current_index);
    it_jour_mois = list_size_monthly.begin();
    std::advance(it_jour_mois, current_index);
    

    for (int steps_remaining = step; steps_remaining > 0; steps_remaining--) {
      if (type_of_csv == 0){   
          unite = (*it_jour_mois) * 86400; // Days in the month * 86400 seconds
          //std::cout << "Unite M: " << unite <<"s"<< std::endl;
      }
      if(steps_remaining == step) {
        // First step, we need to handle the first step duration
        duration_per_step = n<0 ? time_left: total_duration ;
        //std::cout << "Duree 2:" << duration_per_step << "s" << std::endl;
      }else if (steps_remaining <= 1) {
        // Last step, we need to handle the last step duration
        duration_per_step = (unite - new_time_left);
        if (duration_per_step <= 0) {
          duration_per_step = -1; // Avoid division by zero
          XBT_ERROR("Duration per step is negative or zero, check your time left and unite values.");
        }
        //std::cout << "Duree 1:" << duration_per_step << "s" << std::endl;
      } else {
        // Intermediate steps, we can use the full unite
        duration_per_step = unite;
        //std::cout << "Duree 3:" << duration_per_step << "s" << std::endl;
      }
      emission_per_step = conso_per_second * duration_per_step * (*emission_it);
      emission_added_total += emission_per_step;
      *it += emission_per_step;
      
      //Prints debug : 
      /*std::cout << "Conso per step :" << conso_per_second * duration_per_step << " kWh" << std::endl;
      std::cout << "Emission IT :" <<(*emission_it) << std::endl;
      std::cout << "\tCurrent index :" <<current_index << std::endl;
      std::cout << "\tEmission per step :" <<emission_per_step << std::endl;
      std::cout << "\tEmission added total :" <<emission_added_total << std::endl;*/
      ++it;
      ++emission_it;
      current_index++;
      if (it_jour_mois != list_size_monthly.end() && type_of_csv == 0) {
            ++it_jour_mois; // Move to the next month
      }
      if ((current_index >= N && N>1) || (current_index >= N && N ==1 & n < 0)) {
        add_emission_list_to_export();
        current_index = 0;
        it = total_emissions_list_.begin();
        emission_it = list_emission_value.begin();
        if(type_of_csv == 0) {
          it_jour_mois = list_size_monthly.begin(); // Reset to the first month
        }
      }
      if (current_index%24 == 0 && type_of_csv == 2) {
        add_emission_list_to_export(); // Reset the list every 24 hours
        // Reset to the first hour of the day
        current_index = 0;
        it = total_emissions_list_.begin();
        
        ///if (is end of list) emission_it = list_emission_value.begin();
      }
    }
    time_left = new_time_left;
    return emission_added_total;
}


void HostEmissions::update() {
  double start_time  = last_updated_;
  double finish_time = simgrid::s4u::Engine::get_clock();
  if (start_time < finish_time) {
    
    double new_conso_joule = sg_host_get_consumed_energy(host_);
    //std::cout << "Conso before update  = " <<conso_total_joule << "(J); Conso after update = " <<new_conso_joule << "(J)" << std::endl;
    
    double new_conso_this_step = JouleTokWattH(new_conso_joule - conso_total_joule);
    conso_total_joule = new_conso_joule;
    
    double emissions_this_step = 0.0;
    double previous_emissions = total_emissions_;

    // Update the emission list
    if (type_of_csv != -1) {
      double current_time = simgrid::s4u::Engine::get_clock(); // Get simulation time
      emissions_this_step = add_emission_to_list(new_conso_this_step, start_time, current_time);
    } else {
      emissions_this_step = new_conso_this_step * emission_value;
    }
    //std::cout << "Emission this step  = " <<emissions_this_step << "(gCO2)" << std::endl;
    total_emissions_ += emissions_this_step;
    last_updated_ = finish_time;
        
    XBT_DEBUG("[update_emission of %s] period=[%.8f-%.8f]; total emission before: %.8f gCO2 -> added now: %.8f gCO2",
      host_->get_cname(), start_time, finish_time, previous_emissions, emissions_this_step);
    if (!host_was_used_) host_was_used_ = host_->is_on(); // Check if the host is on
    
    }
}


HostEmissions::HostEmissions(simgrid::s4u::Host* ptr) : host_(ptr), last_update_time_(simgrid::s4u::Engine::get_clock()) {
      const char* emission_file = host_->get_property("emission_file");
      if (emission_file == nullptr) {
        XBT_INFO("No emission file specified for host %s, using default value: %f gCO2/kWh", host_->get_cname(), emission_value);
        return;
      }
      std::filesystem::path p = emission_file;

      std::filesystem::path t = std::filesystem::current_path() / emission_file;
      int type = set_emission_file_type(emission_file);
      int list_size = 0;
      if (type_of_csv == 0) list_size = 12;
      else if (type_of_csv == 1) list_size = 366;
      else if (type_of_csv == 2) list_size = 24; // 24 hours * 366 days
      else if (type_of_csv == 3) list_size = 1;
      else list_size = 0;

      total_emissions_list_.clear();
      for (int i = 0; i < list_size; ++i)
          total_emissions_list_.push_back(0.0);

      int v = read_emission_file(p);
      if (v != -1) {
          emission_value = v;
          //std::cout << "Emission Val = " << emission_value << std::endl;
          //print_emission_list(list_emission_value);
      } else {
          XBT_INFO("Emission file not found or invalid %s, using default value: %f gCO2/kWh", t.string().c_str(), emission_value);
      }
}

HostEmissions::~HostEmissions() = default;

double HostEmissions::get_emission(){      
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
      if ( host->extension<HostEmissions>()->type_of_csv != -1) // If the type of CSV is set, export the emission list
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

/** @ingroup plugin_host_emission
 *  @brief Enable host emission plugin
 *  @details Enable emssion plugin to get CO2 emissions based on energy consumption.
 *         Call this function before loading your platform.
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

/** @ingroup plugin_host_emission
 *  @brief updates the emissons of all hosts
 *
 * This function iterates over all hosts in the simulation and updates their emissions.
 */
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

/** @ingroup plugin_host_emission
 *  @brief Ensures that the Emission plugin is initialized before use.
 *
 * This function checks whether the Emission plugin has been properly initialized.
 * If the plugin is not active, it throws an InitializationError with a descriptive message,
 * instructing the user to call sg_host_emission_plugin_init() before using any related functions.
 *
 * @throws simgrid::xbt::InitializationError if the Emission plugin is not active.
 */
static void ensure_plugin_inited()
{
  if (not HostEmissions::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError("The Emission plugin is not active. Please call sg_host_emission_plugin_init() "
                                            "before calling any function related to that plugin.");
}

/** @ingroup plugin_host_emission
 *  @brief Retrieves the CO2 emissions for a specified host.
 *
 * This function returns the total CO2 emission (in appropriate units) 
 * of a given host. It is based on the energy consumption of the host.
 *
 * @param host The host for which to retrieve CO2 emissions.
 * @return The CO2 emissions value for the specified host in g of CO2.
 */
double sg_host_get_emission(const_sg_host_t host) {
    ensure_plugin_inited();
    return host->extension<HostEmissions>()->get_emission();
}

/** @ingroup plugin_host_emission
 *  @brief Sets the CO2 emission value for the specified host.
 *
 * This function updates the CO2 emission metric associated with the given host.
 *
 * @param host    The host whose CO2 emission value is to be set.
 * @param newCO2  The new CO2 emission value to assign to the host in g of CO2 per kWh.
 */
void sg_host_setCO2(const_sg_host_t host, double newCO2){
  host->extension<HostEmissions>()->setCO2(newCO2);
}

/** @ingroup plugin_host_emission
 *  @brief Retrieves the CO2 emissions associated with the specified host.
 *
 * This function returns the constant CO2 value (in appropriate units) 
 * that the computation of emissions is based on.
 *
 * @param host The host for which to retrieve CO2 emissions constant.
 * @return The CO2 emissions value for the specified host in g of CO2 per kWh.
 */
double sg_host_get_CO2(const_sg_host_t host){
  ensure_plugin_inited();
  return host->extension<HostEmissions>()->getCO2();
}

/** @ingroup plugin_host_emission
 *  @brief Retrieves the country associated with a given host.
 *
 * This function returns the country information for the specified host
 * by accessing its HostEmissions extension.
 *
 * @param host The host object for which to retrieve the country.
 * @return A string representing the country of the host.
 */
std::string sg_host_get_country(const_sg_host_t host){
  ensure_plugin_inited();
  return host->extension<HostEmissions>()->getCountry();
}

/** @ingroup plugin_host_emission
 *  @brief Exports the emission list for a given host.
 *
 * This function ensures that the emission plugin is initialized and then
 * exports the emission list associated with the specified host.
 * 
 * The exportion is done automatically at the end of the simulation,
 * so this function is not mandatory to call.
 *
 * @param host The host whose emission list is to be exported.
 */
void sg_host_export_emission_list(const_sg_host_t host){
  ensure_plugin_inited();
  host->extension<HostEmissions>()->export_emission_list();
}
