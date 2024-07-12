# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath("."))

# -- Project information -----------------------------------------------------

project = "Gaia"
copyright = "2023, Gaia contributors"
author = "Gaia contributors"


# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
needs_sphinx = "4.5"

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named "sphinx.ext.*") or your custom
# ones.
# [[[ begin extensions marker ]]]
# Tell Sphinx to use both the `breathe` and `exhale` extensions
extensions = [
    "breathe",
    "exhale",
    "myst_parser",
]

# Setup the `breathe` extension
breathe_projects = {"ExhaleCompanion": "./_doxygen/xml"}
breathe_default_project = "ExhaleCompanion"

# Setup the `exhale` extension
from textwrap import dedent
exhale_args = {
    ############################################################################
    # These arguments are required.                                            #
    ############################################################################
    "containmentFolder":     "./reference",
    "rootFileName":          "code_reference.rst",
    "rootFileTitle":         "Code Reference",
    "doxygenStripFromPath":  "../src",
    ############################################################################
    # Suggested optional arguments.                                            #
    ############################################################################
    "createTreeView":        True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": dedent('''
        INPUT       = ../src
        RECURSIVE = true
        EXCLUDE_PATTERNS = *.cpp *.c
    '''),
    ############################################################################
    # HTML Theme specific configurations.                                      #
    ############################################################################
    # Fix broken Sphinx RTD Theme 'Edit on GitHub' links
    # Search for 'Edit on GitHub' on the FAQ:
    #     http://exhale.readthedocs.io/en/latest/faq.html
    "pageLevelConfigMeta": ":github_url: https://github.com/nyx-org/gaia",
    ############################################################################
    # Individual page layout example configuration.                            #
    ############################################################################
    # Example of adding contents directives on custom kinds with custom title
    "contentsTitle": "Page Contents",
    "kindsWithContentsDirectives": ["class", "file", "namespace", "struct"],
    # This is a testing site which is why I'm adding this
    "includeTemplateParamOrderList": True,
    ############################################################################
    # useful to see ;)
    "verboseBuild": False 
}

# Tell sphinx what the primary language being documented is.
primary_domain = "cpp"

# Tell sphinx what the pygments highlight language should be.
highlight_language = "cpp"
# [[[ end extensions marker ]]]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "*.cpp", "*.c"]

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
import exhale
# NOTE: this is the companion site for Exhale, which is why I'm setting the
#       version to be the same.  For your own projects, you would NOT do this!
version = exhale.__version__
# The full version, including alpha/beta/rc tags.
release = exhale.__version__

# -- Options for HTML output -------------------------------------------------

# [[[ begin theme marker ]]]
# The name of the Pygments (syntax highlighting) style to use.
# `sphinx` works very well with the RTD theme, but you can always change it
pygments_style = "sphinx"

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "alabaster"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]
# [[[ end theme marker ]]]

rst_epilog = ".. |theme| replace:: ``{0}``".format(html_theme)