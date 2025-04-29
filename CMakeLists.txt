cmake_minimum_required(VERSION 3.10)

# Set CMake policies to suppress warnings
cmake_policy(SET CMP0144 NEW)
cmake_policy(SET CMP0167 NEW)

# Project name and C++ standard
project(WebSocket_HFT)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define Windows version
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-DWIN64)
    endif()
endif()

# Set architecture for Windows
if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM x64)
    add_definitions(-D_AMD64_)
    add_compile_options(/EHsc /MT)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# Windows version definition
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
add_definitions(-D_HAS_STD_BYTE=0)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Boost configuration - use vcpkg's installation
set(BOOST_ROOT "D:/vcpkg/installed/x64-windows")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
set(BOOST_NO_SYSTEM_PATHS ON)

# Add vcpkg's installed directory to prefix path
list(APPEND CMAKE_PREFIX_PATH "D:/vcpkg/installed/x64-windows")

# OpenSSL configuration
set(OPENSSL_ROOT_DIR "D:/vcpkg/installed/x64-windows")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libssl.lib")

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add source files
set(SOURCES
    error_handler.cpp
    latency_module.cpp
    websocket_server.cpp
    performance_dashboard.cpp
    strategy_manager.cpp
    trade_execution.cpp
    deribit_trader.cpp
    deribit_client.cpp
    market_data_manager.cpp
    benchmark_tool.cpp
)

# Add header files
set(HEADERS
    deribit_trader.h
    websocket_handler.h
    websocket_server.h
    trade_execution.h
    latency_module.h
    error_handler.h
    performance_monitor.h
    benchmark.h
    performance_dashboard.h
    config_manager.h
    strategy_manager.h
    market_data_manager.h
    risk_manager.h
    config_loader.h
)

# Add test files
set(TEST_SOURCES
    websocket_server_test.cpp
    benchmark_test.cpp
    performance_dashboard_test.cpp
)

# Create main executable
add_executable(deribit_trader ${SOURCES})

# Create test executable
add_executable(websocket_server_test 
    ${TEST_SOURCES}
    websocket_server.cpp
    error_handler.cpp
    latency_module.cpp
)

# Create example executable
add_executable(basic_trading_example examples/basic_trading.cpp ${SOURCES})

# Create benchmark executable
add_executable(benchmark_tool benchmark_tool.cpp ${SOURCES})

# Link libraries for main executable
target_link_libraries(deribit_trader
    PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Link libraries for test executable
target_link_libraries(websocket_server_test
    PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    GTest::gtest_main
)

# Link libraries for example executable
target_link_libraries(basic_trading_example
    PRIVATE
    Boost::system
    Boost::thread
    Boost::chrono
    Boost::atomic
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    psapi
)

# Link libraries for benchmark executable
target_link_libraries(benchmark_tool
    PRIVATE
    Boost::system
    Boost::thread
    Boost::chrono
    Boost::atomic
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    psapi
)

# Include directories for all targets
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Enable testing
enable_testing()

# Add tests to test suite
add_test(NAME websocket_server_test COMMAND websocket_server_test)
add_test(NAME benchmark_test COMMAND websocket_server_test --gtest_filter=BenchmarkTest.*)
add_test(NAME performance_dashboard_test COMMAND websocket_server_test --gtest_filter=PerformanceDashboardTest.*)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add compiler flags for optimization
if(MSVC)
    target_compile_options(deribit_trader PRIVATE /O2 /Oi /Ot /GL)
    target_compile_options(websocket_server_test PRIVATE /O2 /Oi /Ot /GL)
    target_compile_options(basic_trading_example PRIVATE /O2 /Oi /Ot /GL)
    target_compile_options(benchmark_tool PRIVATE /O2 /Oi /Ot /GL)
else()
    target_compile_options(deribit_trader PRIVATE -O3 -march=native)
    target_compile_options(websocket_server_test PRIVATE -O3 -march=native)
    target_compile_options(basic_trading_example PRIVATE -O3 -march=native)
    target_compile_options(benchmark_tool PRIVATE -O3 -march=native)
endif()

# Add compiler definitions
target_compile_definitions(deribit_trader PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_ASIO_STANDALONE
    ENABLE_PERFORMANCE_DASHBOARD
)

target_compile_definitions(basic_trading_example PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_ASIO_STANDALONE
)

target_compile_definitions(benchmark_tool PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_ASIO_STANDALONE
)

# Add compiler definitions for test executable
target_compile_definitions(websocket_server_test PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_ASIO_STANDALONE
    _WIN32_WINNT=0x0A00
    WIN64
    _AMD64_
)

# Set output directory for all targets
set_target_properties(deribit_trader basic_trading_example benchmark_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Set output directory for test executable
set_target_properties(websocket_server_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install targets
install(TARGETS deribit_trader basic_trading_example benchmark_tool
    RUNTIME DESTINATION bin
)




