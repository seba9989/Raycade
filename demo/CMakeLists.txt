# Demonstracja silnika: main.cpp i sceny z laboratoriów.
# Sceny muszą być tu (w pliku wykonywalnym), a nie w bibliotece statycznej - linker
# pominąłby pliki .o, do których nic się nie odwołuje, razem z rejestracją makra SCENE.
file(GLOB_RECURSE DEMO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(demo ${DEMO_SOURCES})
target_link_libraries(demo PRIVATE raycade::raycade raycade_warnings)

# Program wynikowy: build/raycade-demo (skrypty devenv, debugger Zeda).
set_target_properties(demo PROPERTIES
    OUTPUT_NAME raycade-demo
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Zasoby gry (bitmapy, sprite sheety) kopiowane obok pliku wykonywalnego
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    add_custom_command(TARGET demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:demo>/assets)
endif()
