# s4u-emission-exec1 Project

This project demonstrates the use of SimGrid's S4U API to simulate a computation and log CO2 emissions. The main components of the project are as follows:

## Files

- **src/s4u-emission-exec1.cpp**: This file contains the main implementation of the simulation. It defines the `test_execution` function, which simulates a computation and logs CO2 emissions. The `main` function initializes the simulation engine and loads the platform configuration.

- **CMakeLists.txt**: This file is the configuration file for CMake. It specifies the project name, the required C++ standard, and the source files to be compiled. It also includes directives to find and link the SimGrid library.

## Building the Project

To build the project, follow these steps:

1. Ensure you have CMake and a compatible C++ compiler installed on your system.
2. Navigate to the `s4u-emission-exec1` directory.
3. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```bash
   cmake ..
   ```
5. Build the project:
   ```bash
   make
   ```

## Running the Simulation

After building the project, you can run the simulation with the following command:

```bash
./s4u-emission-exec1 <platform_file>
```

Replace `<platform_file>` with the path to your platform configuration file (e.g., `../platforms/emission_platform.xml`).

## Dependencies

This project requires the SimGrid library. Make sure to install it and set up the environment correctly to link against it during the build process.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.