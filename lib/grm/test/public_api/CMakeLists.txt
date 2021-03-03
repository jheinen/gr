cmake_minimum_required(VERSION 3.1...3.16)

project(
  grm_test_public_api
  DESCRIPTION "Test the public api of GRM"
  LANGUAGES C
)

set(EXECUTABLE_SOURCES
    bar_errorbar.c
    barplot.c
    custom_receiver.c
    custom_sender.c
    event_handling.c
    heatmap.c
    histogram.c
    hold_append.c
    imshow.c
    isosurface.c
    line_errorbar.c
    merge_args.c
    multi_plot.c
    pie.c
    plot.c
    polar_histogram.c
    quiver.c
    raw.c
    receiver.c
    scatter_errorbar.c
    sender.c
    size.c
    subplots.c
)

foreach(executable_source ${EXECUTABLE_SOURCES})
  get_filename_component(executable "${executable_source}" NAME_WE)
  add_executable("${PROJECT_NAME}_${executable}" "${executable_source}")
  target_link_libraries("${PROJECT_NAME}_${executable}" PRIVATE grm_shared)
  target_link_libraries("${PROJECT_NAME}_${executable}" PRIVATE gr_shared)
  target_link_libraries("${PROJECT_NAME}_${executable}" PRIVATE m)
  target_compile_options("${PROJECT_NAME}_${executable}" PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(
    "${PROJECT_NAME}_${executable}" PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF
  )
endforeach()
