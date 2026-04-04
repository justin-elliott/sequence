include(GoogleTest)

set(CMAKE_BUILD_TYPE "Debug")

add_executable(
    SequenceTest
    sequence_test.cpp
)
target_compile_definitions(
    SequenceTest PRIVATE
    CHECKED_ITERATORS=1
)
target_compile_options(
    SequenceTest PRIVATE
    -fprofile-instr-generate 
    -fcoverage-mapping
)
target_link_libraries(
    SequenceTest
    Sequence
    GTest::gmock
    GTest::gtest_main 
)
target_link_options(
    SequenceTest PRIVATE 
    -fprofile-instr-generate 
    -fcoverage-mapping
)
add_test(
    NAME SequenceTest
    COMMAND $<TARGET_FILE:SequenceTest>
)

gtest_discover_tests(SequenceTest)

find_program(LLVM_PROFDATA llvm-profdata REQUIRED)
find_program(LLVM_COV llvm-cov REQUIRED)
find_program(BROWSER NAMES x-www-browser REQUIRED)

add_custom_target(
  coverage
  COMMAND ${LLVM_PROFDATA} merge -o ${CMAKE_BINARY_DIR}/default.profdata $<TARGET_FILE_DIR:SequenceTest>/default.profraw
  COMMAND ${LLVM_COV} show -Xdemangler=c++filt -instr-profile=default.profdata $<TARGET_FILE:SequenceTest> -format=html -output-dir=coverage_report
  COMMAND echo ${CMAKE_BINARY_DIR}/coverage-report/index.html
  COMMAND ${BROWSER} coverage_report/index.html
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM USES_TERMINAL
)
add_dependencies(coverage SequenceTest)
