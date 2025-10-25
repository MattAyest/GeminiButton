Import("env")
env.Append(CPPDEFINES=[("UNIT_TESTING", 1)]) # Example define for tests
# This ensures build flags are part of the compilation database generation
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)
env.Replace(COMPILATIONDB_USE_ABSOLUTE_PATHS=True)
