import os
from io import StringIO

from conan import ConanFile
from conan.errors import ConanException
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class LibfnTestConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VirtualRunEnv"
    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["LIBFN_TEST_CXX26"] = bool(self.dependencies["libfn"].options.cxx26)
        tc.cache_variables["LIBFN_TEST_AGGREGATE"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def _check_quine(self, name):
        stdout = StringIO()
        self.run(os.path.join(self.cpp.build.bindir, name), env="conanrun", stdout=stdout)
        # main.cpp is a quine; normalize newlines on both sides so MSVC's \r\n stdout compares equal
        with open(os.path.join(self.source_folder, "src", "main.cpp")) as source:
            if stdout.getvalue().replace("\r\n", "\n") != source.read():
                raise ConanException(f"{name} output differs from src/main.cpp")

    def test(self):
        if not can_run(self):
            return
        self._check_quine("main")
        if self.dependencies["libfn"].options.cxx26:
            self._check_quine("main_cxx26")
