"""Generate a new reference file."""
from __future__ import annotations

import os

from capnp_stub_generator.cli import main as stub_generator_main

here = os.path.dirname(__file__)


def main():
    """Generate a new reference from the dummy schema."""
    stub_generator_main(
        [
            "-p",
            os.path.join(here, "dummy.capnp"),
            "-c",
            os.path.join(here, "dummy_capnp.py"),
            os.path.join(here, "dummy_capnp.pyi"),
            os.path.join(here, "ref_dummy_capnp.pyi_nocheck"),
        ]
    )

    os.rename(os.path.join(here, "dummy_capnp.pyi"), os.path.join(here, "ref_dummy_capnp.pyi_nocheck"))


if __name__ == "__main__":
    main()
