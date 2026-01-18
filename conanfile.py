from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class Project(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators =  "CMakeDeps"


    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False #workaround because this leads to useless options in cmake-tools configure
        tc.generate()

    def configure(self):
        # We can control the options of our dependencies based on current options
        self.options["catch2"].with_main = True
        self.options["catch2"].with_benchmark = True

    def requirements(self):
        self.requires("boost/1.90.0", force=True)
        self.requires("openssl/3.5.1", force=True)
        self.requires("fmt/11.2.0")
        self.requires("catch2/3.8.1")