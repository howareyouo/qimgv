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

add_library(qimgv_test_support STATIC ${TEST_SUPPORT_SOURCES})
target_link_libraries(qimgv_test_support
    PUBLIC Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(actiontable_tests test_actiontable.cpp)
target_link_libraries(actiontable_tests PRIVATE qimgv_test_support)
add_test(NAME actiontable_tests COMMAND actiontable_tests)

add_executable(actionmanager_tests test_actionmanager.cpp)
target_link_libraries(actionmanager_tests PRIVATE qimgv_test_support)
add_test(NAME actionmanager_tests COMMAND actionmanager_tests)

add_executable(unit_tests
    test_mapoverlay.cpp
    ../gui/overlays/mapoverlay.cpp
)
target_link_libraries(unit_tests PRIVATE qimgv_test_support)
add_test(NAME unit_tests COMMAND unit_tests)
