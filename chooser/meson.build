# Simple chooser
gdm_simple_chooser_src = [
 'chooser-main.c',
 'scdm-chooser-host.c',
 'scdm-chooser-session.c',
 'scdm-host-chooser-dialog.c',
 'scdm-host-chooser-widget.c',
]

gdm_simple_chooser_deps = [
  glib_dep,
  gtk_dep,
  libscdmcommon_dep,
  libgdm_dep,
  x_deps,
  xdmcp_dep,
]

gdm_simple_chooser = executable('scdm-simple-chooser',
  gdm_simple_chooser_src,
  dependencies: gdm_simple_chooser_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)

# Host chooser
gdm_host_chooser_src = [
 'scdm-host-chooser.c',
 'scdm-chooser-host.c',
 'scdm-host-chooser-dialog.c',
 'scdm-host-chooser-widget.c',
]

gdm_host_chooser_deps = [
  glib_dep,
  gtk_dep,
  libscdmcommon_dep,
  libgdm_dep,
  x_deps,
  xdmcp_dep,
]

gdm_host_chooser = executable('scdm-host-chooser',
  gdm_host_chooser_src,
  dependencies: gdm_host_chooser_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)
