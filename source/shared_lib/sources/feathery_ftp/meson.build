src_feathery_ftp = [
  'ftpTargetWin32.c',
  'ftpServer.c',
  'ftpTargetPosix.c',
  'ftpLib.c',
  'ftpRuntime.c',
  'ftpCmds.c',
  'ftpSession.c',
  'ftpAccount.c',
  'ftpMessages.c',
]

inc_feathery_ftp = ['../../include/feathery_ftp/']

lib_featheryftp = static_library(
  'feathery_ftp',
  src_feathery_ftp,
  include_directories: inc_feathery_ftp,
)

feathery_ftp_dep = declare_dependency(
  link_with: lib_featheryftp,
  include_directories: inc_feathery_ftp,
)
