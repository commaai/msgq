"""Benchmark same-process pub/sub: python examples/benchmark.py --help."""

import argparse
from contextlib import ExitStack
import multiprocessing
import os
from pathlib import Path
import platform
import socket
import statistics
import tempfile
import time
import uuid

import msgq


def positive_int(value):
  number = int(value)
  if number <= 0:
    raise argparse.ArgumentTypeError("must be greater than zero")
  return number


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--iterations", type=positive_int, default=10_000, help="messages per run (default: %(default)s)")
  parser.add_argument("--repeat", type=positive_int, default=5, help="runs per message size (default: %(default)s)")
  parser.add_argument("--zmq", action="store_true", help="also benchmark pyzmq XPUB/SUB over IPC (requires pyzmq)")
  parser.add_argument("--pipe", action="store_true", help="also benchmark multiprocessing.Pipe with raw bytes")
  parser.add_argument("--plot", type=Path, help="save a chart, e.g. examples/benchmark.png (requires matplotlib)")
  args = parser.parse_args()

  zmq = None
  if args.zmq:
    try:
      import zmq
    except ImportError:
      parser.error("--zmq requires pyzmq: python -m pip install pyzmq")

  plt = None
  if args.plot:
    try:
      import matplotlib
      matplotlib.use("Agg")
      import matplotlib.pyplot as plt
    except ImportError:
      parser.error("--plot requires matplotlib: python -m pip install matplotlib")

  with ExitStack() as cleanup:
    results = run(args, zmq, cleanup)
  if plt is not None:
    plot_results(results, args, plt)


def run(args, zmq, cleanup):
  endpoint = f"msgq_benchmark_{uuid.uuid4().hex}"
  publisher = msgq.pub_sock(endpoint)
  subscriber = msgq.sub_sock(endpoint, timeout=1000)
  backends = [("msgq", publisher.send, subscriber.receive)]

  if zmq is not None:
    # Keep Unix socket paths short enough for macOS as well as Linux.
    directory = cleanup.enter_context(tempfile.TemporaryDirectory(prefix="msgq_bench_", dir="/tmp"))
    context = zmq.Context()
    cleanup.callback(context.term)
    zmq_publisher = context.socket(zmq.XPUB)
    cleanup.callback(zmq_publisher.close, linger=0)
    zmq_subscriber = context.socket(zmq.SUB)
    cleanup.callback(zmq_subscriber.close, linger=0)
    zmq_publisher.setsockopt(zmq.RCVTIMEO, 5000)
    zmq_subscriber.setsockopt(zmq.RCVTIMEO, 1000)
    zmq_subscriber.setsockopt(zmq.SUBSCRIBE, b"")
    zmq_endpoint = f"ipc://{directory}/socket"
    zmq_publisher.bind(zmq_endpoint)
    zmq_subscriber.connect(zmq_endpoint)
    # XPUB lets us wait for the subscription instead of guessing a startup delay.
    if zmq_publisher.recv() != b"\x01":
      raise RuntimeError("Unexpected ZeroMQ subscription")
    backends.append(("pyzmq IPC", zmq_publisher.send, zmq_subscriber.recv))

  if args.pipe:
    pipe_reader, pipe_writer = multiprocessing.Pipe(duplex=True)
    cleanup.callback(pipe_reader.close)
    cleanup.callback(pipe_writer.close)
    # On Linux/macOS, a duplex Pipe uses a Unix socket pair. Make room for
    # a full 64 KiB message before the same-process receiver gets to run.
    with socket.socket(fileno=os.dup(pipe_writer.fileno())) as pipe_socket:
      pipe_socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 256 * 1024)
    # Fail instead of hanging if the OS cannot buffer the entire message.
    os.set_blocking(pipe_writer.fileno(), False)
    backends.append(("Pipe bytes", pipe_writer.send_bytes, pipe_reader.recv_bytes))

  print(f"Python {platform.python_version()} | {platform.platform()}")
  if zmq is not None:
    print(f"pyzmq {zmq.__version__} | libzmq {zmq.zmq_version()} | XPUB/SUB, copying bytes")
  if args.pipe:
    print("Pipe: send_bytes/recv_bytes, duplex Unix socket pair, requested send buffer 256 KiB")
  print(f"{args.iterations:,} messages × {args.repeat} runs per size; median results, 100 warmup messages")
  print("Same-process send + receive, one message in flight, including Python overhead.")
  print("Not cross-process latency or maximum streaming throughput.\n")
  print(f"{'Backend':<12} {'Bytes':>10} {'Messages/s':>14} {'MiB/s':>12} {'µs/message':>14}")
  print("-" * 66)

  results = []
  for size in (64, 1024, 65536):
    payload = b"x" * size
    for name, send, receive in backends:
      for _ in range(100):
        send(payload)
        if receive() != payload:
          raise RuntimeError(f"{name}: warmup message was lost or corrupted")

      durations = []
      for _ in range(args.repeat):
        start = time.perf_counter()
        for _ in range(args.iterations):
          send(payload)
          received = receive()
          if received is None:
            raise RuntimeError(f"{name}: receive timed out")
        durations.append(time.perf_counter() - start)
        if received != payload:
          raise RuntimeError(f"{name}: received message was corrupted")

      seconds_per_message = statistics.median(durations) / args.iterations
      messages_per_second = 1 / seconds_per_message
      mib_per_second = messages_per_second * size / 1024**2
      results.append((name, size, messages_per_second))
      print(f"{name:<12} {size:>10,} {messages_per_second:>14,.0f} {mib_per_second:>12,.1f} {seconds_per_message * 1e6:>14.2f}", flush=True)
  return results


