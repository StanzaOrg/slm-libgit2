
from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class Conantest(ConanFile):
  package_type = "application"

  # Binary configuration
  settings = "os", "arch", "compiler", "build_type"
  generators = "CMakeDeps", "CMakeToolchain"

  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}
  implements = ["auto_shared_fpic"]

  # requirements(): Define the dependencies of the package
  def requirements(self):
    self.output.info("conanfile.py: requirements()")

    self.requires(f"libgit2/1.7.2")

  def configure(self):
    self.output.info("conanfile.py: configure()")

    self.options["libgit2"].shared = False
    self.options["libgit2"].fPIC = True
    self.options["libgit2"].with_ntlmclient = False
    self.options["libgit2"].with_sha1 = "openssl"

  def layout(self):
    cmake_layout(self)

  def build(self):
    self.output.info("conanfile.py: configure()")
    cmake = CMake(self)
    cmake.configure()
    cmake.build()

