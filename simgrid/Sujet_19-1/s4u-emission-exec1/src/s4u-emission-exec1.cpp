#include "simgrid/s4u.hpp"

//Change this to include your full directroy path to emission .h file
#include "/home/yann/newSimGrid/simgrid/include/simgrid/plugins/emission.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

void test_execution() {
    sg4::Host* host1 = sg4::Host::by_name("MyHost1");
    //sg4::Host* host2 = sg4::Host::by_name("MyHost2");

    if (!host1) {
        XBT_ERROR("Hosts not found");
        return;
    }

    double start = sg4::Engine::get_clock();
    XBT_INFO("Sleep for 10 seconds");
    sg4::this_actor::sleep_for(10);

    double emission1 = sg_host_get_emission(host1);
    if (emission1 < 0) {
        XBT_ERROR("Failed to get emission data for host1");
        return;
    }
    XBT_INFO("Done sleeping (duration: %.2f s); CO2 emission = %.2f g",
             sg4::Engine::get_clock() - start , emission1);

    start = sg4::Engine::get_clock();
    double flopAmount = 100E6;
    XBT_INFO("Run a computation of %.0E flops", flopAmount);
    sg4::this_actor::execute(flopAmount);
    XBT_INFO(
      "Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; CO2 Emission =%.0f g",
      sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_emission(host1));
}

int main (int argc, char *argv[]) {
    sg_host_emission_plugin_init();
    sg4::Engine e(&argc, argv);

    xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/emission_platform.xml\n", argv[0], argv[0]);

    e.load_platform(argv[1]);
  
    e.add_actor("emission test", e.host_by_name("MyHost1"), test_execution);
    
    XBT_INFO("End of simulation.");
    e.run();
    return 0;
}