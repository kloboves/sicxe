file(GLOB SOURCES *.cc)
file(GLOB LOGIC_SOURCES logic/*.cc)

add_library(machine_lib ${SOURCES} ${LOGIC_SOURCES})
