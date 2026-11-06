add_executable(FirmwareTargetsTest
  FirmwareTargetsTest.cpp
)

target_include_directories(FirmwareTargetsTest PRIVATE
  ${REPO_ROOT}/src/network
)

target_link_libraries(FirmwareTargetsTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(FirmwareTargetsTest)
