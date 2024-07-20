#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT="$(cd $DIR/../ && pwd)"
ARCH=$(uname -m)

if [[ $SHELL == "/bin/zsh" ]]; then
  RC_FILE="$HOME/.zshrc"
elif [[ $SHELL == "/bin/bash" ]]; then
  RC_FILE="$HOME/.bash_profile"
fi

# Install brew if required
if [[ $(command -v brew) == "" ]]; then
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"

  if [[ $ARCH == "x86_64" ]]; then
      echo 'eval "$(/usr/local/homebrew/bin/brew shellenv)"' >> $RC_FILE
      eval "$(/usr/local/homebrew/bin/brew shellenv)"
  else
      echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> $RC_FILE
      eval "$(/opt/homebrew/bin/brew shellenv)"
  fi
fi

brew bundle --file=- <<-EOS
brew "zeromq"
cask "gcc-arm-embedded"
brew "gcc@13"
EOS

if [[ -z "$NO_CATCH2" ]]; then
  cd /tmp
  git clone -b v2.x --depth 1 https://github.com/catchorg/Catch2.git
  cd Catch2
  mv single_include/* "$ROOT"
  rm -rf Catch2
fi
