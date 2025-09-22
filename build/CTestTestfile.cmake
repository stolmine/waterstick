# CMake generated Testfile for 
# Source directory: /Users/why/repos/waterstick
# Build directory: /Users/why/repos/waterstick/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(SmoothingPerformanceTest "/Users/why/repos/waterstick/build/bin/Release/smoothing_performance_test")
set_tests_properties(SmoothingPerformanceTest PROPERTIES  _BACKTRACE_TRIPLES "/Users/why/repos/waterstick/CMakeLists.txt;126;add_test;/Users/why/repos/waterstick/CMakeLists.txt;0;")
subdirs("vst3sdk")
