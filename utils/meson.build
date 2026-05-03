# scdm-flexiserver
gdm_flexiserver_deps = [
  glib_dep,
  libscdmcommon_dep,
]

gdm_flexiserver = executable('scdmflexiserver',
  'scdmflexiserver.c',
  dependencies: gdm_flexiserver_deps,
  include_directories: config_h_dir,
  install: true,
)

# scdm-screenshot
gdm_screenshot_deps = [
  glib_dep,
  gtk_dep,
  x_deps,
  libcanberra_gtk_dep,
]

gdm_screenshot = executable('scdm-screenshot',
  'scdm-screenshot.c',
  dependencies: gdm_screenshot_deps,
  include_directories: config_h_dir,
  install: true,
)

# scdm-disable-wayland
gdm_disable_wayland_deps = [
  glib_dep,
]

gdm_disable_wayland = executable('scdm-disable-wayland',
  'scdm-disable-wayland.c',
  dependencies: gdm_disable_wayland_deps,
  include_directories: config_h_dir,
  install: true,
  install_dir: get_option('libexecdir'),
)

