# Simple chooser
scdm_simple_chooser_src = [
 'chooser-main.c',
 'scdm-chooser-host.c',
 'scdm-chooser-session.c',
 'scdm-host-chooser-dialog.c',
 'scdm-host-chooser-widget.c',
]

scdm_simple_chooser_deps = [
  glib_dep,
  gtk_dep,
  libscdmcommon_dep,
  libscdm_dep,
  x_deps,
  xdmcp_dep,
]

scdm_simple_chooser = executable('scdm-simple-chooser',
  scdm_simple_chooser_src,
  dependencies: scdm_simple_chooser_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)

# Host chooser
scdm_host_chooser_src = [
 'scdm-host-chooser.c',
 'scdm-chooser-host.c',
 'scdm-host-chooser-dialog.c',
 'scdm-host-chooser-widget.c',
]

scdm_host_chooser_deps = [
  glib_dep,
  gtk_dep,
  libscdmcommon_dep,
  libscdm_dep,
  x_deps,
  xdmcp_dep,
]

scdm_host_chooser = executable('scdm-host-chooser',
  scdm_host_chooser_src,
  dependencies: scdm_host_chooser_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)
