function(MakeLibrary LIBRARY_NAME INCLUDE_DIR)
CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} SOURCES)
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if(LIB_OPT)
	if (WIN32)
		set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	endif ()	
	
	add_library(${LIBRARY_NAME} SHARED ${SOURCES})
else()
	add_library(${LIBRARY_NAME} STATIC ${SOURCES})
endif()

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}>
)

install(TARGETS ${LIBRARY_NAME})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/ DESTINATION include/${LIBRARY_NAME} FILES_MATCHING PATTERN "*.h*")
endfunction()