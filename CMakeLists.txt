cmake_minimum_required(VERSION 3.16)
project(kdevllm VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REQUIRED_KF_VERSION "6.0.0")
find_package(ECM ${REQUIRED_KF_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Set default prefix to /usr to ensure KDevelop finds the plugin naturally
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Default install prefix for KDevelop plugin" FORCE)
endif()

# Force installation to the exact system Qt plugin directory
set(KDE_INSTALL_USE_QT_SYS_PATHS ON CACHE BOOL "Install plugins to system Qt plugin directory" FORCE)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF6 ${REQUIRED_KF_VERSION} REQUIRED COMPONENTS
    I18n
    TextEditor
    CoreAddons
    XmlGui
    SyntaxHighlighting
    Config
)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
)

option(BUILD_TESTING "Build the testing tree" OFF)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

find_package(KDevPlatform 6.0 REQUIRED)

add_subdirectory(src)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
