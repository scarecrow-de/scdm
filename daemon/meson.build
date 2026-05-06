# D-Bus interfaces
display_dbus_gen = gnome.gdbus_codegen('scdm-display-glue',
  'scdm-display.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)
local_display_dbus_gen = gnome.gdbus_codegen('scdm-local-display-glue',
  'scdm-local-display.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)
local_display_factory_dbus_gen = gnome.gdbus_codegen('scdm-local-display-factory-glue',
  'scdm-local-display-factory.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)
manager_dbus_gen = gnome.gdbus_codegen('scdm-manager-glue',
  'scdm-manager.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)
session_dbus_gen = gnome.gdbus_codegen('scdm-session-glue',
  'scdm-session.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)
session_worker_dbus_gen = gnome.gdbus_codegen('scdm-session-worker-glue',
  'scdm-session-worker.xml',
  namespace: 'ScdmDBus',
  interface_prefix: 'io.github.scarecrow_de.DisplayManager',
  autocleanup: 'all',
)

gdm_session_enums = gnome.mkenums('scdm-session-enum-types',
  h_template: 'scdm-session-enum-types.h.in',
  c_template: 'scdm-session-enum-types.c.in',
  sources: 'scdm-session.h',
)
gdm_session_worker_enums = gnome.mkenums('scdm-session-worker-enum-types',
  h_template: 'scdm-session-worker-enum-types.h.in',
  c_template: 'scdm-session-worker-enum-types.c.in',
  sources: 'scdm-session-worker.h',
)

# Daemons deps
gdm_daemon_deps = [
  libscdmcommon_dep,
  accountsservice_dep,
  gobject_dep,
  gio_dep,
  gio_unix_dep,
  libpam_dep,
  x_deps,
  xcb_dep,
]

if xdmcp_dep.found() and get_option('tcp-wrappers')
  gdm_daemon_deps += libwrap_dep
endif

# test-session-client
test_session_client_src = [
  'test-session-client.c',
  session_dbus_gen,
  manager_dbus_gen,
]

test_session_client = executable('test-session-client',
  test_session_client_src,
  dependencies: gdm_daemon_deps,
  include_directories: config_h_dir,
)

# Session worker
gdm_session_worker_src = [
  'session-worker-main.c',
  'scdm-session.c',
  'scdm-session-settings.c',
  'scdm-session-auditor.c',
  'scdm-session-record.c',
  'scdm-session-worker.c',
  'scdm-session-worker-job.c',
  'scdm-session-worker-common.c',
  'scdm-dbus-util.c',
  session_dbus_gen,
  session_worker_dbus_gen,
  gdm_session_enums,
  gdm_session_worker_enums,
]

gdm_session_worker_deps = [
  gdm_daemon_deps,
]

gdm_session_worker_includes = [
  config_h_dir,
]

if pam_extensions_supported
  gdm_session_worker_src += '../pam-extensions/scdm-pam-extensions.h'
  gdm_session_worker_includes += pam_extensions_inc
endif

if libaudit_dep.found()
  gdm_session_worker_deps += libaudit_dep

  gdm_session_worker_src += [
    'scdm-session-linux-auditor.c',
  ]
endif

if have_adt
  gdm_session_worker_src += 'scdm-session-solaris-auditor.c'
endif

gdm_session_worker = executable('scdm-session-worker',
  gdm_session_worker_src,
  dependencies: gdm_session_worker_deps,
  include_directories: gdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# Wayland session
gdm_wayland_session_src = [
  'scdm-wayland-session.c',
  manager_dbus_gen,
]

gdm_wayland_session = executable('scdm-wayland-session',
  gdm_wayland_session_src,
  dependencies: gdm_daemon_deps,
  include_directories: gdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# X session
gdm_x_session_src = [
  'scdm-x-session.c',
  manager_dbus_gen,
]

gdm_x_session = executable('scdm-x-session',
  gdm_x_session_src,
  dependencies: gdm_daemon_deps,
  include_directories: gdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# GDM daemon
gdm_daemon_sources = files(
  'scdm-dbus-util.c',
  'scdm-display-access-file.c',
  'scdm-display-factory.c',
  'scdm-display-store.c',
  'scdm-display.c',
  'scdm-launch-environment.c',
  'scdm-legacy-display.c',
  'scdm-local-display-factory.c',
  'scdm-local-display.c',
  'scdm-manager.c',
  'scdm-server.c',
  'scdm-session-record.c',
  'scdm-session-worker-common.c',
  'scdm-session-worker-job.c',
  'scdm-session.c',
  'main.c',
)

gdm_daemon_gen_sources = [
  display_dbus_gen,
  local_display_factory_dbus_gen,
  manager_dbus_gen,
  local_display_dbus_gen,
  session_dbus_gen,
  session_worker_dbus_gen,
  gdm_session_enums,
]

if xdmcp_dep.found()
  gdm_daemon_deps += xdmcp_dep

  gdm_daemon_sources = [
    gdm_daemon_sources,
    files(
      'scdm-xdmcp-display-factory.c',
      'scdm-xdmcp-display.c',
      'scdm-xdmcp-chooser-display.c',
    ),
  ]
endif

gdm_daemon = executable('scdm',
  [ gdm_daemon_sources, gdm_daemon_gen_sources ],
  dependencies: gdm_daemon_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('sbindir')
)
