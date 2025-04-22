#include <include/simgrid/plugins/emission.h>
#include <simgrid/plugins/energy.h>
#include <iostream>
#include <string>

double sg_host_get_emission(const_sg_host_t host, const std::string& csv_file_path);

int main() {
    const std::string csv_file_path = "\\wsl.localhost\\Ubuntu\\home\\xepz\\BE\\simgrid\\Sujet_19\\FR_2024_monthly.csv";
    double emission = sg_host_get_emission(nullptr, csv_file_path);
    std::cout << "Emission: " << emission << " gCO2" << std::endl;
    return 0;
}