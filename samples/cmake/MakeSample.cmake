function(MakeSample NAME)
	add_executable(${NAME} ${NAME}.cpp)

	target_link_libraries(${NAME} PUBLIC
		Utils
		MinimalSocket
	)	
	
	set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${CMAKE_INSTALL_PREFIX}/bin/")
	
	install(TARGETS ${NAME})
endfunction()