cmake_minimum_required(VERSION 3.25)

project(rembg_gui_cpp LANGUAGES CXX)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Network Test)
include(GNUInstallDirs)

get_filename_component(REMBG_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(REMBG_ONNXRUNTIME_VERSION "1.23.2" CACHE STRING "ONNX Runtime binary release version")
set(ONNXRUNTIME_ROOT "" CACHE PATH "Path to an extracted official ONNX Runtime binary release")

function(rembg_configure_onnxruntime)
  if(NOT ONNXRUNTIME_ROOT)
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(ort_package "onnxruntime-win-x64-${REMBG_ONNXRUNTIME_VERSION}")
      set(ort_archive "${ort_package}.zip")
    elseif(APPLE)
      set(ort_package "onnxruntime-osx-universal2-${REMBG_ONNXRUNTIME_VERSION}")
      set(ort_archive "${ort_package}.tgz")
    elseif(UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
      set(ort_package "onnxruntime-linux-x64-${REMBG_ONNXRUNTIME_VERSION}")
      set(ort_archive "${ort_package}.tgz")
    else()
      message(FATAL_ERROR "Set ONNXRUNTIME_ROOT to an extracted official ONNX Runtime release for this platform.")
    endif()

    set(ort_download_dir "${CMAKE_BINARY_DIR}/_deps/onnxruntime")
    set(ort_archive_path "${ort_download_dir}/${ort_archive}")
    set(ort_extract_path "${ort_download_dir}/${ort_package}")
    set(ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${REMBG_ONNXRUNTIME_VERSION}/${ort_archive}")

    file(MAKE_DIRECTORY "${ort_download_dir}")
    if(NOT EXISTS "${ort_archive_path}")
      message(STATUS "Downloading ONNX Runtime ${REMBG_ONNXRUNTIME_VERSION} from ${ort_url}")
      file(DOWNLOAD "${ort_url}" "${ort_archive_path}" SHOW_PROGRESS STATUS ort_download_status)
      list(GET ort_download_status 0 ort_download_code)
      list(GET ort_download_status 1 ort_download_message)
      if(NOT ort_download_code EQUAL 0)
        message(FATAL_ERROR "Failed to download ONNX Runtime: ${ort_download_message}")
      endif()
    endif()

    if(NOT EXISTS "${ort_extract_path}")
      execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${ort_archive_path}"
        WORKING_DIRECTORY "${ort_download_dir}"
        COMMAND_ERROR_IS_FATAL ANY
      )
    endif()

    set(ONNXRUNTIME_ROOT "${ort_extract_path}" CACHE PATH "Path to an extracted official ONNX Runtime binary release" FORCE)
  endif()

  add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
  set_target_properties(onnxruntime::onnxruntime
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_ROOT}/include"
  )

  if(WIN32)
    set(ort_library "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll")
    set_target_properties(onnxruntime::onnxruntime
      PROPERTIES
        IMPORTED_IMPLIB "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib"
        IMPORTED_LOCATION "${ort_library}"
    )
    set(ort_runtime_files
      "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
      "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_shared.dll"
    )
  elseif(APPLE)
    set(ort_library "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.dylib")
    set_target_properties(onnxruntime::onnxruntime PROPERTIES IMPORTED_LOCATION "${ort_library}")
    file(GLOB ort_runtime_files "${ONNXRUNTIME_ROOT}/lib/libonnxruntime*.dylib")
  else()
    set(ort_library "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
    set_target_properties(onnxruntime::onnxruntime PROPERTIES IMPORTED_LOCATION "${ort_library}")
    file(GLOB ort_runtime_files "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so*")
  endif()

  if(NOT EXISTS "${ort_library}")
    message(FATAL_ERROR "ONNX Runtime library not found under ONNXRUNTIME_ROOT: ${ort_library}")
  endif()

  set(REMBG_ONNXRUNTIME_RUNTIME_FILES "${ort_runtime_files}" PARENT_SCOPE)
endfunction()

rembg_configure_onnxruntime()

if(WIN32)
  set(REMBG_ONNXRUNTIME_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
else()
  set(REMBG_ONNXRUNTIME_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(rembg_native_lib
  src/BackgroundRemovalEngine.cpp
  src/ImageProcessing.cpp
  src/ImageViewSync.cpp
  src/MainWindow.cpp
  src/ModelManager.cpp
  src/PanScrollArea.cpp
  src/RemovalWorker.cpp
)

target_include_directories(rembg_native_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(rembg_native_lib PUBLIC cxx_std_23)

target_link_libraries(rembg_native_lib
  PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Network
    onnxruntime::onnxruntime
)

if(MSVC)
  target_compile_options(rembg_native_lib PRIVATE /permissive- /W4)
else()
  target_compile_options(rembg_native_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

function(rembg_import_qt_static_plugins target)
  if(NOT COMMAND qt_import_plugins)
    return()
  endif()

  set(platform_plugins)
  foreach(plugin IN ITEMS
      Qt6::QXcbIntegrationPlugin
      Qt6::QMinimalIntegrationPlugin
      Qt6::QOffscreenIntegrationPlugin
      Qt6::QLinuxFbIntegrationPlugin
      Qt6::QEglFSIntegrationPlugin
      Qt6::QVncIntegrationPlugin
  )
    if(TARGET ${plugin})
      list(APPEND platform_plugins ${plugin})
    endif()
  endforeach()

  set(image_plugins)
  foreach(plugin IN ITEMS
      Qt6::QGifPlugin
      Qt6::QICOPlugin
      Qt6::QJpegPlugin
  )
    if(TARGET ${plugin})
      list(APPEND image_plugins ${plugin})
    endif()
  endforeach()

  set(tls_plugins)
  foreach(plugin IN ITEMS
      Qt6::QTlsBackendOpenSSLPlugin
      Qt6::QSchannelBackendPlugin
      Qt6::QSecureTransportBackendPlugin
      Qt6::QTlsBackendCertOnlyPlugin
  )
    if(TARGET ${plugin})
      list(APPEND tls_plugins ${plugin})
    endif()
  endforeach()

  set(plugin_imports)
  if(platform_plugins)
    list(APPEND plugin_imports platforms ${platform_plugins})
  endif()
  if(image_plugins)
    list(APPEND plugin_imports imageformats ${image_plugins})
  endif()
  if(tls_plugins)
    list(APPEND plugin_imports tls ${tls_plugins})
  endif()

  if(plugin_imports)
    qt_import_plugins(${target} INCLUDE_BY_TYPE ${plugin_imports})
  endif()
endfunction()

add_executable(rembg-gui-native src/main.cpp)
target_link_libraries(rembg-gui-native PRIVATE rembg_native_lib)
rembg_import_qt_static_plugins(rembg-gui-native)
set_target_properties(rembg-gui-native PROPERTIES OUTPUT_NAME rembg-gui)

add_custom_command(TARGET rembg-gui-native POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          ${REMBG_ONNXRUNTIME_RUNTIME_FILES}
          $<TARGET_FILE_DIR:rembg-gui-native>
)

if(WIN32)
  set_target_properties(rembg-gui-native PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
  set_target_properties(rembg-gui-native PROPERTIES MACOSX_BUNDLE TRUE)
elseif(UNIX)
  set_target_properties(rembg-gui-native PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
  )
endif()

install(TARGETS rembg-gui-native
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
  install(FILES $<TARGET_RUNTIME_DLLS:rembg-gui-native> DESTINATION ${CMAKE_INSTALL_BINDIR})
  if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(REMBG_VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  else()
    set(REMBG_VCPKG_BIN_DIR "")
  endif()

  install(CODE "
    set(runtime_dirs \"$<TARGET_FILE_DIR:rembg-gui-native>\")
    if(EXISTS \"${REMBG_VCPKG_BIN_DIR}\")
      list(APPEND runtime_dirs \"${REMBG_VCPKG_BIN_DIR}\")
    endif()

    foreach(runtime_dir IN LISTS runtime_dirs)
      file(GLOB runtime_dlls \"\${runtime_dir}/*.dll\")
      foreach(runtime_dll IN LISTS runtime_dlls)
        get_filename_component(runtime_name \"\${runtime_dll}\" NAME)
        if(NOT runtime_name STREQUAL \"Qt6Test.dll\")
          file(INSTALL
            DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
            TYPE FILE
            FILES \"\${runtime_dll}\"
          )
        endif()
      endforeach()
    endforeach()
  ")
endif()

install(FILES ${REMBG_ONNXRUNTIME_RUNTIME_FILES} DESTINATION ${REMBG_ONNXRUNTIME_INSTALL_DIR})

install(FILES ${REMBG_REPO_ROOT}/gui.py DESTINATION .)

if(UNIX AND NOT APPLE)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/rembg-gui.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
  )
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/rembg-gui.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
  )
endif()

if(COMMAND qt_generate_deploy_app_script)
  qt_generate_deploy_app_script(
    TARGET rembg-gui-native
    OUTPUT_SCRIPT rembg_gui_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT ${rembg_gui_deploy_script})
endif()

function(rembg_resolve_qt_plugin_dir output_variable)
  if(TARGET Qt6::qmake)
    get_target_property(qmake_executable Qt6::qmake IMPORTED_LOCATION)
    if(qmake_executable)
      execute_process(
        COMMAND "${qmake_executable}" -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE qmake_plugin_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE qmake_result
      )
      if(qmake_result EQUAL 0 AND qmake_plugin_dir)
        file(TO_CMAKE_PATH "${qmake_plugin_dir}" qmake_plugin_dir)
        set(${output_variable} "${qmake_plugin_dir}" PARENT_SCOPE)
        return()
      endif()
    endif()
  endif()

  if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(${output_variable} "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins" PARENT_SCOPE)
    return()
  endif()

  set(${output_variable} "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/Qt6/plugins" PARENT_SCOPE)
endfunction()

if(BUILD_TESTING)
  if(WIN32 OR APPLE)
    set(REMBG_ENABLE_GUI_SMOKE_TESTS_DEFAULT ON)
  else()
    set(REMBG_ENABLE_GUI_SMOKE_TESTS_DEFAULT OFF)
  endif()
  option(
    REMBG_ENABLE_GUI_SMOKE_TESTS
    "Build QWidget smoke tests that require a Qt platform plugin"
    ${REMBG_ENABLE_GUI_SMOKE_TESTS_DEFAULT}
  )

  file(TO_CMAKE_PATH "${REMBG_REPO_ROOT}/example.png" REMBG_NATIVE_SAMPLE_IMAGE)
  add_executable(rembg-native-tests
    tests/ImageProcessingTests.cpp
    tests/IntegrationTests.cpp
    tests/TestMain.cpp
  )
  target_compile_features(rembg-native-tests PRIVATE cxx_std_23)
  target_compile_definitions(rembg-native-tests
    PRIVATE
      REMBG_NATIVE_SAMPLE_IMAGE="${REMBG_NATIVE_SAMPLE_IMAGE}"
  )
  target_link_libraries(rembg-native-tests PRIVATE rembg_native_lib Qt6::Test)
  add_custom_command(TARGET rembg-native-tests POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${REMBG_ONNXRUNTIME_RUNTIME_FILES}
            $<TARGET_FILE_DIR:rembg-native-tests>
  )
  add_test(NAME rembg-native-tests COMMAND rembg-native-tests)
  set(REMBG_TEST_ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    "PATH=path_list_prepend:$<TARGET_FILE_DIR:onnxruntime::onnxruntime>"
  )
  rembg_resolve_qt_plugin_dir(REMBG_QT_PLUGIN_DIR)
  set(REMBG_TEST_ENVIRONMENT
    "QT_PLUGIN_PATH=${REMBG_QT_PLUGIN_DIR}"
  )
  rembg_import_qt_static_plugins(rembg-native-tests)
  set_tests_properties(rembg-native-tests
    PROPERTIES
      TIMEOUT 600
      ENVIRONMENT "${REMBG_TEST_ENVIRONMENT}"
      ENVIRONMENT_MODIFICATION "${REMBG_TEST_ENVIRONMENT_MODIFICATION}"
  )

  if(REMBG_ENABLE_GUI_SMOKE_TESTS)
    add_executable(rembg-gui-smoke-tests
      tests/GuiSmokeTestMain.cpp
      tests/MainWindowSmokeTest.cpp
    )
    target_compile_features(rembg-gui-smoke-tests PRIVATE cxx_std_23)
    target_link_libraries(rembg-gui-smoke-tests PRIVATE rembg_native_lib Qt6::Test)
    rembg_import_qt_static_plugins(rembg-gui-smoke-tests)
    add_custom_command(TARGET rembg-gui-smoke-tests POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              ${REMBG_ONNXRUNTIME_RUNTIME_FILES}
              $<TARGET_FILE_DIR:rembg-gui-smoke-tests>
    )
    add_test(NAME rembg-gui-smoke-tests COMMAND rembg-gui-smoke-tests)

    rembg_resolve_qt_plugin_dir(REMBG_QT_PLUGIN_DIR)
    set(REMBG_QT_PLATFORM_PLUGIN_DIR "${REMBG_QT_PLUGIN_DIR}/platforms")
    set(REMBG_GUI_TEST_ENVIRONMENT
      "QT_QPA_PLATFORM=minimal"
      "QT_PLUGIN_PATH=${REMBG_QT_PLUGIN_DIR}"
      "QT_QPA_PLATFORM_PLUGIN_PATH=${REMBG_QT_PLATFORM_PLUGIN_DIR}"
    )
    set_tests_properties(rembg-gui-smoke-tests
      PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "${REMBG_GUI_TEST_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "${REMBG_TEST_ENVIRONMENT_MODIFICATION}"
    )
  endif()
endif()
