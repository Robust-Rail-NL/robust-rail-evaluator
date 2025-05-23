import os
import re
import sys
import platform
import subprocess
# import argparse



from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)



class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        
        conda_env = os.environ.get("CONDA_PREFIX")  # Get active Conda environment path

        if not conda_env:
            print("Warning: No Conda environment detected! Using system Python.")
        else:
            print(f"Detected Conda environment: {conda_env}")
        
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep
        
        
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        conda_env = os.environ.get("CONDA_PREFIX")  # Get active Conda environment path
        
        # Adds the root path to the conda environment 
        cmake_args+= ['-DCONDA_ENV=' + conda_env]
        
        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=ext.sourcedir+"/build", env=env)
        subprocess.check_call(['cmake', '--build', '.', '--target', 'pyTORS_pyi'] + build_args, cwd=ext.sourcedir+"/build")

setup(
    name='pyTORS',
    version='0.2',
    author='Delft University of Technology',
    author_email='J.G.M.vanderLinden@tudelft.nl',
    description='A python wrapper for the cTORS library',
    long_description='',
    ext_modules=[CMakeExtension('pyTORS')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
