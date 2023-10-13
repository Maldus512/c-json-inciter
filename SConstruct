import multiprocessing

TEST_SUITE = "test_suite"

CFLAGS = [
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-g",
    "-O0",
]


def PhonyTargets(
    target,
    action,
    depends,
    env=None,
):
    # Creates a Phony target
    if not env:
        env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption("num_jobs", num_cpu)
    print("Running with -j {}".format(GetOption("num_jobs")))

    env_options = {
        "CPPPATH": ["."],
        "CPPDEFINES": [],
        "CCFLAGS": CFLAGS,
        "LIBS": ["-lcmocka"],
    }

    env = Environment(**env_options)
    env.Tool("compilation_db")

    sources = Glob(f"test/*.c")

    tests = env.Program(f"test/{TEST_SUITE}", sources)
    compileDB = env.CompilationDatabase('compile_commands.json')
    env.Depends(tests, compileDB)
    PhonyTargets("test", f"cd test && ./{TEST_SUITE}", tests, env)


main()
