<div align="center" style="text-align: center;">

<h1>MSGQ</h1>
<p><b>High-performance pub/sub messaging, made simple.<br>For Python, C, and C++.</b></p>

<h3>
  <a href="#quickstart">Quickstart</a>
  <span> · </span>
  <a href="examples/">Examples</a>
  <span> · </span>
  <a href="https://discord.comma.ai">Discord</a>
</h3>

[![Discord](https://img.shields.io/badge/Discord-Join-5865F2?logo=discord&logoColor=white)](https://discord.comma.ai)
[![Tests](https://github.com/commaai/msgq/actions/workflows/tests.yml/badge.svg?branch=master)](https://github.com/commaai/msgq/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

</div>

---

MSGQ lets programs on the same machine exchange messages. A publisher sends messages to a named endpoint, and subscribers listen on that same endpoint. Each endpoint supports one publisher and multiple subscribers.

MSGQ is a generic high performance IPC pub sub system with a single publisher and multiple subscribers. It uses a ring buffer in shared memory to efficiently read and write data. Each read requires a copy. Writing can be done without a copy, as long as the size of the data is known in advance. This library also provides a spoofed implementation that can be used for deterministic testing, and visionipc, an IPC system specifically for large contiguous buffers (like images/video).

<p align="center">
  <img src="examples/benchmark.png" alt="Verified 1 KiB cross-process ping-pong: delivered messages per second, counting requests and replies."><br>
  <sub>1 KiB cross-process ping-pong on x86 Linux; counts requests + replies; Zenoh tuned.</sub>
</p>

<details>
<summary>Benchmark details and reproduction</summary>

Two spawned processes exchange one request and reply at a time with blocking receives.
Both verify every sequence number and full payload. Each round trip counts as two delivered messages;
timing includes Python allocation and verification. This measures ping-pong performance, not maximum
streaming throughput or fan-out capacity.

The chart shows the median of five shuffled samples, each lasting at least 2 seconds and 1,000 round trips,
after at least 100 warm-up exchanges and 0.1 seconds. Tests ran in Ubuntu 24.04 on a Linux VM with four AMD EPYC vCPUs.
MSGQ uses shared memory with its default blocking receive; elapsed time includes fallback polling stalls.
pyzmq and Zenoh use Unix sockets, LCM uses host-local UDP multicast,
and `multiprocessing.Pipe` uses raw bytes without pickling.
Zenoh disables QoS prioritization and uses its low-latency transport for 64-byte and 1 KiB messages.
It uses standard transport for 64 KiB because low-latency mode cannot fragment large messages.
Native blocking receive and the default shared-memory settings are retained.
Use `--no-zenoh-tuned` to compare the original Zenoh defaults.
The five 1 KiB samples varied as follows; all samples, including stalls, are retained.

| Backend | Median messages/sec | Sample range |
| --- | ---: | ---: |
| MSGQ | 362k | 333–372k |
| pyzmq | 114k | 106–118k |
| multiprocessing.Pipe | 232k | 229–232k |
| Zenoh | 86k | 85–88k |
| LCM | 174k | 172–174k |

See [raw samples and environment](examples/benchmark-results.json) and the [benchmark script](examples/benchmark.py).

```sh
uv run examples/benchmark.py --json examples/benchmark-results.json --plot examples/benchmark.png
```

</details>

## Quickstart

```sh
python -m pip install git+https://github.com/commaai/msgq.git
```

From a local checkout, run the [publisher](examples/publisher.py) and [subscriber](examples/subscriber.py) in separate terminals:

```sh
python examples/publisher.py   # terminal 1
python examples/subscriber.py  # terminal 2
```

The subscriber prints `Hello from MSGQ!` once per second.

The core API sends and receives bytes:

```python
import msgq

publisher = msgq.pub_sock("hello")
subscriber = msgq.sub_sock("hello")
publisher.send(b"Hello from MSGQ!")
print(subscriber.receive())  # b'Hello from MSGQ!'
```

## Contributing

Issues and pull requests are welcome on [GitHub](https://github.com/commaai/msgq). Run `./test.sh` to build, lint, and test the package.

## License

MSGQ is available under the [MIT License](LICENSE).

## Under the hood

### Storage
The storage for the queue consists of an area of metadata, and the actual buffer. The metadata contains:

1. A counter to the number of readers that are active
2. A pointer to the head of the queue for writing. From now on referred to as *write pointer*
3. A cycle counter for the writer. This counter is incremented when the writer wraps around
4. N pointers, pointing to the current read position for all the readers. From now on referred to as *read pointer*
5. N counters,  counting the number of cycles for all the readers
6. N booleans, indicating validity for all the readers. From now on referred to as *validity flag*

The counter and the pointer are both 32 bit values, packed into 64 bit so they can be read and written atomically.

The data buffer is a ring buffer. All messages are prefixed by an 8 byte size field, followed by the data. A size of -1 indicates a wrap-around, and means the next message is stored at the beginning of the buffer.


### Writing
Writing involves the following steps:

1. Check if the area that is to be written overlaps with any of the read pointers, mark those readers as invalid by clearing the validity flag.
2. Write the message
3. Increase the write pointer by the size of the message

In case there is not enough space at the end of the buffer, a special empty message with a prefix of -1 is written. The cycle counter is incremented by one. In this case step 1 will check there are no read pointers pointing to the remainder of the buffer. Then another write cycle will start with the actual message.

There always needs to be 8 bytes of empty space at the end of the buffer. By doing this there is always space to write the -1.

### Reset reader
When the reader is lagging too much behind the read pointer becomes invalid and no longer points to the beginning of a valid message. To reset a reader to the current write pointer, the following steps are performed:

1. Set valid flag
2. Set read cycle counter to that of the writer
3. Set read pointer to write pointer

### Reading
Reading involves the following steps:

1. Read the size field at the current read pointer
2. Read the validity flag
3. Copy the data out of the buffer
4. Increase the read pointer by the size of the message
5. Check the validity flag again

Before starting the copy, the valid flag is checked. This is to prevent a race condition where the size prefix was invalid, and the read could read outside of the buffer. Make sure that step 1 and 2 are not reordered by your compiler or CPU.

If a writer overwrites the data while it's being copied out, the data will be invalid. Therefore the validity flag is also checked after reading it. The order of step 4 and 5 does not matter.

If at steps 2 or 5 the validity flag is not set, the reader is reset. Any data that was already read is discarded. After the reader is reset, the reading starts from the beginning.

If a message with size -1 is encountered, step 3 and 4 are replaced by increasing the cycle counter and setting the read pointer to the beginning of the buffer. After that another read is performed.
