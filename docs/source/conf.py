# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Sumo Firmware'
copyright = '2026-%Y, Equipe Paralela'
author = 'Guilherme Nunes Trofino'
release = 'v1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'exhale',
    'sphinx_design',
    'sphinx.ext.duration',
    "sphinx.ext.intersphinx",
    'sphinxcontrib.mermaid',
]

breathe_projects = {
    "firmware": "./firmware/api/xml"
}
breathe_default_project = "firmware"

exhale_args = {
    "createTreeView":       True,
    "containmentFolder":    "./firmware/api/rst",
    "doxygenStripFromPath": "./firmware/api/xml",
    "rootFileName":         "index.rst",
    "rootFileTitle":        "API",
}

primary_domain = 'c'

templates_path = ['_templates']
exclude_patterns = []

intersphinx_mapping = {
    "paralela-template": ("https://paralela-template.readthedocs.io/en/latest/", None),
}



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
