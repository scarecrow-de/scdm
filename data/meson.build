subdir('applications')
subdir('autostart')
subdir('dconf')

# XPath configs
foreach file : [ 'Init', 'PreSession', 'PostSession' ]
  generated_file = configure_file(
    input: '@0@.in'.format(file),
    output: file,
    configuration: { 'X_PATH':  x_path },
  )

  install_data(generated_file,
    install_dir: scdmconfdir / file,
    install_mode: 'rwxr-xr-x',
    rename: 'Default'
  )
endforeach

install_data('PostLogin',
  rename: 'Default.sample',
  install_mode: 'rwxr-xr-x',
  install_dir: scdmconfdir / 'PostLogin',
)

# scdm.conf
configure_file(
  input: 'scdm.conf.in',
  output: '@BASENAME@',
  configuration: {
    'GDM_USERNAME': get_option('user')
  },
  install_dir: dbus_sys_dir,
)
configure_file(
  input: 'scdm.conf-custom.in',
  output: scdm_custom_conf.split('/')[-1],
  copy: true,
  install_mode: 'rw-r--r--',
  install_dir: run_command(find_program('dirname'), scdm_custom_conf).stdout().strip(),
)

# GSettings schema
install_data('io.github.scarecrow_de.login-screen.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)
compiled_gschema = gnome.compile_schemas()

# scdm.schema
scdm_schema = configure_file(
  input: 'scdm.schemas.in',
  output: '@BASENAME@',
  configuration: {
    'GDM_USERNAME': get_option('user'),
    'GDM_GROUPNAME': get_option('group'),
    'scdmconfdir': scdmconfdir,
  },
  install_dir: get_option('datadir') / 'scdm',
)

install_data('locale.alias',
  install_dir: get_option('datadir') / 'scdm',
)

scdm_gnome_shell_component = 'io.github.scarecrow_de.Shell'
scdm_gnome_session_required_components = [
  'io.github.scarecrow_de.SettingsDaemon.A11ySettings',
  'io.github.scarecrow_de.SettingsDaemon.Color',
  'io.github.scarecrow_de.SettingsDaemon.Datetime',
  'io.github.scarecrow_de.SettingsDaemon.Housekeeping',
  'io.github.scarecrow_de.SettingsDaemon.Keyboard',
  'io.github.scarecrow_de.SettingsDaemon.MediaKeys',
  'io.github.scarecrow_de.SettingsDaemon.Power',
  'io.github.scarecrow_de.SettingsDaemon.PrintNotifications',
  'io.github.scarecrow_de.SettingsDaemon.Rfkill',
  'io.github.scarecrow_de.SettingsDaemon.ScreensaverProxy',
  'io.github.scarecrow_de.SettingsDaemon.Sharing',
  'io.github.scarecrow_de.SettingsDaemon.Smartcard',
  'io.github.scarecrow_de.SettingsDaemon.Sound',
  'io.github.scarecrow_de.SettingsDaemon.Wacom',
]

scdm_gnome_user_session_wanted_components = scdm_gnome_session_required_components
scdm_gnome_user_session_wanted_components += [
  'io.github.scarecrow_de.SettingsDaemon.XSettings',
]

configure_file(
  input: 'gnome-login.session.in',
  output: '@BASENAME@',
  configuration: {
    'libexecdir': scdm_prefix / get_option('libexecdir'),
    'check_accelerated_dir': check_accelerated_dir,
    'gnome_required_components': ';'.join(
      [ scdm_gnome_shell_component ] + scdm_gnome_session_required_components),
  },
  install_dir: get_option('datadir') / 'gnome-session' / 'sessions',
)

# PAM
pam_data_files_map = {
  'redhat': [
    'scdm-autologin',
    'scdm-launch-environment',
    'scdm-fingerprint',
    'scdm-smartcard',
    'scdm-password',
    'scdm-pin',
  ],
  'openembedded': [
    'scdm-autologin',
    'scdm-launch-environment',
    'scdm-password',
    'scdm-pin',
  ],
  'exherbo': [
    'scdm-autologin',
    'scdm-launch-environment',
    'scdm-fingerprint',
    'scdm-smartcard',
    'scdm-password',
    'scdm-pin',
  ],
  'lfs': [
    'scdm-autologin',
    'scdm-launch-environment',
    'scdm-fingerprint',
    'scdm-smartcard',
    'scdm-password',
    'scdm-pin',
  ],
  'arch': [
    'scdm-autologin',
    'scdm-launch-environment',
    'scdm-fingerprint',
    'scdm-smartcard',
    'scdm-password',
    'scdm-pin',
  ],
  'none': [],
  # We should no longer have 'autodetect' at this point
}

pam_data_files = pam_data_files_map[default_pam_config]
pam_prefix = (get_option('pam-prefix') != '')? get_option('pam-prefix') : get_option('sysconfdir')
foreach _pam_filename : pam_data_files
  install_data('pam-@0@/@1@.pam'.format(default_pam_config, _pam_filename),
    rename: _pam_filename,
    install_dir: pam_prefix / 'pam.d',
  )
endforeach

scdm_rules = configure_file(
  input: '61-scdm.rules.in',
  output: '@BASENAME@',
  configuration: {
    'libexecdir': scdm_prefix / get_option('libexecdir'),
  },
  install_dir: udev_dir,
)

# DBus service files
service_config = configuration_data()
service_config.set('sbindir', scdm_prefix / get_option('sbindir'))
service_config.set('GDM_INITIAL_VT', get_option('initial-vt'))
service_config.set('LANG_CONFIG_FILE', lang_config_file)
if plymouth_dep.found()
  service_config.set('PLYMOUTH_QUIT_SERVICE', 'plymouth-quit.service')
else
  service_config.set('PLYMOUTH_QUIT_SERVICE', '')
endif

if get_option('systemdsystemunitdir') != ''
  systemd_systemunitdir = get_option('systemdsystemunitdir')
else
  systemd_systemunitdir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')
endif

if get_option('systemduserunitdir') != ''
  systemd_userunitdir = get_option('systemduserunitdir')
else
  systemd_userunitdir = systemd_dep.get_pkgconfig_variable('systemduserunitdir',
                                                           define_variable: ['prefix', get_option('prefix')])
endif

configure_file(
  input: 'scdm.service.in',
  output: '@BASENAME@',
  configuration: service_config,
  install_dir: systemd_systemunitdir,
  format: 'cmake'
)

scdm_gnome_session_wanted_targets = []
foreach component: scdm_gnome_user_session_wanted_components
  scdm_gnome_session_wanted_targets += 'Wants=@0@.target'.format(component)
endforeach

configure_file(
  input: 'session.conf.in',
  output: 'session.conf',
  configuration: {
    'requires_component': scdm_gnome_shell_component,
    'wants_required_components': '\n'.join(scdm_gnome_session_wanted_targets),
  },
  install_dir: systemd_userunitdir / 'gnome-session@gnome-login.target.d',
)

# XSession
if get_option('scdm-xsession')
  configure_file(
    input: 'Xsession.in',
    output: '@BASENAME@',
    configuration: {
      'libexecdir': scdm_prefix / get_option('libexecdir'),
      'XSESSION_SHELL': get_option('solaris')? '/bin/ksh' : '/bin/sh',
    },
    install_dir: scdmconfdir,
  )
endif
