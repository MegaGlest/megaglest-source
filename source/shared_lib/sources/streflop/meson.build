includes = [
    '../../include/streflop',
    '../../include/streflop/libm/flt-32',
    '../../include/streflop/libm/headers',
]

add_cpp_args = [
    '-DSTREFLOP_SSE',
    '-fno-strict-aliasing',
    '-mieee-fp',
    '-mfpmath=sse',
    '-msse',
    '-msse2',
    '-fPIC',
]

#Random_lib = static_library('Random', 'Random.cpp', cpp_args: add_cpp_args, include_directories : includes)

#src = [
#  'SMath.cpp',
#  'streflopC.cpp',
#]

# src_basename = [
#   'e_expf.c',
#   'w_expf.c',
# ]

# foreach file : src_basename
#     src += [join_paths('libm/', file)]
# endforeach

src_basename = [
    'e_acosf.cpp',
    'e_acoshf.cpp',
    'e_asinf.cpp',
    'e_atan2f.cpp',
    'e_atanhf.cpp',
    'e_coshf.cpp',
    'e_exp2f.cpp',
    'e_expf.cpp',
    'e_fmodf.cpp',
    'e_gammaf_r.cpp',
    'e_hypotf.cpp',
    'e_j0f.cpp',
    'e_j1f.cpp',
    'e_jnf.cpp',
    'e_lgammaf_r.cpp',
    'e_log10f.cpp',
    'e_log2f.cpp',
    'e_logf.cpp',
    'e_powf.cpp',
    'e_remainderf.cpp',
    'e_rem_pio2f.cpp',
    'e_sinhf.cpp',
    'e_sqrtf.cpp',
    'k_cosf.cpp',
    'k_rem_pio2f.cpp',
    'k_sinf.cpp',
    'k_tanf.cpp',
    's_asinhf.cpp',
    's_atanf.cpp',
    's_cbrtf.cpp',
    's_ceilf.cpp',
    's_copysignf.cpp',
    's_cosf.cpp',
    's_erff.cpp',
    's_expm1f.cpp',
    's_fabsf.cpp',
    's_finitef.cpp',
    's_floorf.cpp',
    's_fpclassifyf.cpp',
    's_frexpf.cpp',
    's_ilogbf.cpp',
    's_isinff.cpp',
    's_isnanf.cpp',
    's_ldexpf.cpp',
    's_llrintf.cpp',
    's_llroundf.cpp',
    's_log1pf.cpp',
    's_logbf.cpp',
    's_lrintf.cpp',
    's_lroundf.cpp',
    's_modff.cpp',
    's_nearbyintf.cpp',
    's_nextafterf.cpp',
    's_remquof.cpp',
    's_rintf.cpp',
    's_roundf.cpp',
    's_scalblnf.cpp',
    's_scalbnf.cpp',
    's_signbitf.cpp',
    's_sincosf.cpp',
    's_sinf.cpp',
    's_tanf.cpp',
    's_tanhf.cpp',
    's_truncf.cpp',
    'w_expf.cpp',
]

libm32_lib_src = []
foreach file : src_basename
    libm32_lib_src += [join_paths('libm/flt-32', file)]
endforeach

libm32_lib = static_library(
    'libm32',
    libm32_lib_src,
    include_directories: includes,
)

lib_streflop = static_library(
    'streflop',
    cpp_args: add_cpp_args,
    include_directories: includes,
    link_with: [libm32_lib],
    #   dependencies: [dep_curl, dep_sdl]
)

dep_streflop = declare_dependency(
    link_with: lib_streflop,
    include_directories: includes,
)
