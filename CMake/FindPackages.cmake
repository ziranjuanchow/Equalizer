# generated by Buildyard, do not edit.

include(System)
set(FIND_PACKAGES_FOUND ${SYSTEM} ${FIND_PACKAGES_FOUND_EXTRA})

find_package(VMMLIB 1.5.0 REQUIRED)
if(VMMLIB_FOUND)
  set(VMMLIB_name VMMLIB)
elseif(VMMLIB_FOUND)
  set(VMMLIB_name VMMLIB)
endif()
if(VMMLIB_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_VMMLIB)
  link_directories(${${VMMLIB_name}_LIBRARY_DIRS})
  include_directories(${${VMMLIB_name}_INCLUDE_DIRS})
endif()

find_package(Lunchbox 1.5.0 REQUIRED)
if(Lunchbox_FOUND)
  set(Lunchbox_name Lunchbox)
elseif(LUNCHBOX_FOUND)
  set(Lunchbox_name LUNCHBOX)
endif()
if(Lunchbox_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_LUNCHBOX)
  link_directories(${${Lunchbox_name}_LIBRARY_DIRS})
  include_directories(${${Lunchbox_name}_INCLUDE_DIRS})
endif()

find_package(Collage 0.7.0 REQUIRED)
if(Collage_FOUND)
  set(Collage_name Collage)
elseif(COLLAGE_FOUND)
  set(Collage_name COLLAGE)
endif()
if(Collage_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_COLLAGE)
  link_directories(${${Collage_name}_LIBRARY_DIRS})
  include_directories(${${Collage_name}_INCLUDE_DIRS})
endif()

find_package(OpenGL  REQUIRED)
if(OpenGL_FOUND)
  set(OpenGL_name OpenGL)
elseif(OPENGL_FOUND)
  set(OpenGL_name OPENGL)
endif()
if(OpenGL_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_OPENGL)
  link_directories(${${OpenGL_name}_LIBRARY_DIRS})
  include_directories(${${OpenGL_name}_INCLUDE_DIRS})
endif()

find_package(Boost 1.41.0 REQUIRED system regex date_time serialization)
if(Boost_FOUND)
  set(Boost_name Boost)
elseif(BOOST_FOUND)
  set(Boost_name BOOST)
endif()
if(Boost_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_BOOST)
  link_directories(${${Boost_name}_LIBRARY_DIRS})
  include_directories(SYSTEM ${${Boost_name}_INCLUDE_DIRS})
endif()

find_package(OpenMP  REQUIRED)
if(OpenMP_FOUND)
  set(OpenMP_name OpenMP)
elseif(OPENMP_FOUND)
  set(OpenMP_name OPENMP)
endif()
if(OpenMP_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_OPENMP)
  link_directories(${${OpenMP_name}_LIBRARY_DIRS})
  include_directories(${${OpenMP_name}_INCLUDE_DIRS})
endif()

find_package(X11 )
if(X11_FOUND)
  set(X11_name X11)
elseif(X11_FOUND)
  set(X11_name X11)
endif()
if(X11_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_X11)
  link_directories(${${X11_name}_LIBRARY_DIRS})
  include_directories(${${X11_name}_INCLUDE_DIRS})
endif()

find_package(gpusd 1.5.0)
if(gpusd_FOUND)
  set(gpusd_name gpusd)
elseif(GPUSD_FOUND)
  set(gpusd_name GPUSD)
endif()
if(gpusd_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_GPUSD)
  link_directories(${${gpusd_name}_LIBRARY_DIRS})
  include_directories(${${gpusd_name}_INCLUDE_DIRS})
endif()

find_package(GLStats 0.9.0)
if(GLStats_FOUND)
  set(GLStats_name GLStats)
elseif(GLSTATS_FOUND)
  set(GLStats_name GLSTATS)
endif()
if(GLStats_name)
  list(APPEND FIND_PACKAGES_FOUND EQUALIZER_USE_GLSTATS)
  link_directories(${${GLStats_name}_LIBRARY_DIRS})
  include_directories(${${GLStats_name}_INCLUDE_DIRS})
endif()


# Write defines.h and options.cmake
if(NOT FIND_PACKAGES_INCLUDE)
  set(FIND_PACKAGES_INCLUDE
    "${CMAKE_BINARY_DIR}/include/${CMAKE_PROJECT_NAME}/defines${SYSTEM}.h")
endif()
if(NOT OPTIONS_CMAKE)
  set(OPTIONS_CMAKE ${CMAKE_BINARY_DIR}/options.cmake)
endif()
set(DEFINES_FILE ${FIND_PACKAGES_INCLUDE})
set(DEFINES_FILE_IN ${DEFINES_FILE}.in)
file(WRITE ${DEFINES_FILE_IN}
  "// generated by Buildyard, do not edit.\n\n"
  "#ifndef ${CMAKE_PROJECT_NAME}_DEFINES_${SYSTEM}_H\n"
  "#define ${CMAKE_PROJECT_NAME}_DEFINES_${SYSTEM}_H\n\n")
file(WRITE ${OPTIONS_CMAKE} "# Optional modules enabled during build\n")
foreach(DEF ${FIND_PACKAGES_FOUND})
  add_definitions(-D${DEF})
  file(APPEND ${DEFINES_FILE_IN}
  "#ifndef ${DEF}\n"
  "#  define ${DEF}\n"
  "#endif\n")
if(NOT DEF STREQUAL SYSTEM)
  file(APPEND ${OPTIONS_CMAKE} "set(${DEF} ON)\n")
endif()
endforeach()
file(APPEND ${DEFINES_FILE_IN}
  "\n#endif\n")

include(UpdateFile)
update_file(${DEFINES_FILE_IN} ${DEFINES_FILE})
