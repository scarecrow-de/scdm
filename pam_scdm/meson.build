pam_gdm_deps = [
  libpam_dep,
]

if keyutils_dep.found()
  pam_gdm_deps += keyutils_dep
endif

pam_scdm = library('pam_scdm',
  'pam_scdm.c',
  dependencies: pam_gdm_deps,
  include_directories: config_h_dir,
  name_prefix: '',
  install: true,
  install_dir: pam_mod_dir,
)
