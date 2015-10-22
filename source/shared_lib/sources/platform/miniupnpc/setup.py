#! /usr/bin/python
# $Id: setup.py,v 1.11 2015/07/09 12:43:29 nanard Exp $
# the MiniUPnP Project (c) 2007-2014 Thomas Bernard
# http://miniupnp.tuxfamily.org/ or http://miniupnp.free.fr/
#
# python script to build the miniupnpc module under unix
#
# replace libminiupnpc.a by libminiupnpc.so for shared library usage
from setuptools import setup, Extension
from distutils import sysconfig
sysconfig.get_config_vars()["OPT"] = ''
sysconfig.get_config_vars()["CFLAGS"] = ''
setup(name="miniupnpc",
      version=open('VERSION').read().strip(),
      author='Thomas BERNARD',
      author_email='miniupnp@free.fr',
      license=open('LICENSE').read(),
      url='http://miniupnp.free.fr/',
      description='miniUPnP client',
      ext_modules=[
         Extension(name="miniupnpc", sources=["miniupnpcmodule.c"],
                   extra_objects=["libminiupnpc.a"])
      ])

