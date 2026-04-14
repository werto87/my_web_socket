from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class Project(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators =  "CMakeDeps"
    python_requires = "shared/1.0.0"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False #workaround because this leads to useless options in cmake-tools configure
        tc.generate()

    def configure(self):
        # We can control the options of our dependencies based on current options
        self.options["catch2"].with_main = True
        self.options["catch2"].with_benchmark = True

    def requirements(self):
        sharedConan = self.python_requires["shared"].module.SharedConan
        all_deps = {**sharedConan.COMMON, **sharedConan.BACKEND}
        deps_to_use = [
            "boost",
            "openssl",
            "spdlog"
        ]
        for pkg_name in deps_to_use:
            version, isModernDurak = all_deps[pkg_name]
            self.requires(
                    f"{pkg_name}/{version}{'@modern-durak' if isModernDurak else ''}",
                )


        self.requires("catch2/3.8.1")