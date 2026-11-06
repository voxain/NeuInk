add_executable(KOReaderTrustTest
  KOReaderTrustTest.cpp
)

target_include_directories(KOReaderTrustTest PRIVATE
  ${REPO_ROOT}/lib/KOReaderSync
)

target_link_libraries(KOReaderTrustTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(KOReaderTrustTest)
