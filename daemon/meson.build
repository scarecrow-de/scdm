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

scdm_session_enums = gnome.mkenums('scdm-session-enum-types',
  h_template: 'scdm-session-enum-types.h.in',
  c_template: 'scdm-session-enum-types.c.in',
  sources: 'scdm-session.h',
)
scdm_session_worker_enums = gnome.mkenums('scdm-session-worker-enum-types',
  h_template: 'scdm-session-worker-enum-types.h.in',
  c_template: 'scdm-session-worker-enum-types.c.in',
  sources: 'scdm-session-worker.h',
)

# Daemons deps
scdm_daemon_deps = [
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
  scdm_daemon_deps += libwrap_dep
endif

# test-session-client
test_session_client_src = [
  'test-session-client.c',
  session_dbus_gen,
  manager_dbus_gen,
]

test_session_client = executable('test-session-client',
  test_session_client_src,
  dependencies: scdm_daemon_deps,
  include_directories: config_h_dir,
)

# Session worker
scdm_session_worker_src = [
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
  scdm_session_enums,
  scdm_session_worker_enums,
]

scdm_session_worker_deps = [
  scdm_daemon_deps,
]

scdm_session_worker_includes = [
  config_h_dir,
]

if pam_extensions_supported
  scdm_session_worker_src += '../pam-extensions/scdm-pam-extensions.h'
  scdm_session_worker_includes += pam_extensions_inc
endif

if libaudit_dep.found()
  scdm_session_worker_deps += libaudit_dep

  scdm_session_worker_src += [
    'scdm-session-linux-auditor.c',
  ]
endif

if have_adt
  scdm_session_worker_src += 'scdm-session-solaris-auditor.c'
endif

scdm_session_worker = executable('scdm-session-worker',
  scdm_session_worker_src,
  dependencies: scdm_session_worker_deps,
  include_directories: scdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# Wayland session
scdm_wayland_session_src = [
  'scdm-wayland-session.c',
  manager_dbus_gen,
]

scdm_wayland_session = executable('scdm-wayland-session',
  scdm_wayland_session_src,
  dependencies: scdm_daemon_deps,
  include_directories: scdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# X session
scdm_x_session_src = [
  'scdm-x-session.c',
  manager_dbus_gen,
]

scdm_x_session = executable('scdm-x-session',
  scdm_x_session_src,
  dependencies: scdm_daemon_deps,
  include_directories: scdm_session_worker_includes,
  install: true,
  install_dir: get_option('libexecdir'),
)

# GDM daemon
scdm_daemon_sources = files(
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

scdm_daemon_gen_sources = [
  display_dbus_gen,
  local_display_factory_dbus_gen,
  manager_dbus_gen,
  local_display_dbus_gen,
  session_dbus_gen,
  session_worker_dbus_gen,
  scdm_session_enums,
]

if xdmcp_dep.found()
  scdm_daemon_deps += xdmcp_dep

  scdm_daemon_sources = [
    scdm_daemon_sources,
    files(
      'scdm-xdmcp-display-factory.c',
      'scdm-xdmcp-display.c',
      'scdm-xdmcp-chooser-display.c',
    ),
  ]
endif

scdm_daemon = executable('scdm',
  [ scdm_daemon_sources, scdm_daemon_gen_sources ],
  dependencies: scdm_daemon_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('sbindir')
)
