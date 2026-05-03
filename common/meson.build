libscdmcommon_src = files(
  'scdm-address.c',
  'scdm-common.c',
  'scdm-log.c',
  'scdm-profile.c',
  'scdm-settings-backend.c',
  'scdm-settings-desktop-backend.c',
  'scdm-settings-direct.c',
  'scdm-settings-utils.c',
  'scdm-settings.c',
)

libscdmcommon_deps = [
  libsystemd_dep,
  gobject_dep,
  gio_dep,
  gio_unix_dep,
]

if libselinux_dep.found()
  libscdmcommon_deps += libselinux_dep
endif

libscdmcommon_lib = static_library('scdmcommon',
  libscdmcommon_src,
  dependencies: libscdmcommon_deps,
  include_directories: config_h_dir,
)

libscdmcommon_dep = declare_dependency(
  link_with: libscdmcommon_lib,
  dependencies: libscdmcommon_deps,
  include_directories: include_directories('.'),
)

install_data('gdb-cmd')

# test-log exectuable
test_log = executable('test-log',
  'test-log.c',
  dependencies: libscdmcommon_dep,
  include_directories: config_h_dir,
)
