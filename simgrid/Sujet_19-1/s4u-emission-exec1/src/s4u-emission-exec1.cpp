#include "simgrid/s4u.hpp"

//Change this to include your full directroy path to emission .h file   
#include "/home/yann/newSimGrid/simgrid/include/simgrid/plugins/emission.h"
#include "/home/yann/newSimGrid/simgrid/include/simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

void test_execution() {
    sg4::Host* host1 = sg4::Host::by_name("MyHost1");

    sg_host_setCO2(host1, 90);

    sg4::Host* host2 = sg4::Host::by_name("MyHost2");
    sg4::Host* host3 = sg4::Host::by_name("MyHost3");
    host1->turn_on();
    host2->turn_off();
    host3->turn_off();
    if (!host1) {
        XBT_ERROR("Hosts not found");
        return;
    }


    double c02_h1 = sg_host_get_CO2(host1);
    std::string country_h1 = sg_host_get_country(host1);
    XBT_INFO("Host1 CO2 emission: %.2f gCO2/kWh, Country: %s", c02_h1, country_h1.c_str());


    double c02_h2 = sg_host_get_CO2(host2);
    std::string country_h2 = sg_host_get_country(host2);
    XBT_INFO("Host2 CO2 emission: %.2f gCO2/kWh, Country: %s", c02_h2, country_h2.c_str());

    //Test IDLE
    double start = sg4::Engine::get_clock();
    XBT_INFO("Sleep for 10 seconds");
    sg4::this_actor::sleep_for(10);

    double emission1 = sg_host_get_emission(host1);
    if (emission1 < 0) {
        XBT_ERROR("Failed to get emission data for host1");
        return;
    }
    XBT_INFO("Done sleeping (duration: %.2f s); CO2 emission = %lf g",
             sg4::Engine::get_clock() - start , emission1);

    
    //Executer une tache
    start = sg4::Engine::get_clock();
    double flopAmount = 5E9;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    
    sg4::this_actor::execute(flopAmount);
    
    XBT_INFO(
      "Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission =%lf g",
      sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emission1);

    //Executer 2 tache
    host2->turn_on();
    start = sg4::Engine::get_clock();

    double flopAmount1 = 6E9;
    double flopAmount2 = 5E9;
    XBT_INFO("Run a computation of %.0E flops on host1, Run a computation of %.0E flops on host2", flopAmount1, flopAmount2);
    simgrid::s4u::ExecPtr exec1 = host1->exec_init(flopAmount1);
    simgrid::s4u::ExecPtr exec2 = host2->exec_init(flopAmount2);
    exec1->start();
    exec2->start();
    exec1->wait();
    exec2->wait();
    emission1 = sg_host_get_emission(host1);
    double emission2 = sg_host_get_emission(host2);
    XBT_INFO(
        "Computation done (duration: %.2f s). Host1 peak speed=%.0E flop/s; Host1 CO2 Emission =%lf g, Host2 peak speed=%.0E flop/s; Host2 CO2 Emission =%lf g",
        sg4::Engine::get_clock() - start, host1->get_speed(), emission1, host2->get_speed(), emission2);
  
    host2->turn_off();  
    
    //Executer N tache(N = nb Coeur)
    start = sg4::Engine::get_clock();
    int n = host1->get_core_count();    
    XBT_INFO("Run a computation of %.0E flops on each core of host1", flopAmount1);
    sg4::this_actor::thread_execute(host1,flopAmount1, n);
    double newEmission = sg_host_get_emission(host1) - emission1;
    XBT_INFO(
        "Computation on each cores done (duration: %.2f s). Host1 CO2 Emission =%lf g",
        sg4::Engine::get_clock() - start, newEmission);

}


void testsPt3(){
    sg4::Host* host1 = sg4::Host::by_name("MyHost1");
    sg4::Host* host2 = sg4::Host::by_name("MyHost2");
    sg4::Host* host3 = sg4::Host::by_name("MyHost3");
    //sg4::Host* host4 = sg4::Host::by_name("MyHost4");

    //Sleep for an hour
    /*XBT_INFO("Sleep for 1 hour");
    sg4::this_actor::sleep_for(3600);*/

    host2->turn_off();
    host3->turn_off();

    //Exec qui dure moins d’une heure
    double start = sg4::Engine::get_clock();
    double emissionBefore = sg_host_get_emission(host1);
    double consoBefore = sg_host_get_consumed_energy(host1);
    double flopAmount = 4E9;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    
    sg4::this_actor::execute(flopAmount);
    
    XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission = %lf g; Energy consumption = %.0f J",
        sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emissionBefore, 
        sg_host_get_consumed_energy(host1) - consoBefore);
    
    //Exec qui dure plus d’une heure
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(host1);
    consoBefore = sg_host_get_consumed_energy(host1);
    flopAmount = 36E10;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission = %lf g; Energy consumption = %.0f J",
        sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emissionBefore,
        sg_host_get_consumed_energy(host1) - consoBefore);

    //Exec qui dure plus d’un jour
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(host1);
    consoBefore = sg_host_get_consumed_energy(host1);
    flopAmount = flopAmount*24;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission = %lf g; Energy consumption = %.0f J",
        sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emissionBefore,
        sg_host_get_consumed_energy(host1) - consoBefore);

    //Exec qui dure plus d’un mois
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(host1);
    consoBefore = sg_host_get_consumed_energy(host1);
    flopAmount = flopAmount*32;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission = %lf g; Energy consumption = %.0f J",
        sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emissionBefore, 
        sg_host_get_consumed_energy(host1) - consoBefore);
    
    //Annuler une exec et vérifier si le calcul est bon
    //TODO

}


int main (int argc, char *argv[]) {
    sg_host_emission_plugin_init();
    sg4::Engine e(&argc, argv);

    xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/emission_platform.xml\n", argv[0], argv[0]);

    e.load_platform(argv[1]);
  
    //e.add_actor("Emission Test 1", e.host_by_name("MyHost1"), test_execution);
    e.add_actor("Emission Test 2", e.host_by_name("MyHost1"), testsPt3);
    XBT_INFO("End of simulation.");
    e.run();
    return 0;
}