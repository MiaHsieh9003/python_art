"""Adaptive Radix Tree (ART) implemetation for python."""
# 有 __init__.py（即使是空的），才能被 Python 當作 package 匯入。
VERSION = (0, 2, 3)

__version__ = '.'.join(map(str, VERSION[0:3]))
__author__ = 'Lipin Dmitriy'
__contact__ = 'blackwithwhite666@gmail.com'
__homepage__ = 'https://github.com/blackwithwhite666/pyart'
__docformat__ = 'restructuredtext'

# -eof meta-

from .tree import Tree
