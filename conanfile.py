from conans import ConanFile, CMake, tools
import os


class xlsxioConan(ConanFile):
    name = "xlsxio"
    version = "0.2.33"
    url = "https://github.com/brechtsanders/xlsxio"
    homepage = "https://zlib.net"
    license = ""
    generators = "cmake"
    exports_sources = "CMakeLists.txt", "build/*", "include/*", "lib/*", "src/*", "CMake/*", "Makefile", "template*"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "libzip": [True, False],
        "pc_files": [True, False],
        "tools": [True, False],
        "examples": [True, False],
        "write": [True, False]
    }
    
    default_options = { 
        "minizip:bzip2": False, # Disable bzip2 for minizip, because it is not necessary and break building
        "shared": True,
        "libzip": False, # Not implemented another option
        "pc_files": False,
        "tools": False,
        "examples": False,
        "write": True
    }
    
    build_requires = ("minizip/1.2.12",
                      "expat/2.2.7")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_SHARED"] = self.options.shared
        cmake.definitions["BUILD_STATIC"] = not self.options.shared
        cmake.definitions["WITH_LIBZIP"] = self.options.libzip
        cmake.definitions["BUILD_PC_FILES"] = self.options.pc_files
        cmake.definitions["BUILD_TOOLS"] = self.options.tools
        cmake.definitions["BUILD_EXAMPLES"] = self.options.examples
        cmake.configure()
        cmake.build()
                    
    def package(self):
        self.copy("*.h", src="include", dst="include")
        
        if os.getcwd() == self.build_folder:
            if self.options.shared:
                self.copy("*.dll", src=self.build_folder, dst="bin", keep_path=False)
                self.copy("*.so*", src=self.build_folder, dst="bin", symlinks=True, keep_path=False)
            else:
                self.copy("*{0}.lib".format(self.name), src=self.build_folder, dst="lib", keep_path=False)
                self.copy("*.a", src=self.build_folder, dst="lib", keep_path=False)
                    
    
    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libs = ["{0}_read".format(self.name)]
        if self.options.write:
            self.cpp_info.libs + ["{0}_write".format(self.name)]
        if not self.options.shared:
            self.cpp_info.defines = ["{0}_STATICLIB".format(self.name.upper())]
        
        