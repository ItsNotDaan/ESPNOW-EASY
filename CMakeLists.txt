# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the components directory
# To use an example, uncomment one of the lines below and comment out "main"
set(EXTRA_COMPONENT_DIRS 
    components
    main                  # Default placeholder application
    # examples/master       # Uncomment to build master example
    # examples/slave        # Uncomment to build slave example
    # examples/multi_slave  # Uncomment to build multi-slave example
    # examples/master_led   # Uncomment to build master with LED
    # examples/slave_led    # Uncomment to build slave with LED
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espnow-easy)
