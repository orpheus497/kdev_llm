# CMake generated Testfile for 
# Source directory: /home/orpheus497/Documents/Projects/kdev_llm
# Build directory: /home/orpheus497/Documents/Projects/kdev_llm/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(appstreamtest "/usr/bin/cmake" "-DAPPSTREAMCLI=/usr/bin/appstreamcli" "-DINSTALL_FILES=/home/orpheus497/Documents/Projects/kdev_llm/build/install_manifest.txt" "-P" "/usr/share/ECM/kde-modules/appstreamtest.cmake")
set_tests_properties(appstreamtest PROPERTIES  _BACKTRACE_TRIPLES "/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;177;add_test;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;195;appstreamtest;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;0;;/home/orpheus497/Documents/Projects/kdev_llm/CMakeLists.txt;12;include;/home/orpheus497/Documents/Projects/kdev_llm/CMakeLists.txt;0;")
subdirs("src")
