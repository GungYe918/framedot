# framedotInstall.cmake
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

function(framedot_setup_install)
  # Install headers
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/framedot/"
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/framedot")

  # Install generated config header
  install(FILES
    "${PROJECT_BINARY_DIR}/generated/framedot/core/Config.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/framedot/core"
  )

  # Export targets (the library target is created in src/)
  install(EXPORT framedotTargets
          NAMESPACE framedot::
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/framedot")

  # Package config
  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/framedotConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/framedotConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/framedot"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/framedotConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${PROJECT_BINARY_DIR}/framedotConfig.cmake"
    "${PROJECT_BINARY_DIR}/framedotConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/framedot"
  )
endfunction()