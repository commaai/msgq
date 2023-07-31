# Stub-file generator for cap'n proto schemas

Generates Python stubs files from cap'n proto schemas.
Useful for IDE auto-completion and static type checking.

## Usage

Clone and install with pip:

```Python
pip install capnp-stub-generator
```

Run on a set of files:

```
capnp-stub-generator -p "path/to/capnp/schemas/**/*.capnp" \
    -c "path/to/output/directory/**/*_capnp.py" "path/to/output/directory/**/*_capnp.pyi" \
    -e "**/c-capnproto/**/*.capnp" \
    -r
```

where the options are

- `-p` - search paths that contain schema files
- `-c` - cleanup paths (delete matching files before generation)
- `-e` - exclude paths that shall not be converted to stubs
- `-r` - recursive file search

Currently, stub files are always created adjacent to schema files.

For a runnable example, see the [test generation script](./capnp-stub-generator/tests/test_generation.py).

## Style and packaging

This repository is a fork from a company-internal repository. Issues can be reported here, will be fixed upstream, and backported.
Therefore, this repository does not (yet) contain a style checking and packaging pipeline.

The repository may become independent in the future.
