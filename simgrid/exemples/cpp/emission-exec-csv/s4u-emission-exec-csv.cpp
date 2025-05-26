#include "simgrid/s4u.hpp"

//Change this to include your full directroy path to emission .h file   
#include <simgrid/plugins/emission.h>
#include <simgrid/plugins/energy.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

void test_execution_CSV(){
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
    flopAmount = flopAmount*31;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission = %lf g; Energy consumption = %.0f J",
        sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1) - emissionBefore, 
        sg_host_get_consumed_energy(host1) - consoBefore);

}


int main (int argc, char *argv[]) {
    sg_host_emission_plugin_init();
    sg4::Engine e(&argc, argv);

    xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/emission_platform.xml\n", argv[0], argv[0]);

    e.load_platform(argv[1]);
    
    e.add_actor("Emission Test 2", e.host_by_name("MyHost1"), test_execution_CSV);
    XBT_INFO("End of simulation.");
    e.run();
    return 0;
}