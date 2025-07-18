# 透過 setup.py 編譯 C 擴充模組（pyart.tree:含 tree.pyx 和 art.c）
import os
import re
import sys

from setuptools import setup, Extension, find_packages
from setuptools.command.sdist import sdist
from setuptools.command.build_ext import build_ext
from Cython.Build import cythonize
#-----------------------------------------------------------------------------
# Flags and default values.
#-----------------------------------------------------------------------------

PACKAGE = 'pyart'

cmdclass = {}
extensions = []
extension_kwargs = {}

# try to find cython
try:
    from Cython.Distutils import build_ext as build_ext_c
    cython_installed = True
except ImportError:
    cython_installed = False


# current location 代表目前 setup.py 檔案所在資料夾的絕對路徑
here = os.path.abspath(os.path.dirname(__file__))


#-----------------------------------------------------------------------------
# Commands
#-----------------------------------------------------------------------------

class CheckSDist(sdist):
    """Custom sdist that ensures Cython has compiled all pyx files to c."""

    def initialize_options(self):
        sdist.initialize_options(self)
        self._pyxfiles = []
        for root, dirs, files in os.walk(PACKAGE):
            for f in files:
                if f.endswith('.pyx'):
                    self._pyxfiles.append(os.path.join(root, f))

    def run(self):
        if 'cython' in cmdclass:
            self.run_command('cython')
        else:
            for pyxfile in self._pyxfiles:
                cfile = pyxfile[:-3] + 'c'
                msg = "C-source file '%s' not found."%(cfile)+\
                " Run 'setup.py cython' before sdist."
                assert os.path.isfile(cfile), msg
        sdist.run(self)

cmdclass['sdist'] = CheckSDist


if cython_installed:

    class CythonCommand(build_ext_c):
        """Custom distutils command subclassed from Cython.Distutils.build_ext
        to compile pyx->c, and stop there. All this does is override the
        C-compile method build_extension() with a no-op."""

        description = "Compile Cython sources to C"

        def build_extension(self, ext):
            pass

    class zbuild_ext(build_ext_c):
        def run(self):
            from distutils.command.build_ext import build_ext as _build_ext
            return _build_ext.run(self)

            # os.system("cython --emit-h pyart/SK-RM/art_skrm.pyx")

    cmdclass['cython'] = CythonCommand
    cmdclass['build_ext'] = zbuild_ext

else:

    class CheckingBuildExt(build_ext):
        """Subclass build_ext to get clearer report if Cython is neccessary."""

        def check_cython_extensions(self, extensions):
            for ext in extensions:
                for src in ext.sources:
                    msg = "Cython-generated file '%s' not found." % src
                    assert os.path.exists(src), msg

        def build_extensions(self):
            self.check_cython_extensions(self.extensions)
            self.check_extensions_list(self.extensions)

            for ext in self.extensions:
                self.build_extension(ext)

    cmdclass['build_ext'] = CheckingBuildExt


#-----------------------------------------------------------------------------
# Extensions
#-----------------------------------------------------------------------------


suffix = '.pyx' if cython_installed else '.c'


def source_extension(name):
    parts = name.split('.')
    parts[-1] = parts[-1] + suffix
    return os.path.join(PACKAGE, *parts)


def prepare_sources(sources):
    def to_string(s):
        # if isinstance(s, unicode):
        if isinstance(s, bytes):
            return s.decode('utf-8')
        return s
    return [to_string(source) for source in sources]


modules = {
    # 'art_skrm': dict(
    #     include_dirs=[
    #         os.path.join(here, 'pyart'),
    #     ],
    #     sources=[
    #         os.path.join(here, 'pyart', 'art_skrm.pyx'),
    #     ],
    #     extra_compile_args=['-std=c99'],
    # ),
    'tree': dict(
        include_dirs=[
            os.path.join(here, 'src'),
            os.path.join(here, 'src', 'SK-RM'),
        ],
        sources=[
            # source_extension('tree'),
            os.path.join(here, 'pyart', 'tree.pyx'),
            os.path.join(here, 'src', 'art.c'),
            os.path.join(here, 'src', 'SK-RM', 'artSkrm.c'),
            os.path.join(here, 'src', 'SK-RM', 'hashTable.c')
            # 'src'	表示你有一個子資料夾叫做 src（可能存放 C 程式）; 'art.c'	你要編譯的 C 原始碼檔案名稱
        ],
        extra_compile_args=['-std=c99'],
    ),
}

