=================================================
SkART - Skyrmion racetrack memory Adaptive Radix Tree
=================================================

|Based on|

.. |Based on| github link: https://github.com/blackwithwhite666/pyart

Installing
==========

pystat can be installed via pypi:

::

    pip install pyart


Building
========

Get the source:

::

    git clone https://github.com/MiaHsieh9003/python_art
    cd pyart


Excute steps
================

install
::
    Step 1: pip install . --user

    note: if need clean and rebuild 
    .. python3 setup.py clean --all
    .. rm -rf build/ pyart/*.so
    .. pip install . --user

Compile extension:
::

    Step2: python setup.py build_ext --inplace

python test
:: 
    Step3: excute tests/art_ycsb.py