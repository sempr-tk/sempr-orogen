PROJECT(sempr)
cmake_minimum_required(VERSION 2.6)

SET (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.orogen/config")
INCLUDE(semprBase)
add_definitions(-Wno-unknown-pragma)

# FIND_PACKAGE(KDL)
# FIND_PACKAGE(OCL)