# collect extensions
for module, kwargs in modules.items():
    kwargs = dict(extension_kwargs, **kwargs)
    kwargs.setdefault('sources', [source_extension(module)])
    kwargs['extra_compile_args'].extend(['-g', '-O0', '-Wno-unused-function']) # close warning from unused function
    kwargs['sources'] = prepare_sources(kwargs['sources'])
    ext = Extension('{0}.{1}'.format(PACKAGE, module), **kwargs)
    if suffix == '.pyx' and ext.sources[0].endswith('.c'):
        # undo setuptools stupidly clobbering cython sources:
        ext.sources = kwargs['sources']
    extensions.append(ext)

# extensions = [
#     Extension(
#         name="pyart.tree",
#         sources=[
#             "pyart/tree.pyx",        # 主模組
#             "src/art.c",             # C 函式
#             "src/SK-RM/art_skrm.pyx" # ⬅ 加這個，確保含有 art_track_domain_trans()
#         ],
#         include_dirs=["src", "src/SK-RM"],
#         language="c",
#     )
# ]
# extensions.append(
#     Extension(
#         "pyart.tree",  # 注意：需寫成 package.module
#         sources=[
#             "pyart/tree.pyx",
#             "src/art.c",
#         ],
#         extra_compile_args=["-std=c99"],
#     )
# )
# extensions.append(
#     Extension(
#         name="pyart.art_skrm",
#         sources=["src/SK-RM/art_skrm.pyx"],
#         include_dirs=["src", "src/SK-RM"],
#         extra_compile_args=["-std=c99"],
#     )
# )

#-----------------------------------------------------------------------------
# Description, version and other meta information.
#-----------------------------------------------------------------------------

re_meta = re.compile(r'__(\w+?)__\s*=\s*(.*)')
re_vers = re.compile(r'VERSION\s*=\s*\((.*?)\)')
re_doc = re.compile(r'^"""(.+?)"""')
rq = lambda s: s.strip("\"'")


def add_default(m):
    attr_name, attr_value = m.groups()
    return ((attr_name, rq(attr_value)),)


def add_version(m):
    v = list(map(rq, m.groups()[0].split(', ')))
    return (('VERSION', '.'.join(v[0:3]) + ''.join(v[3:])),)


def add_doc(m):
    return (('doc', m.groups()[0]),)

pats = {re_meta: add_default,
        re_vers: add_version,
        re_doc: add_doc}
meta_fh = open(os.path.join(here, '{0}/__init__.py'.format(PACKAGE)), encoding="utf-8")
try:
    meta = {}
    for line in meta_fh:
        if line.strip() == '# -eof meta-':
            break
        for pattern, handler in pats.items():
            m = pattern.match(line.strip())
            if m:
                meta.update(handler(m))
finally:
    meta_fh.close()

with open(os.path.join(here, 'README.rst')) as f:
    README = f.read()

with open(os.path.join(here, 'CHANGES.rst')) as f:
    CHANGES = f.read()


#-----------------------------------------------------------------------------
# Setup
#-----------------------------------------------------------------------------

setup(
    name=PACKAGE,
    version=meta['VERSION'],
    description=meta['doc'],
    author=meta['author'],
    author_email=meta['contact'],
    url=meta['homepage'],
    long_description=README + '\n\n' + CHANGES,
    keywords='thrift soa',
    license='BSD-3-Clause',
    cmdclass=cmdclass,
    ext_modules=cythonize(extensions, language_level="3"),  #, compiler_directives={"language_level": 3}
    packages=find_packages(),
    install_requires=[],
    zip_safe=False,
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        # "License :: OSI Approved :: BSD License",
        "Operating System :: POSIX",
        "Programming Language :: Python :: 3.11",  # 2.7
    ],
)
