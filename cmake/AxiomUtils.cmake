function(add_axiom_target name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE axiom)

    add_custom_command(
        TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/resources
        COMMENT "Copying resources to target output directory"
    )


endfunction()