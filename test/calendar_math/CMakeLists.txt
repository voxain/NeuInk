add_executable(CalendarMathTest
  CalendarMathTest.cpp
)
target_include_directories(CalendarMathTest PRIVATE ${REPO_ROOT}/lib/hal)
target_link_libraries(CalendarMathTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(CalendarMathTest)
