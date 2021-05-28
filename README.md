GR - a universal framework for visualization applications
=========================================================

[![MIT license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![GitHub tag](https://img.shields.io/github/tag/jheinen/gr.svg)](https://github.com/jheinen/gr/releases)
[![PyPI version](https://img.shields.io/pypi/v/gr.svg)](https://pypi.python.org/pypi/gr)
[![DOI](https://zenodo.org/badge/17747322.svg)](https://zenodo.org/badge/latestdoi/17747322)

*GR* is a universal framework for cross-platform visualization applications.
It offers developers a compact, portable and consistent graphics library for
their programs. Applications range from publication quality 2D graphs to the
representation of complex 3D scenes.

*GR* is essentially based on an implementation of a Graphical Kernel System (GKS).
As a self-contained system it can quickly and easily be integrated into existing
applications (i.e. using the `ctypes` mechanism in Python or `ccall` in Julia).

The *GR* framework can be used in imperative programming systems or integrated
into modern object-oriented systems, in particular those based on GUI toolkits.
*GR* is characterized by its high interoperability and can be used with modern
web technologies. The *GR* framework is especially suitable for real-time
or signal processing environments.

*GR* was developed by the Scientific IT-Systems group at the Peter Grünberg
Institute at Forschunsgzentrum Jülich. The main development has been done
by Josef Heinen who currently maintains the software, but there are other
developers who currently make valuable contributions. Special thanks to
Florian Rhiem (*GR3*) and Christian Felder (qtgr, setup.py).

Starting with release 0.6 *GR* can be used as a backend
for [Matplotlib](http://matplotlib.org) and significantly improve
the performance of existing Matplotlib or PyPlot applications written
in Python or Julia, respectively.
In [this](http://gr-framework.org/tutorials/matplotlib.html) tutorial
section you can find some examples.

Beginning with version 0.10.0 *GR* supports inline graphics which shows
up in IPython's Qt Console or interactive computing environments for *Python*
and *Julia*, such as [IPython](http://ipython.org) and
[Jupyter](https://jupyter.org). An interesting example can be found
[here](http://pgi-jcns.fz-juelich.de/pub/doc/700K_460.html).

## Installation and Getting Started

To install *GR* and try it using *Python*, *Julia* or *C*, please see the corresponding documentation:

- [Python package gr](https://gr-framework.org/python.html)
- [Julia package GR](https://gr-framework.org/julia.html)
- [C library GR](https://gr-framework.org/c.html)
- [Ruby package GR](https://github.com/red-data-tools/GR.rb)

## Documentation

You can find more information about *GR* on the [GR home page](http://gr-framework.org).

## Contributing

If you want to improve *GR*, please read the [contribution guide](https://github.com/sciapp/gr/blob/develop/CONTRIBUTING.md) for a few notes on how to report issues or submit changes.

## Support

If you have any questions about *GR* or run into any issues setting up or running GR, please [open an issue on GitHub](https://github.com/sciapp/gr/issues/new), either in this repo or in the repo for the language binding you are using ([Python](https://github.com/sciapp/python-gr/issues/new), [Julia](https://github.com/jheinen/GR.jl/issues/new), [Ruby](https://github.com/red-data-tools/GR.rb/issues/new)).
