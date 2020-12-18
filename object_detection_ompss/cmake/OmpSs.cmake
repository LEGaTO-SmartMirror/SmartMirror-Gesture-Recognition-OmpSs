function(INIT_OMPSS)
    if(DEFINED ENV{OMPSS_HOME})
        set(OMPSS_HOME $ENV{OMPSS_HOME})
        message(STATUS "OMPSS_HOME=${OMPSS_HOME}")
        set(MCC ${OMPSS_HOME}/bin/mcc CACHE INTERNAL "C Mercurium Compiler")
        set(MCXX ${OMPSS_HOME}/bin/mcxx CACHE INTERNAL "C++ Mercurium Compiler")
        set(LIBS_MCXX "" CACHE INTERNAL "List of libraries for linking") # Reset the link library list
        set(INCS_MCXX "" CACHE INTERNAL "List of include dirs for compiling") # Reset the include directory list
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
        message(WARNING "ADD_LIBRARY_DIR_OMPSS called without any library dirs")
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

function(ADD_INCLUDE_DIR_OMPSS)
    if(${ARGC} EQUAL 0)
        message(WARNING "ADD_INCLUDE_OMPSS called without any include dirs")
        return()
    endif()

    foreach(ARG IN LISTS ARGV)
        list(LENGTH ${ARG} LL)
        # A list of length 0 is a single string
        if(${LL} GREATER_EQUAL 1)
            foreach(INC IN LISTS ${ARG})
                list(APPEND INCS_MCXX "-I${INC}")
            endforeach()
        else()
            list(APPEND INCS_MCXX "-I${ARG}")
        endif()
    endforeach()

    set(INCS_MCXX ${INCS_MCXX} CACHE INTERNAL "List of include dirs for compiling")
endfunction(ADD_INCLUDE_DIR_OMPSS)


function(ADD_EXECUTABLE_OMPSS)
    # set(options OPTIONAL)
    set(oneValueArgs TARGET_NAME MAIN OUTPUT_NAME)
    set(multiValueArgs C_SRC CXX_SRC)
    cmake_parse_arguments(OMPSS_EXE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED OMPSS_EXE_MAIN)
        message(FATAL_ERROR "[ADD_OMPSS_EXECUTABLE]: Argument MAIN not specified, please provide a valid main.c file.")
    endif()

    if(NOT DEFINED OMPSS_EXE_OUTPUT_NAME)
        message(FATAL_ERROR "[ADD_OMPSS_EXECUTABLE]: Argument OUTPUT_NAME not specified, please provide a valid output name.")
    endif()

    if(NOT DEFINED OMPSS_EXE_TARGET_NAME)
        set(OMPSS_EXE_TARGET_NAME MAIN_MCC)
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
        COMMAND ${MCC} -c --ompss-2 ${OMPSS_EXE_MAIN} -o ${MCC_OUTPUT_OBJ} ${INCS_MCXX}
        DEPENDS ${MCC_DEPENDS} ${OMPSS_EXE_MAIN}
        COMMENT "Compiling mcc main"
    )

    if(DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(FULL_OUTPUT_NAME "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OMPSS_EXE_OUTPUT_NAME}")
    else()
        set(FULL_OUTPUT_NAME "${OMPSS_EXE_OUTPUT_NAME}")
    endif()

    # Link everything using MCXX
    ADD_CUSTOM_COMMAND(OUTPUT ${OMPSS_EXE_OUTPUT_NAME}
        COMMAND ${MCXX} --ompss-2 ${MCC_OUTPUT_OBJ} ${ADDITIONAL_OBJS} -o ${FULL_OUTPUT_NAME} ${LIBS_MCXX}
        DEPENDS ${MCC_OUTPUT_OBJ}
        COMMENT "Linking mcc main"
    )

    ADD_CUSTOM_TARGET(${OMPSS_EXE_TARGET_NAME} ALL
        DEPENDS ${MCC_OUTPUT_OBJ} ${OMPSS_EXE_OUTPUT_NAME}
        VERBATIM
    )
endfunction(ADD_EXECUTABLE_OMPSS)

function(COPY_BINARY_TO_HOST TARGET HOST USER REMOTE_PATH EXECUTABLE)
    if(DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(FULL_OUTPUT_NAME "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE}")
    else()
        set(FULL_OUTPUT_NAME "${EXECUTABLE}")
    endif()
    
    ADD_CUSTOM_TARGET(SCP ALL
        COMMAND scp ${FULL_OUTPUT_NAME} ${USER}@${HOST}:${REMOTE_PATH}/
        DEPENDS ${TARGET}
        COMMENT "Copying binary to remote host"
    )
endfunction(COPY_BINARY_TO_HOST)
