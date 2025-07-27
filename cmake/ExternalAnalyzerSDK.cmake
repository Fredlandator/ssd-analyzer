# Use the C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY OR NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/)
endif()

# Load the Analyzer SDK - Prefer local installation over FetchContent
if(NOT TARGET Saleae::AnalyzerSDK)
    # Chercher d'abord un SDK installé localement
    if(EXISTS "${CMAKE_SOURCE_DIR}/AnalyzerSDK/AnalyzerSDKConfig.cmake")
        message(STATUS "Using local AnalyzerSDK from: ${CMAKE_SOURCE_DIR}/AnalyzerSDK")
        include(${CMAKE_SOURCE_DIR}/AnalyzerSDK/AnalyzerSDKConfig.cmake)
    else()
        # Fallback: Essayer FetchContent si le SDK local n'est pas trouvé
        message(STATUS "Local AnalyzerSDK not found, trying FetchContent...")
        
        include(FetchContent)
        
        FetchContent_Declare(
            analyzersdk
            GIT_REPOSITORY https://github.com/saleae/AnalyzerSDK.git
            GIT_TAG        master  # Utiliser master - c'est la branche par défaut
            GIT_SHALLOW    True
            GIT_PROGRESS   True
            TIMEOUT        120     # Augmenter le timeout
        )
        
        FetchContent_GetProperties(analyzersdk)
        
        if(NOT analyzersdk_POPULATED)
            message(STATUS "Downloading Saleae AnalyzerSDK...")
            FetchContent_Populate(analyzersdk)
            
            # Vérifier que le téléchargement a réussi
            if(NOT EXISTS "${analyzersdk_SOURCE_DIR}/AnalyzerSDKConfig.cmake")
                message(FATAL_ERROR "Failed to download AnalyzerSDK. Please download manually from https://github.com/saleae/AnalyzerSDK and place in ${CMAKE_SOURCE_DIR}/AnalyzerSDK/")
            endif()
            
            include(${analyzersdk_SOURCE_DIR}/AnalyzerSDKConfig.cmake)
        endif()
    endif()

    # Copier les bibliothèques si nécessaire
    if(APPLE OR WIN32)
        if(TARGET Saleae::AnalyzerSDK)
            get_target_property(analyzersdk_lib_location Saleae::AnalyzerSDK IMPORTED_LOCATION)
            if(analyzersdk_lib_location AND CMAKE_LIBRARY_OUTPUT_DIRECTORY)
                file(COPY ${analyzersdk_lib_location} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
            endif()
        endif()
    endif()
endif()

function(add_analyzer_plugin TARGET)
    set(options )
    set(single_value_args )
    set(multi_value_args SOURCES)
    cmake_parse_arguments( _p "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN} )

    add_library(${TARGET} MODULE ${_p_SOURCES})
    target_link_libraries(${TARGET} PRIVATE Saleae::AnalyzerSDK)

    set(ANALYZER_DESTINATION "Analyzers")
    install(TARGETS ${TARGET} RUNTIME DESTINATION ${ANALYZER_DESTINATION}
                              LIBRARY DESTINATION ${ANALYZER_DESTINATION})

    set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ANALYZER_DESTINATION}
                                               LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ANALYZER_DESTINATION})
endfunction()