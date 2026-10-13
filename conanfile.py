import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load, rmdir

required_conan_version = ">=2.0.9"


class LibfnConan(ConanFile):
    name = "libfn"
    license = "ISC"
    author = "Bronek Kozicki, Alex Kremer, Gašper Ažman"
    url = "https://github.com/libfn/functional"
    homepage = "https://libfn.org"
    description = (
        "Functional programming in C++"
    )
    topics = ("functional", "header-only", "monadic", "expected", "optional", "cpp23")

    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "cxx26": [True, False],
    }
    default_options = {
        "cxx26": False,
    }
    no_copy_source = True

    exports = "VERSION"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "LICENSE.md", "README.md", "VERSION"

    def set_version(self):
        self.version = load(self, os.path.join(self.recipe_folder, "VERSION")).strip()

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25]")

    def package_id(self):
        self.info.clear()

    def validate(self):
        check_min_cppstd(self, 26 if self.options.cxx26 else 20)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["LIBFN_TESTS"] = False
        # Avoid creating a compile_commands.json symlink in the shared source folder.
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()

    def package(self):
        copy(
            self,
            pattern="LICENSE.md",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "libfn")
        self.cpp_info.set_property("cmake_target_name", "libfn::libfn")

        version = self.cpp_info.components["version"]
        version.set_property("cmake_target_name", "libfn::version")
        version.bindirs = []
        version.libdirs = []
        version.includedirs = ["include"]

        pfn = self.cpp_info.components["pfn"]
        pfn.set_property("cmake_target_name", "libfn::pfn")
        pfn.bindirs = []
        pfn.libdirs = []
        pfn.includedirs = ["include"]
        pfn.requires = ["version"]

        fn = self.cpp_info.components["fn"]
        fn.set_property("cmake_target_name", "libfn::fn")
        fn.bindirs = []
        fn.libdirs = []
        fn.includedirs = ["include"]
        fn.requires = ["pfn", "version"]
        # Require explicit mode selection; a separate component would also enable
        # C++26 through the libfn::libfn aggregate.
        if self.options.cxx26:
            fn.defines.append("LIBFN_CXX26")
            fn.set_property("cmake_target_aliases", ["libfn::fn_cxx26"])

        # CMakeDeps generates targets without the installed CMake usage requirements.
        # Copy the compiler options from cmake/CompilationOptions.cmake.
        compiler = self.settings.get_safe("compiler")
        for component in (version, pfn, fn):
            if compiler == "msvc":
                component.cxxflags.append("/permissive-")
                component.defines.append("_HAS_CXX23")
            elif compiler in ("clang", "apple-clang"):
                component.cxxflags.append("-Wno-missing-braces")
