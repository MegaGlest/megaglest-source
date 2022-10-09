# special macros SpecialMacros.cmake

include(CheckFunctionExists)
include(AddFileDependencies)

#macro(special_add_library lib)
#  if (WIN32)
#    add_library(${lib} STATIC ${ARGN})
#  else (WIN32)
#    add_library(${lib} SHARED ${ARGN})
#  endif (WIN32)
#endmacro(special_add_library)

macro(special_check_for_sse _max_sse_level_desired) 
  set(${_max_sse_level_desired})
  
  message(STATUS "Max SSE desired: [${_max_sse_level_desired}]")

  # check for SSE extensions
  include(CheckCXXSourceRuns)
  include(CheckCSourceRuns)

  # Add vs_build to stop flags added to msbuild.
  IF(NOT MINGW AND NOT VS_BUILD)
          if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
           set(SSE_FLAGS)
          
           set(CMAKE_REQUIRED_FLAGS "-msse3")
           check_cxx_source_runs("
            #include <pmmintrin.h>
          
            int main()
            {
               __m128d a, b;
               double vals[2] = {0};
               a = _mm_loadu_pd(vals);
               b = _mm_hadd_pd(a,a);
               _mm_storeu_pd(vals, b);
               return 0;
            }"
            HAS_SSE3_EXTENSIONS)
          
           set(CMAKE_REQUIRED_FLAGS "-msse2")
           check_cxx_source_runs("
            #include <emmintrin.h>
          
            int main()
            {
                __m128d a, b;
                double vals[2] = {0};
                a = _mm_loadu_pd(vals);
                b = _mm_add_pd(a,a);
                _mm_storeu_pd(vals,b);
                return 0;
             }"
             HAS_SSE2_EXTENSIONS)
          
           set(CMAKE_REQUIRED_FLAGS "-msse")
           check_cxx_source_runs("
            #include <xmmintrin.h>
            int main()
            {
                __m128 a, b;
                float vals[4] = {0};
                a = _mm_loadu_ps(vals);
                b = a;
                b = _mm_add_ps(a,b);
                _mm_storeu_ps(vals,b);
                return 0;
            }"
            HAS_SSE_EXTENSIONS)
   ELSE()
        set(HAS_SSE_EXTENSIONS ON)
   ENDIF()
  
   set(CMAKE_REQUIRED_FLAGS)
  
   if(HAS_SSE3_EXTENSIONS AND (${_max_sse_level_desired} MATCHES "3"))
        set(SSE_FLAGS "-msse3")
        IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        ELSE()
                set(SSE_FLAGS "${SSE_FLAGS} -mfpmath=sse")
        ENDIF()
        message(STATUS "Found SSE3 extensions, using flags: ${SSE_FLAGS}")
   elseif(HAS_SSE2_EXTENSIONS AND (${_max_sse_level_desired} MATCHES "2" OR ${_max_sse_level_desired} MATCHES "3"))
        set(SSE_FLAGS "-msse2")
        IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        ELSE()
                set(SSE_FLAGS "${SSE_FLAGS} -mfpmath=sse")
        ENDIF()

        message(STATUS "Found SSE2 extensions, using flags: ${SSE_FLAGS}")
   elseif(HAS_SSE_EXTENSIONS AND (${_max_sse_level_desired} MATCHES "1" OR ${_max_sse_level_desired} MATCHES "2" OR ${_max_sse_level_desired} MATCHES "3"))
        set(SSE_FLAGS "-msse")
        IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        ELSE()
                set(SSE_FLAGS "${SSE_FLAGS} -mfpmath=sse")
        ENDIF()

        message(STATUS "Found SSE extensions, using flags: ${SSE_FLAGS}")
   endif()
   
   add_definitions(${SSE_FLAGS})

  elseif(MSVC)
   check_cxx_source_runs("
    #include <emmintrin.h>
  
    int main()
    {
        __m128d a, b;
        double vals[2] = {0};
        a = _mm_loadu_pd(vals);
        b = _mm_add_pd(a,a);
        _mm_storeu_pd(vals,b);
        return 0;
     }"
     HAS_SSE2_EXTENSIONS)
   if( HAS_SSE2_EXTENSIONS AND (NOT ${_max_sse_level_desired} OR ${_max_sse_level_desired} MATCHES "2"))
    message(STATUS "Found SSE2 extensions")
    set(SSE_FLAGS "/arch:SSE2 /fp:fast -D__SSE__ -D__SSE2__" )
   endif()
   
   add_definitions(${SSE_FLAGS})
  endif()
endmacro(special_check_for_sse)

macro(special_check_for_x87) 
  # check for X87 support
  include(CheckCXXSourceRuns)
  include(CheckCSourceRuns)

  if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
  
   IF(NOT MINGW)
           check_cxx_source_runs("
            int main()
            {
               unsigned short fpu_mode;
               do { asm volatile (\"fstcw %0\" : \"=m\" (fpu_mode) : ); } while (0);
               fpu_mode &= 0xFCFF; 
               do { asm volatile (\"fclex \\\\n fldcw %0\" : : \"m\" (fpu_mode)); } while (0);

               return 0;
            }"
            HAS_X87_SUPPORT)
   ELSE()
        set(HAS_X87_SUPPORT On)
   ENDIF()
  
   if(HAS_X87_SUPPORT)
    message(STATUS "Found X87 support.")
   endif()

  elseif(MSVC)

    set(HAS_X87_SUPPORT On)

  endif()
endmacro(special_check_for_x87)

macro(special_add_compile_flags target)
  set(args ${ARGN})
  separate_arguments(args)
  get_target_property(_flags ${target} COMPILE_FLAGS)
  if(NOT _flags)
    set(_flags ${ARGN})
  else(NOT _flags)
    separate_arguments(_flags)
    list(APPEND _flags "${args}")
  endif(NOT _flags)

  _special_list_to_string(_flags_str "${_flags}")
  set_target_properties(${target} PROPERTIES
                        COMPILE_FLAGS "${_flags_str}")
endmacro(special_add_compile_flags)

macro(special_add_link_flags target)
  set(args ${ARGN})
  separate_arguments(args)
  get_target_property(_flags ${target} LINK_FLAGS)
  if(NOT _flags)
    set(_flags ${ARGN})
  else(NOT _flags)
    separate_arguments(_flags)
    list(APPEND _flags "${args}")
  endif(NOT _flags)

  _special_list_to_string(_flags_str "${_flags}")
  set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "${_flags_str}")
endmacro(special_add_link_flags)

macro(_special_list_to_string _string _list)
    set(${_string})
    foreach(_item ${_list})
        string(LENGTH "${${_string}}" _len)
        if(${_len} GREATER 0)
          set(${_string} "${${_string}} ${_item}")
        else(${_len} GREATER 0)
          set(${_string} "${_item}")
        endif(${_len} GREATER 0)
    endforeach(_item)
endmacro(_special_list_to_string)
