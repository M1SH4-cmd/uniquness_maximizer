cmake_minimum_required(VERSION 3.16)

project(uniquness_maximizer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        #source code:
        src/main.cpp
        src/mainwindow.cpp
        src/maximizer.cpp
        src/core/document.cpp
        src/document/docx_reader.cpp
        src/document/document_chunker.cpp
        src/analysis/prompt_builder.cpp
        src/analysis/response_parser.cpp
        src/analysis/analysis_engine.cpp
        src/providers/ollama_provider.cpp

        #headers:
        include/mainwindow.h
        include/maximizer.h
        include/core/document.h
        include/core/document_chunk.h
        include/document/docx_reader.h
        include/document/document_chunker.h
        include/core/ai_types.h
        include/core/analysis_types.h
        include/analysis/prompt_builder.h
        include/analysis/response_parser.h
        include/analysis/analysis_engine.h
        include/providers/ai_provider.h
        include/providers/ollama_provider.h

        #qrc files:
        res_file.qrc

        #pugixml lib:
        lib/pugixml/pugixml.cpp

        #external libs:

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(uniquness_maximizer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

    )
else()
    if(ANDROID)
        add_library(uniquness_maximizer SHARED
          ${PROJECT_SOURCES}
        )
    else()
        add_executable(uniquness_maximizer
          ${PROJECT_SOURCES}
        )
    endif()
endif()

if(WIN32)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")

    if(EXISTS "${APP_ICON}")
        message(STATUS "Found icon file: ${APP_ICON}")

        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources/icon.rc")
        file(WRITE ${RC_FILE} "IDI_ICON1 ICON \"${APP_ICON}\"")

        target_sources(uniquness_maximizer PRIVATE ${RC_FILE})

        set_target_properties(uniquness_maximizer PROPERTIES
            WIN32_EXECUTABLE TRUE
            LINK_FLAGS "/MANIFEST:NO"
        )
    else()
        message(WARNING "Icon file not found: ${APP_ICON}")
    endif()
endif()

target_link_libraries(uniquness_maximizer PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Network
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.lib
)
target_include_directories(uniquness_maximizer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/include
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.uniquness_maximizer)
endif()
set_target_properties(uniquness_maximizer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS uniquness_maximizer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(uniquness_maximizer)
endif()

add_custom_command(TARGET uniquness_maximizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.dll
        $<TARGET_FILE_DIR:uniquness_maximizer>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.dll
        $<TARGET_FILE_DIR:uniquness_maximizer>
)

include(CTest)
if(BUILD_TESTING)
    add_executable(docx_reader_test
        tests/docx_reader_test.cpp
        src/core/document.cpp
        src/document/docx_reader.cpp
        lib/pugixml/pugixml.cpp
    )
    target_link_libraries(docx_reader_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.lib
    )
    target_include_directories(docx_reader_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/include
    )
    add_test(NAME docx_reader_test COMMAND docx_reader_test)

    add_executable(document_chunker_test
        tests/document_chunker_test.cpp
        src/core/document.cpp
        src/document/document_chunker.cpp
    )
    target_link_libraries(document_chunker_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
    )
    target_include_directories(document_chunker_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME document_chunker_test COMMAND document_chunker_test)

    add_executable(analysis_contract_test
        tests/analysis_contract_test.cpp
        src/analysis/prompt_builder.cpp
        src/analysis/response_parser.cpp
    )
    target_link_libraries(analysis_contract_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
    )
    target_include_directories(analysis_contract_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME analysis_contract_test COMMAND analysis_contract_test)

    add_executable(analysis_engine_test
        tests/analysis_engine_test.cpp
        src/core/document.cpp
        src/document/document_chunker.cpp
        src/analysis/prompt_builder.cpp
        src/analysis/response_parser.cpp
        src/analysis/analysis_engine.cpp
        include/analysis/analysis_engine.h
    )
    target_link_libraries(analysis_engine_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
    )
    target_include_directories(analysis_engine_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME analysis_engine_test COMMAND analysis_engine_test)

    add_executable(ollama_provider_test
        tests/ollama_provider_test.cpp
        src/providers/ollama_provider.cpp
        include/providers/ollama_provider.h
    )
    target_link_libraries(ollama_provider_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
    )
    target_include_directories(ollama_provider_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME ollama_provider_test COMMAND ollama_provider_test)

    add_executable(ollama_integration_test
        tests/ollama_integration_test.cpp
        src/providers/ollama_provider.cpp
        include/providers/ollama_provider.h
        src/analysis/prompt_builder.cpp
        src/analysis/response_parser.cpp
    )
    target_link_libraries(ollama_integration_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
    )
    target_include_directories(ollama_integration_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME ollama_integration_test COMMAND ollama_integration_test)

    add_executable(document_test
        tests/document_test.cpp
        src/core/document.cpp
    )
    target_link_libraries(document_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
    )
    target_include_directories(document_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME document_test COMMAND document_test)

    add_executable(docx_reader_errors_test
        tests/docx_reader_errors_test.cpp
        src/core/document.cpp
        src/document/docx_reader.cpp
        lib/pugixml/pugixml.cpp
    )
    target_link_libraries(docx_reader_errors_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.lib
    )
    target_include_directories(docx_reader_errors_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/include
    )
    add_test(NAME docx_reader_errors_test COMMAND docx_reader_errors_test)

    add_executable(ollama_endpoints_test
        tests/ollama_endpoints_test.cpp
        src/providers/ollama_provider.cpp
        include/providers/ollama_provider.h
    )
    target_link_libraries(ollama_endpoints_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
    )
    target_include_directories(ollama_endpoints_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME ollama_endpoints_test COMMAND ollama_endpoints_test)

    add_executable(mainwindow_keystore_test
        tests/mainwindow_keystore_test.cpp
        src/mainwindow.cpp
        include/mainwindow.h
    )
    target_link_libraries(mainwindow_keystore_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
    )
    target_include_directories(mainwindow_keystore_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME mainwindow_keystore_test COMMAND mainwindow_keystore_test)
endif()
