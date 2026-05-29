import os

from conan import ConanFile
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
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not can_run(self):
            return
        # pfn is always available.
        self.run(os.path.join(self.cpp.build.bindir, "pfn_quine"), env="conanrun")
        # fn is only available when disable_cxx23 is False.
        if not self.dependencies["libfn"].options.disable_cxx23:
            self.run(os.path.join(self.cpp.build.bindir, "fn_quine"), env="conanrun")
