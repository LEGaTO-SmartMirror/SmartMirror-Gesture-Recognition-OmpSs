function(INIT_OMPSS)
    if(DEFINED ENV{OMPSS_HOME})
        set(OMPSS_HOME $ENV{OMPSS_HOME})
        message("OMPSS_HOME=${OMPSS_HOME}")
        set(NOS6_INCLUDE_DIRS ${OMPSS_HOME}/include)
        set(NOS6_LIBRARY_PATH ${OMPSS_HOME}/lib)
        set(MCC ${OMPSS_HOME}/bin/mcc CACHE INTERNAL "C Mercurium Compiler")
        set(MCXX ${OMPSS_HOME}/bin/mcxx CACHE INTERNAL "C++ Mercurium Compiler")
        set(LIBS_MCXX "" CACHE INTERNAL "List of libraries for linking") # Reset the link library list
    else()
        message(FATAL_ERROR "Environment variable OMPSS_HOME not set")
    endif()
endfunction(INIT_OMPSS)

function(PROCESS_LIB_STR)
    if(${ARGC} EQUAL 0)
        message(WARNING "PROCESS_LIB_STR called without any arguments")
        return()
    endif()

    set(LIB ${ARGV0})
    set(LIB_STR "-l${LIB}")

    if(LIB MATCHES "^-l.*" OR LIB MATCHES "\.a$")
        set(LIB_STR "${LIB}")
    elseif(LIB MATCHES "\/")
        # Replace '/' with ';' to create a cmake list
        string(REPLACE "\/" ";" ll ${LIB})
        # The last item in the list will be the name of the library
        list(GET ll -1 item)
        # Get the path to the library
        string(REPLACE "\/${item}" "" lp ${LIB})

        # Append the library path to the library list
        list(APPEND LIBS_MCXX "-L${lp}")

        # If the library name starts with "lib" remove it to prevent compiler errors  
        if(item MATCHES "^lib")
            string(REGEX REPLACE "^lib" "" item ${item})
        endif()
        # If the library name ends with ".so" remove it to prevent compiler errors
        if(item MATCHES ".so$")
            string(REGEX REPLACE ".so$" "" item ${item})
        endif()
        # Set the library string to add to the library list
        set(LIB_STR "-l${item}")
    endif()

    set(LIB_STR ${LIB_STR} PARENT_SCOPE)
endfunction(PROCESS_LIB_STR)

function(ADD_LIBRARY_DIR_OMPSS)
    if(${ARGC} EQUAL 0)
        message(WARNING "ADD_LIBRARY_DIR_OMPSS called without any libraries")
        return()
    endif()

    foreach(ARG IN LISTS ARGV)
        list(LENGTH ${ARG} LL)
        # A list of length 0 is a single string
        if(${LL} GREATER_EQUAL 1)
            foreach(LIB IN LISTS ${ARG})
                list(APPEND LIBS_MCXX "-L${LIB}")
            endforeach()
        else()
            list(APPEND LIBS_MCXX "-L${ARG}")
        endif()
    endforeach()

    set(LIBS_MCXX ${LIBS_MCXX} CACHE INTERNAL "List of libraries for linking")
endfunction(ADD_LIBRARY_DIR_OMPSS)

function(ADD_LIBRARY_OMPSS)
    if(${ARGC} EQUAL 0)
        message(WARNING "ADD_LIBRARY_OMPSS called without any libraries")
        return()
    endif()

    foreach(ARG IN LISTS ARGV)
        list(LENGTH ${ARG} LL)
        # A list of length 0 is a single string
        if(${LL} GREATER_EQUAL 1)
            foreach(LIB IN LISTS ${ARG})
                PROCESS_LIB_STR(${LIB})
                list(APPEND LIBS_MCXX ${LIB_STR})
            endforeach()
        else()
            PROCESS_LIB_STR(${ARG})
            list(APPEND LIBS_MCXX ${LIB_STR})
        endif()
    endforeach()

    set(LIBS_MCXX ${LIBS_MCXX} CACHE INTERNAL "List of libraries for linking")
endfunction(ADD_LIBRARY_OMPSS)

function(ADD_OMPSS_EXECUTABLE)
    # set(options OPTIONAL)
    set(oneValueArgs MAIN OUTPUT_NAME)
    set(multiValueArgs C_SRC CXX_SRC)
    cmake_parse_arguments(OMPSS_EXE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED OMPSS_EXE_MAIN)
        message(FATAL_ERROR "[ADD_OMPSS_EXECUTABLE]: Argument MAIN not specified, please provide a valid main.c file.")
    endif()

    if(NOT DEFINED OMPSS_EXE_OUTPUT_NAME)
        message(FATAL_ERROR "[ADD_OMPSS_EXECUTABLE]: Argument OUTPUT_NAME not specified, please provide a valid output name.")
    endif()

    set(MCC_OUTPUT_OBJ "mcc_main.o")

    # Compile CPP sources using GCC
    if(DEFINED OMPSS_EXE_CXX_SRC)
        add_library(cpp_obj OBJECT ${OMPSS_EXE_CXX_SRC})
        set_target_properties(cpp_obj PROPERTIES LINKER_LANGUAGE CXX)
        list(APPEND ADDITIONAL_OBJS $<TARGET_OBJECTS:cpp_obj>)
        set(MCC_DEPENDS cpp_obj)
    endif()

    # Compile C main using MCC
    ADD_CUSTOM_COMMAND(OUTPUT ${MCC_OUTPUT_OBJ}
        COMMAND ${MCC} -c --ompss-2 ${OMPSS_EXE_MAIN} -o ${MCC_OUTPUT_OBJ}
        DEPENDS ${MCC_DEPENDS}
        COMMENT "Compiling mcc main"
    )

    # Link everything using MCXX
    ADD_CUSTOM_COMMAND(OUTPUT ${OMPSS_EXE_OUTPUT_NAME}
        COMMAND ${MCXX} --ompss-2 ${MCC_OUTPUT_OBJ} ${ADDITIONAL_OBJS} -o ${OMPSS_EXE_OUTPUT_NAME} ${LIBS_MCXX}
        DEPENDS ${MCC_OUTPUT_OBJ}
        COMMENT "Linking mcc main"
    )

    ADD_CUSTOM_TARGET(MAIN_MCC ALL
        DEPENDS ${MCC_OUTPUT_OBJ} ${OMPSS_EXE_OUTPUT_NAME}
        VERBATIM
    )
endfunction(ADD_OMPSS_EXECUTABLE)
