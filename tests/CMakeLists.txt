add_executable(PurpleTubaTests
    DspTests.cpp
    MidiLearnTests.cpp
    ParameterTests.cpp
    ProcessorTests.cpp
    ShortcutTests.cpp
    TestHelpers.h)

# The plug-in's shared code target (a static library) already contains the processor, the DSP
# and the compiled JUCE modules. Link it, and borrow its include paths and JUCE configuration
# macros so the tests see the same headers and settings.
target_link_libraries(PurpleTubaTests PRIVATE PurpleTuba Catch2::Catch2WithMain)
target_include_directories(PurpleTubaTests PRIVATE $<TARGET_PROPERTY:PurpleTuba,INCLUDE_DIRECTORIES>)
target_compile_definitions(PurpleTubaTests PRIVATE $<TARGET_PROPERTY:PurpleTuba,COMPILE_DEFINITIONS>)
target_compile_options(PurpleTubaTests PRIVATE $<TARGET_PROPERTY:PurpleTuba,COMPILE_OPTIONS>)

include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/Catch2/extras/Catch.cmake)
catch_discover_tests(PurpleTubaTests DISCOVERY_MODE PRE_TEST)
