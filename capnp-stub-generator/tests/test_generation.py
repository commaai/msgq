"""Tests the capnproto stub generator."""
from __future__ import annotations

import os

from capnp_stub_generator.cli import main

here = os.path.dirname(__file__)


def test_generation():
    """Compare the generated output to a reference file."""
    main(
        [
            "-p",
            os.path.join(here, "dummy.capnp"),
            "-c",
            os.path.join(here, "dummy_capnp.pyi"),
        ]
    )

    with open(os.path.join(here, "dummy_capnp.pyi"), encoding="utf8") as test_file:
        test_data = test_file.readlines()

    with open(os.path.join(here, "ref_dummy_capnp.pyi_nocheck"), encoding="utf8") as ref_file:
        ref_data = ref_file.readlines()

    assert test_data == ref_data
