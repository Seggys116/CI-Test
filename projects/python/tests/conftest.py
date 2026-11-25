from __future__ import annotations

from hypothesis import settings

settings.register_profile("ci", database=None, derandomize=True, deadline=None)
settings.load_profile("ci")
