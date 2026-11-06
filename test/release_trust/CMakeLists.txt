add_executable(ReleaseTrustTest
  ReleaseTrustTest.cpp
)

target_include_directories(ReleaseTrustTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(ReleaseTrustTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(ReleaseTrustTest)
