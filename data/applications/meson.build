desktop_conf = {
  'LIBDIR': scdm_prefix / get_option('libdir'),
  'LIBEXECDIR': scdm_prefix / get_option('libexecdir'),
  'LOCALSTATEDIR': scdm_prefix / get_option('localstatedir'),
  'SBINDIR': scdm_prefix / get_option('sbindir'),
  'SYSCONFDIR': scdm_prefix / get_option('sysconfdir'),
}

foreach desktop_file : [ 'mime-dummy-handler.desktop', 'mimeapps.list' ]
  configure_file(
    input: desktop_file,
    output: desktop_file,
    configuration: desktop_conf,
    install_dir: get_option('datadir') / 'scdm' / 'greeter' / 'applications',
  )
endforeach
