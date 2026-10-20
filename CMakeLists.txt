cmake_minimum_required(VERSION 3.16.0)

list(APPEND EXTRA_COMPONENT_DIRS
    "components/"
    "components/applications"
    "components/drivers"
    "components/filters"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sumo-firmware)
