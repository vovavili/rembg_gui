=============
GUI for rembg
=============

.. image:: example.png
    :align: center


A simple PyQt6 GUI for rembg_, a tool to remove image backgrounds.

You can run the Python script directly with uv:

.. code-block:: powershell

    uv run gui.py

The inline dependencies use ``rembg[cpu]``. That is the portable default. If
you want CUDA, create your own environment with ``rembg[gpu]`` after your
ONNX Runtime GPU stack is working.

In addition to the Python script, there is a C++/QtWidgets port in ``src_cpp/``. You can download the binary releases from the releases page.

Credits
-------

This package is a slight modification of `Two Image Synchronous Scrolling GitHub gist by acbetter <https://gist.github.com/acbetter/e7d0c600fdc0865f4b0ee05a17b858f2>`_

License
-------
MIT_

.. _rembg: https://github.com/danielgatis/rembg
.. _MIT: https://github.com/vovavili/rembg_gui/blob/master/LICENSE.md
