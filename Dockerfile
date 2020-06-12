FROM ubuntu:16.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    build-essential \
    ca-certificates \
    capnproto \
    cppcheck \
    clang \
    curl \
    git \
    libzmq3-dev \
    libcapnp-dev \
    libtool \
    libssl-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libffi-dev \
    liblzma-dev \
    llvm \
    make \
    python-openssl \
    tk-dev \
    xz-utils \
    wget \
    zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

RUN curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
ENV PATH="/root/.pyenv/bin:/root/.pyenv/shims:${PATH}"
RUN pyenv install 3.7.3 && \
    pyenv global 3.7.3 && \
    pyenv rehash && \
    pip3 install --no-cache-dir pyyaml==5.1.2 Cython==0.29.14 scons==3.1.1 pycapnp==0.6.4 pre-commit==2.4.0 pylint==2.5.2

WORKDIR /project/cereal

ENV PYTHONPATH=/project

COPY . .
RUN rm -rf .git && \
    scons -c && scons -j$(nproc)