def plot_results(results, args, plt):
  from matplotlib.ticker import EngFormatter, MaxNLocator

  labels = {"msgq": "msgq", "pyzmq IPC": "zmq", "Pipe bytes": "multiprocessing.Pipe"}
  rows = sorted(((name, rate) for name, size, rate in results if size == 1024), key=lambda row: row[1])
  maximum = max(rate for _, rate in rows)
  formatter = EngFormatter(sep="", places=2)
  with plt.rc_context({"font.family": "sans-serif", "font.sans-serif": ["Helvetica Neue", "Arial", "DejaVu Sans"]}):
    fig, ax = plt.subplots(figsize=(10, 4.7))
    fig.set_facecolor("white")
    ax.set_xlim(0, maximum * 1.2)
    ax.set_ylim(len(rows) - 0.5, -0.5)
    ax.set_yticks([])
    # Keep grid lines out of the values on the right.
    ticks = MaxNLocator(nbins=4).tick_values(0, maximum)
    ax.set_xticks([tick for tick in ticks if 0 <= tick < maximum])
    ax.xaxis.set_major_formatter(EngFormatter(sep=""))
    ax.set_xlabel("Messages/sec — higher is better", fontsize=19, labelpad=15, color="#172b3a")
    ax.tick_params(length=0, pad=9, labelsize=16, colors="#425b6c")
    ax.set_axisbelow(True)
    ax.grid(axis="x", color="#b7b7b7", linewidth=1.5)
    for side, spine in ax.spines.items():
      spine.set_visible(side == "left")
      spine.set_color("#192a32")
      spine.set_linewidth(2)
    for index, (name, rate) in enumerate(rows):
      ax.barh(index, rate, height=0.78, color="#31cf43" if name == "msgq" else "#b9b9b9", zorder=2)
      ax.text(maximum * 0.029, index, labels[name], va="center", fontsize=22, fontweight="bold", color="#172b3a")
      ax.text(maximum * 1.173, index, formatter(rate), ha="right", va="center", fontsize=23, fontweight="bold", color="#172b3a")
    fig.subplots_adjust(left=0.055, right=0.98, top=0.95, bottom=0.24)
    args.plot.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.plot, dpi=180, facecolor=fig.get_facecolor())
    plt.close(fig)
  print(f"Chart saved to {args.plot}")


if __name__ == "__main__":
  main()
