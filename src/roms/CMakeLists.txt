fips_begin_lib(roms)
  fips_generate(FROM rygar-roms.yml TYPE dump HEADER rygar-roms.h)
  fips_files(dummy.c)
fips_end_lib()
