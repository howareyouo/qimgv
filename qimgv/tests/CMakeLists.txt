enable_testing()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/qimgv)

# The classes under test plus the minimal runtime they need. ActionManager
# normally lives inside the full app; here it is linked against a stubbed
# ScriptManager (see scriptmanager_stub.cpp) to keep the dependency closure
# small.
set(TEST_SUPPORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../themestore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shortcutbuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/actiontable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/inputmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/script.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/actionmanager/actionmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptmanager_stub.cpp
)

add_executable(action_tests
    test_actiontable.cpp
    test_actionmanager.cpp
    ${TEST_SUPPORT_SOURCES}
)
target_link_libraries(action_tests PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
add_test(NAME action_tests COMMAND action_tests)

add_executable(unit_tests
    test_mapoverlay.cpp
    ../gui/overlays/mapoverlay.cpp
    ${TEST_SUPPORT_SOURCES}
)
target_link_libraries(unit_tests PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
add_test(NAME QUI_TEST COMMAND unit_tests)
