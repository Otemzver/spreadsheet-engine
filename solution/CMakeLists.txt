cmake_minimum_required(VERSION 3.16)
project(Spreadsheet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

include(GNUInstallDirs)
find_package(Java 11 REQUIRED COMPONENTS Runtime)

set(ANTLR_JAR "${CMAKE_CURRENT_SOURCE_DIR}/antlr-4.12.0-complete.jar")
set(FORMULA_GRAMMAR "${CMAKE_CURRENT_SOURCE_DIR}/Formula.g4")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

if(NOT EXISTS "${ANTLR_JAR}")
    message(FATAL_ERROR "Missing ANTLR generator: ${ANTLR_JAR}")
endif()

set(GENERATED_SOURCES)
set(GENERATED_HEADERS)

foreach(name FormulaLexer FormulaParser FormulaListener FormulaBaseListener)
    list(APPEND GENERATED_SOURCES "${GENERATED_DIR}/${name}.cpp")
    list(APPEND GENERATED_HEADERS "${GENERATED_DIR}/${name}.h")
endforeach()

add_custom_command(
    OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR_JAR}" -Dlanguage=Cpp -listener -no-visitor -o "${GENERATED_DIR}" Formula.g4
    DEPENDS "${FORMULA_GRAMMAR}" "${ANTLR_JAR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating Formula parser"
    VERBATIM
)

add_custom_target(generate_formula DEPENDS ${GENERATED_SOURCES} ${GENERATED_HEADERS})

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR tests" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build shared ANTLR runtime" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "Build static ANTLR runtime" FORCE)

set(WITH_DEMO OFF CACHE BOOL "Build ANTLR demo" FORCE)
set(WITH_STATIC_CRT OFF CACHE BOOL "Use static Microsoft CRT" FORCE)

add_subdirectory(antlr4_runtime)

add_executable(spreadsheet
    main.cpp
    structures.cpp
    cell.cpp
    sheet.cpp
    formula.cpp
    FormulaAST.cpp
    FormulaParserFacade.cpp

    common.h
    cell.h
    sheet.h
    formula.h
    FormulaAST.h
    FormulaParserFacade.h

    ${GENERATED_SOURCES}
    ${GENERATED_HEADERS}
)

add_dependencies(spreadsheet generate_formula)

target_include_directories(spreadsheet PRIVATE "${GENERATED_DIR}")
target_include_directories(spreadsheet SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/antlr4_runtime/runtime/src")

target_link_libraries(spreadsheet PRIVATE antlr4_static)

if(MSVC)
    target_compile_options(spreadsheet PRIVATE /permissive- /utf-8 /UNDEBUG)
else()
    target_compile_options(spreadsheet PRIVATE -UNDEBUG)
endif()

enable_testing()
add_test(NAME spreadsheet_tests COMMAND spreadsheet)