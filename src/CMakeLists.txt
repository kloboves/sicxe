add_subdirectory(assembler)
add_subdirectory(common)
add_subdirectory(fpga)
add_subdirectory(linker)
add_subdirectory(machine)
add_subdirectory(simulator)

add_executable(sicvm main_vm.cc)
target_link_libraries(sicvm machine_lib common_lib)

add_executable(sicsim main_sim.cc)
target_link_libraries(sicsim simulator_lib linker_lib  machine_lib common_lib)

add_executable(sicasm main_asm.cc)
target_link_libraries(sicasm assembler_lib common_lib)

add_executable(sicld main_ld.cc)
target_link_libraries(sicld linker_lib common_lib)

add_executable(sicfpga main_fpga.cc)
target_link_libraries(sicfpga fpga_lib common_lib)
