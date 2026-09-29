# Build-time GLSL -> SPIR-V compilation (CONVENTIONS §1: glslc). Each shader becomes a C initializer
# list in <build>/shaders/<file>.spv.inc, which the owning target includes as a uint32_t array.

find_program(AXIOM_GLSLC glslc REQUIRED)

# axiom_add_shaders(<target> <shader files...>)
function(axiom_add_shaders target)
  set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  set(outputs)
  foreach(shader ${ARGN})
    get_filename_component(name ${shader} NAME)
    set(output ${out_dir}/${name}.spv.inc)
    add_custom_command(
      OUTPUT ${output}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
      COMMAND ${AXIOM_GLSLC} --target-env=vulkan1.3 -Werror -O -mfmt=c ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${output}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
      COMMENT "glslc ${shader}"
      VERBATIM)
    list(APPEND outputs ${output})
  endforeach()
  target_sources(${target} PRIVATE ${outputs})
  target_include_directories(${target} PRIVATE ${out_dir})
endfunction()
