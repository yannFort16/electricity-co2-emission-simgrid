#include "simgrid/s4u.hpp"

#include <simgrid/plugins/emission.h>
#include <simgrid/plugins/energy.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

void test_execution_CSV(){
    sg4::Host* actor = sg4::this_actor::get_host();

    //Exec qui dure moins d’une heure
    double start = sg4::Engine::get_clock();
    double emissionBefore = sg_host_get_emission(actor);
    double consoBefore = sg_host_get_consumed_energy(actor);
    double flopAmount = 4E9;
    //XBT_INFO("Run a computation of %.0E flops", flopAmount);
    
    sg4::this_actor::execute(flopAmount);
    
    XBT_INFO("Computation  of %.0E flops done (duration: %.2f s). Current peak speed=%.0E flop/s;\n CO2 Emission = %lf g; Energy consumption = %.0f J",
        flopAmount, sg4::Engine::get_clock() - start, actor->get_speed(), sg_host_get_emission(actor) - emissionBefore, 
        sg_host_get_consumed_energy(actor) - consoBefore);
    
    //Exec qui dure plus d’une heure
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(actor);
    consoBefore = sg_host_get_consumed_energy(actor);
    flopAmount = 36E10;
    //XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation  of %.0E flops done (duration: %.2f s). Current peak speed=%.0E flop/s;\n CO2 Emission = %lf g; Energy consumption = %.0f J",
        flopAmount, sg4::Engine::get_clock() - start, actor->get_speed(), sg_host_get_emission(actor) - emissionBefore,
        sg_host_get_consumed_energy(actor) - consoBefore);

    //Exec qui dure plus d’un jour
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(actor);
    consoBefore = sg_host_get_consumed_energy(actor);
    flopAmount = flopAmount*24;
    //XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation  of %.0E flops done (duration: %.2f s). Current peak speed=%.0E flop/s;\n CO2 Emission = %lf g; Energy consumption = %.0f J",
        flopAmount, sg4::Engine::get_clock() - start, actor->get_speed(), sg_host_get_emission(actor) - emissionBefore,
        sg_host_get_consumed_energy(actor) - consoBefore);

    //Exec qui dure plus d’un mois
    start = sg4::Engine::get_clock();
    emissionBefore = sg_host_get_emission(actor);
    consoBefore = sg_host_get_consumed_energy(actor);
    flopAmount = flopAmount*32;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO("Computation  of %.0E flops done (duration: %.2f s). Current peak speed=%.0E flop/s;\n CO2 Emission = %lf g; Energy consumption = %.0f J",
        flopAmount, sg4::Engine::get_clock() - start, actor->get_speed(), sg_host_get_emission(actor) - emissionBefore, 
        sg_host_get_consumed_energy(actor) - consoBefore);
    
    //Idle
    XBT_INFO("Sleep for 1 hour");
    sg4::this_actor::sleep_for(3600);

}


int main (int argc, char *argv[]) {
    sg_host_emission_plugin_init();
    sg4::Engine e(&argc, argv);

    xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/emission_platform.xml\n", argv[0], argv[0]);

    e.load_platform(argv[1]);
  
    e.add_actor("Emission Test 1", e.host_by_name("MyHost1"), test_execution_CSV);
    XBT_INFO("End of simulation 1.");
    e.add_actor("Emission Test 2", e.host_by_name("MyHost2"), test_execution_CSV);
    XBT_INFO("End of simulation 2.");
    e.add_actor("Emission Test 3", e.host_by_name("MyHost3"), test_execution_CSV);
    XBT_INFO("End of simulation 3.");
    e.run();
    return 0;
}