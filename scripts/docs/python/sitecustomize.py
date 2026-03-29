"""Docs build compatibility patches.

Loaded automatically via PYTHONPATH by docs build/serve scripts.
"""

from pygments.formatters.html import HtmlFormatter


_original_html_formatter_init = HtmlFormatter.__init__


def _syphax_html_formatter_init(self, **options):
	"""Normalize None filenames for Python 3.14 + Pygments HTML formatter."""
	if options.get("filename") is None:
		options["filename"] = ""
	_original_html_formatter_init(self, **options)


HtmlFormatter.__init__ = _syphax_html_formatter_init
