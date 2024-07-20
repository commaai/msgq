#!/bin/bash
brew bundle --file=- <<-EOS
brew "zeromq"
cask "gcc-arm-embedded"
brew "gcc@13"
EOS
