#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd "$DIR"

if ! command -v uv &>/dev/null; then
  echo "'uv' is not installed. Installing 'uv'..."
  curl -LsSf https://astral.sh/uv/install.sh | sh

  # doesn't require sourcing on all platforms
  set +e
  source "$HOME/.local/bin/env"
  set -e
fi

case "$(uname -s)" in MINGW*|MSYS*) export UV_PYTHON="${UV_PYTHON:-3.12}";; esac  # not MSYS2's own python, whose wheels are incompatible

export UV_PROJECT_ENVIRONMENT="$DIR/.venv"
uv sync --all-extras
source "$DIR"/.venv/*/activate  # bin/ on POSIX, Scripts/ on Windows
