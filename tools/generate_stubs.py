import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MODULES = ("msgq.ipc_pyx", "msgq.visionipc.visionipc_pyx")


def main() -> None:
  parser = argparse.ArgumentParser(description="Generate nanobind stubs, or check that committed stubs are current.")
  parser.add_argument("--check", action="store_true")
  args = parser.parse_args()

  with tempfile.TemporaryDirectory() as directory:
    for module in MODULES:
      target = ROOT.joinpath(*module.split(".")).with_suffix(".pyi")
      output = Path(directory) / target.name if args.check else target
      subprocess.check_call([sys.executable, "-m", "nanobind.stubgen", "-i", str(ROOT), "-m", module, "-o", str(output), "-q"])
      # Older nanobind versions leave blank lines when a stub has no imports.
      content = output.read_text().lstrip()
      if args.check:
        if content != target.read_text():
          sys.exit(f"{target.relative_to(ROOT)} is out of date; run python tools/generate_stubs.py")
      else:
        output.write_text(content)


if __name__ == "__main__":
  main()
