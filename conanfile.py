import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy, load
from conan.tools.layout import basic_layout


class FunctionalConan(ConanFile):
    name = "functional"
    license = "ISC"
    author = "Bronek Kozicki, Alex Kremer, Gašper Ažman"
    url = "https://github.com/libfn/functional"
    homepage = "https://github.com/libfn/functional"
    description = (
        "Functional programming in C++"
    )
    topics = ("functional", "header-only", "monadic", "expected", "optional", "cpp23")

    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    options = {
        # Mirror of the CMake DISABLE_CXX23 option. When True, only the pfn
        # component (C++20-compatible) is usable; the fn component requires C++23.
        "disable_cxx23": [True, False],
    }
    default_options = {
        "disable_cxx23": False,
    }

    exports = "VERSION"
    exports_sources = "include/*", "LICENSE.md", "README.md"

    def set_version(self):
        self.version = load(self, os.path.join(self.recipe_folder, "VERSION")).strip()

    def layout(self):
        basic_layout(self)

    def validate(self):
        if self.options.disable_cxx23:
            # Only pfn (C++20-compatible polyfills) is available in this mode.
            check_min_cppstd(self, 20)
        else:
            # fn (the main component) requires C++23.
            check_min_cppstd(self, 23)

    def package_id(self):
        # Intentional: header-only library, so all files are identical regardless of options. package_info() runs
        # at consume time with the consumer's options, so the conditional fn component exposure works correctly.
        self.info.clear()

    def package(self):
        copy(
            self,
            pattern="*.hpp",
            src=os.path.join(self.source_folder, "include"),
            dst=os.path.join(self.package_folder, "include"),
        )
        copy(
            self,
            pattern="LICENSE.md",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )

    def package_info(self):
        # find_package(functional) -> functional::functional aggregate target,
        # plus functional::fn and functional::pfn component targets.
        self.cpp_info.set_property("cmake_file_name", "functional")
        self.cpp_info.set_property("cmake_target_name", "functional::functional")

        # fn: the main C++23 component (headers under include/fn/).
        if not self.options.disable_cxx23:
            fn = self.cpp_info.components["fn"]
            fn.set_property("cmake_target_name", "functional::fn")
            fn.bindirs = []
            fn.libdirs = []
            fn.includedirs = ["include"]

        # pfn: the C++20 component (headers under include/pfn/).
        pfn = self.cpp_info.components["pfn"]
        pfn.set_property("cmake_target_name", "functional::pfn")
        pfn.bindirs = []
        pfn.libdirs = []
        pfn.includedirs = ["include"]
