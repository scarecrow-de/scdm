# scdm-flexiserver
scdm_flexiserver_deps = [
  glib_dep,
  libscdmcommon_dep,
]

scdm_flexiserver = executable('scdmflexiserver',
  'scdmflexiserver.c',
  dependencies: scdm_flexiserver_deps,
  include_directories: config_h_dir,
  install: true,
)

# scdm-screenshot
scdm_screenshot_deps = [
  glib_dep,
  gtk_dep,
  x_deps,
  libcanberra_gtk_dep,
]

scdm_screenshot = executable('scdm-screenshot',
  'scdm-screenshot.c',
  dependencies: scdm_screenshot_deps,
  include_directories: config_h_dir,
  install: true,
)

# scdm-disable-wayland
scdm_disable_wayland_deps = [
  glib_dep,
]

scdm_disable_wayland = executable('scdm-disable-wayland',
  'scdm-disable-wayland.c',
  dependencies: scdm_disable_wayland_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)

