#!/usr/bin/env python

"""The setup script."""

try:
    from setuptools import setup, find_packages
except ImportError:
    import subprocess
    import sys

    subprocess.run(
        [sys.executable, "-m", "pip", "install", "--user", "setuptools"], check=True
    )
    from setuptools import setup, find_packages

with open("README.rst") as readme_file:
    readme = readme_file.read()

requirements = [
    "PyQt6",
    "rembg ",
    "Pillow"
]

test_requirements = []

setup(
    author="Vladimir Vilimaitis",
    author_email="vladimirvilimaitis@gmail.com",
    python_requires=">=3.10",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: End Users/Desktop",
        "License :: OSI Approved :: MIT License",
        "Natural Language :: English",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.10",
    ],
    description="A simple PyQT5 GUI for rembg, a tool to remove images background.",
    install_requires=requirements,
    license="MIT license",
    long_description=readme,
    include_package_data=True,
    keywords="rembg_gui",
    name="rembg_gui",
    packages=find_packages(include=["rembg_gui", "rembg_gui.*"]),
    test_suite="tests",
    tests_require=test_requirements,
    url="https://github.com/vovavili/rembg_gui",
    version="1.0.0",
    zip_safe=False,
)
