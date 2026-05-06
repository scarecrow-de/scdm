libscdm_manager_dbus_gen = gnome.gdbus_codegen('scdm-manager-glue',
    sources: '..' / 'daemon' / 'scdm-manager.xml',
    namespace: 'Scdm',
    interface_prefix: 'io.github.scarecrow_de.DisplayManager',
    autocleanup: 'all',
    annotations: [
      [ 'io.github.scarecrow_de.DisplayManager.Manager', 'org.gtk.GDBus.C.Name', 'Manager' ],
    ],
    build_by_default: true,
)
libscdm_client_dbus_gen = gnome.gdbus_codegen('scdm-client-glue',
    sources: '..' / 'daemon' / 'scdm-session.xml',
    namespace: 'Scdm',
    interface_prefix: 'io.github.scarecrow_de.DisplayManager',
    autocleanup: 'all',
    annotations: [
      [ 'io.github.scarecrow_de.DisplayManager.UserVerifier', 'org.gtk.GDBus.C.Name', 'UserVerifier' ],
      [ 'io.github.scarecrow_de.DisplayManager.Greeter', 'org.gtk.GDBus.C.Name', 'Greeter' ],
      [ 'io.github.scarecrow_de.DisplayManager.RemoteGreeter', 'org.gtk.GDBus.C.Name', 'RemoteGreeter' ],
      [ 'io.github.scarecrow_de.DisplayManager.Chooser', 'org.gtk.GDBus.C.Name', 'Chooser' ],
    ],
    build_by_default: true,
    install_header: true,
    install_dir: get_option('includedir') / meson.project_name()
)

libscdm_built_sources = [
  libscdm_manager_dbus_gen,
  libscdm_client_dbus_gen,
]

libscdm_sources = files(
  'scdm-client.c',
  'scdm-sessions.c',
  'scdm-user-switching.c',
)

libscdm_public_headers = [
  'scdm-client.h',
  'scdm-sessions.h',
  'scdm-user-switching.h',
  # scdm-client-glue.h is automatically installed
]

install_headers(libscdm_public_headers,
  subdir: meson.project_name(),
)

libscdm_headers = [
  libscdm_public_headers,
  libscdm_client_dbus_gen[1],
  libscdm_manager_dbus_gen[1],
]

libscdm_deps = [
  glib_dep,
  gio_dep,
  gio_unix_dep,
  libsystemd_dep,
  libscdmcommon_dep,
]

libscdm_link_flags = [
  '-Wl,--version-script,@0@/libscdm.map'.format(meson.current_source_dir()),
]

libscdm = library('scdm',
  libscdm_sources + libscdm_built_sources,
  version: '1.0.0',
  dependencies: libscdm_deps,
  include_directories: config_h_dir,
  link_args: libscdm_link_flags,
  install: true,
)

libscdm_gir_includes = [
  'GLib-2.0',
  'GObject-2.0',
  'Gio-2.0',
]

libscdm_dep = declare_dependency(
  link_with: libscdm,
  dependencies: libscdm_deps,
  include_directories: include_directories('.'),
  sources: libscdm_built_sources,
)

libscdm_gir = gnome.generate_gir(libscdm,
  sources: libscdm_headers + libscdm_sources,
  namespace: 'Scdm',
  nsversion: '1.0',
  identifier_prefix: 'Scdm',
  includes: libscdm_gir_includes,
  install: true,
)

# Pkg-config file
pkgconfig.generate(libscdm,
  name: 'GDM Client Library',
  description: 'Client Library for communicating with GDM daemon',
  filebase: meson.project_name(),
  requires: gobject_dep,
)
