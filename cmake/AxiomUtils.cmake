function(set_warnings_as_errors name)
    set_property(TARGET ${name} PROPERTY COMPILE_WARNING_AS_ERROR ON)
    if (MSVC)
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif ()
endfunction()

function(add_axiom_target name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE axiom)
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    # TODO: slang-rhi has some uses that break warnings-as-errors.
    # set_warnings_as_errors(${name})

endfunction()

function(add_axiom_example name src)
    add_axiom_target(${name} ${src})

    add_custom_command(
            TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources
            COMMENT "Copying resources to target output directory"
    )
endfunction()

function(add_axiom_test name src)
    add_axiom_target(${name} ${src})

    add_custom_command(
            TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_resources
            ${CMAKE_CURRENT_BINARY_DIR}/test_resources
            COMMENT "Copying test_resources to target output directory"
    )
endfunction()
