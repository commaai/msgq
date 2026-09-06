# /// script
# requires-python = ">=3.11"
# dependencies = ["msgq-ipc>=1.0", "pyzmq", "eclipse-zenoh>=1.10", "lcm", "matplotlib"]
# [tool.uv.sources]
# msgq-ipc = { path = ".." }
# [tool.ty.rules]
# unresolved-import = "ignore"
# ///

import os
import lcm
import zmq
import json
import msgq
import time
import zenoh
import random
import argparse
import hashlib
import platform
import tempfile
import matplotlib
import statistics
import subprocess
import multiprocessing

from pathlib import Path
from threading import Event
from collections import deque  # codespell:ignore deque
from functools import partial
from contextlib import ExitStack
from importlib.metadata import version

matplotlib.use("Agg")
import matplotlib.pyplot as plt

BACKENDS = {"msgq": "shared memory, default blocking receive", "pyzmq IPC": "XPUB/SUB, Unix socket",
            "Pipe bytes": "multiprocessing.Pipe, duplex, raw bytes", "zenoh IPC": "Unix stream, native FIFO receiver",
            "LCM UDP": "UDP multicast, TTL 0"}


def positive_int(value):
  number = int(value)
  if number <= 0:
    raise argparse.ArgumentTypeError("must be greater than zero")
  return number


def positive_seconds(value):
  number = float(value)
  if not 0 < number < float("inf"):
    raise argparse.ArgumentTypeError("must be finite and greater than zero")
  return number


def zenoh_options(size, tuned):
  if not tuned:
    return {}
  # Low-latency transport cannot fragment a 64 KiB payload (maximum frame: 65535 bytes).
  return {"transport/unicast/qos/enabled": False, "transport/unicast/lowlatency": size <= 1024}


def setup_backend(name, role, directory, prefix, barrier, pipe, cleanup, size, zenoh_tuned):
  outgoing, incoming = ("request", "reply") if role == "sender" else ("reply", "request")
  if name == "msgq":
    os.environ["OPENPILOT_PREFIX"] = prefix
    publisher = msgq.pub_sock(outgoing)
    # Creating a publisher resets its queue, so both must exist before subscribing.
    barrier.wait(10)
    subscriber = msgq.sub_sock(incoming)
    return publisher.send, subscriber.receive
  if name == "pyzmq IPC":
    context = cleanup.enter_context(zmq.Context())
    publisher = context.socket(zmq.XPUB)
    subscriber = context.socket(zmq.SUB)
    cleanup.callback(publisher.close, linger=0)
    cleanup.callback(subscriber.close, linger=0)
    publisher.setsockopt(zmq.RCVTIMEO, 5000)
    subscriber.setsockopt(zmq.RCVTIMEO, 5000)
    subscriber.setsockopt(zmq.SUBSCRIBE, b"")
    publisher.bind(f"ipc://{directory}/{outgoing}")
    subscriber.connect(f"ipc://{directory}/{incoming}")
    if publisher.recv() != b"\x01":
      raise RuntimeError("Unexpected ZeroMQ subscription")
    return publisher.send, subscriber.recv
  if name == "Pipe bytes":
    cleanup.enter_context(pipe)
    return pipe.send_bytes, pipe.recv_bytes
  if name == "zenoh IPC":
    endpoint = f"unixsock-stream/{directory}/zenoh"
    config = zenoh.Config()
    config.insert_json5("listen/endpoints", repr([endpoint] if role == "sender" else []))
    config.insert_json5("connect/endpoints", repr([] if role == "sender" else [endpoint]))
    config.insert_json5("scouting/multicast/enabled", "false")
    config.insert_json5("scouting/gossip/enabled", "false")
    for key, value in zenoh_options(size, zenoh_tuned).items():
      config.insert_json5(key, json.dumps(value))
    session = cleanup.enter_context(zenoh.open(config))
    zenoh_subscriber = session.declare_subscriber(incoming)
    cleanup.callback(zenoh_subscriber.undeclare)
    publisher = session.declare_publisher(outgoing)
    cleanup.callback(publisher.undeclare)
    ready = Event()
    listener = publisher.declare_matching_listener(lambda status: ready.set() if status.matching else None)
    cleanup.callback(listener.undeclare)
    if publisher.matching_status.matching:
      ready.set()
    if not ready.wait(10):
      raise RuntimeError("Zenoh subscriber did not become ready")
    return publisher.put, lambda: zenoh_subscriber.recv().payload.to_bytes()
  if name == "LCM UDP":
    bus = lcm.LCM("udpm://239.255.76.67:7667?ttl=0")
    messages = deque()  # codespell:ignore deque
    token = Path(directory).name
    subscriber = bus.subscribe(token + incoming, lambda channel, data: messages.append(data))
    cleanup.callback(bus.unsubscribe, subscriber)

    def receive():
      while not messages:
        if bus.handle_timeout(5000) == 0:
          raise TimeoutError("LCM receive timed out; check host multicast support")
      return messages.popleft()

    return partial(bus.publish, token + outgoing), receive
  raise ValueError(name)


def payload(sequence, padding):
  return sequence.to_bytes(8, "little") + padding


def sender(send, receive, size, args):
  padding = b"x" * (size - 8)
  sequence = 0

  def exchange():
    nonlocal sequence
    message = payload(sequence, padding)
    send(message)
    reply = receive()
    if reply != message:
      raise RuntimeError(f"Invalid reply at sequence {sequence}: {None if reply is None else (len(reply), reply[:8])}")
    sequence += 1

  start = time.perf_counter()
  while sequence < 100 or time.perf_counter() - start < 0.1:
    exchange()
  send(b"BEGIN")
  if receive() != b"BEGIN":
    raise RuntimeError("Invalid measurement handshake")
  warm_up_count = sequence
  cpu_start = time.process_time()
  start = time.perf_counter()
  while True:
    for _ in range(64):
      exchange()
    elapsed = time.perf_counter() - start
    count = sequence - warm_up_count
    if count >= args.iterations and elapsed >= args.seconds:
      break
  cpu = time.process_time() - cpu_start
  # Completion is outside timing; no retransmissions or silent loss tolerance.
  send(b"END")
  if receive() != b"END":
    raise RuntimeError("Invalid completion handshake")
  return {"round_trips": count, "seconds": elapsed, "cpu_seconds": cpu, "warm_up_round_trips": warm_up_count}


def echo(send, receive, size):
  padding = b"x" * (size - 8)
  sequence = 0
  measured_start = None
  while True:
    message = receive()
    if message == b"BEGIN" and measured_start is None:
      measured_start = sequence
      send(message)
      cpu_start = time.process_time()
    elif message == b"END" and measured_start is not None:
      cpu = time.process_time() - cpu_start
      send(message)
      return {"round_trips": sequence - measured_start, "cpu_seconds": cpu}
    else:
      if message != payload(sequence, padding):
        raise RuntimeError(f"Invalid request at sequence {sequence}: {None if message is None else (len(message), message[:8])}")
      sequence += 1
      send(message)


def peer(name, role, directory, prefix, barrier, pipe, control, size, args):
  try:
    with ExitStack() as cleanup:
      send, receive = setup_backend(name, role, directory, prefix, barrier, pipe, cleanup, size, args.zenoh_tuned)
      barrier.wait(10)
      result = sender(send, receive, size, args) if role == "sender" else echo(send, receive, size)
    control.send(("ok", result))
  except BaseException as error:
    control.send(("error", f"{name} {role}: {type(error).__name__}: {error}"))
  finally:
    pipe.close()
    control.close()


def measure(name, size, args):
  context = multiprocessing.get_context("spawn")
  shared_root = "/tmp" if platform.system() == "Darwin" else "/dev/shm"
  with tempfile.TemporaryDirectory(prefix="mq_", dir="/tmp") as directory, \
       tempfile.TemporaryDirectory(prefix="msgq_", dir=shared_root) as shared:
    # An owned prefix isolates and removes MSGQ files without touching existing queues.
    prefix = Path(shared).name.removeprefix("msgq_")
    barrier = context.Barrier(2)
    pipes = context.Pipe()
    processes, controls = [], []
    deadline = time.monotonic() + args.timeout
    try:
      for role, pipe in zip(("sender", "echo"), pipes, strict=True):
        parent, child = context.Pipe(duplex=False)
        controls.append(parent)
        process = context.Process(target=peer, args=(name, role, directory, prefix, barrier, pipe, child, size, args))
        process.start()
        processes.append(process)
        child.close()
      for pipe in pipes:
        pipe.close()
      results = {}
      while len(results) < 2:
        if time.monotonic() >= deadline:
          raise TimeoutError(f"{name}: trial exceeded {args.timeout}s")
        for index, control in enumerate(controls):
          if index in results:
            continue
          if control.poll(0.05):
            try:
              status, result = control.recv()
            except EOFError as error:
              raise RuntimeError(f"{name}: peer {index} closed its result pipe unexpectedly") from error
            if status != "ok":
              raise RuntimeError(result)
            results[index] = result
          elif processes[index].exitcode is not None:
            raise RuntimeError(f"{name}: peer exited without results ({processes[index].exitcode})")
      for process in processes:
        process.join(max(0, deadline - time.monotonic()))
        if process.exitcode != 0:
          raise RuntimeError(f"{name}: peer failed to exit cleanly")
      result = results[0]
      if result["round_trips"] != results[1]["round_trips"]:
        raise RuntimeError(f"{name}: peers disagree on verified message count")
      result.update(backend=name, bytes=size, receiver_cpu_seconds=results[1]["cpu_seconds"])
      if name == "zenoh IPC":
        result["zenoh_overrides"] = zenoh_options(size, args.zenoh_tuned)
      return result
    finally:
      for process in processes:
        if process.is_alive():
          process.terminate()
      for process in processes:
        process.join(2)
        if process.is_alive():
          process.kill()
          process.join()
      for connection in (*controls, *pipes):
        connection.close()


def metadata():
  root = Path(__file__).resolve().parents[1]
  def git(*arguments):
    try:
      result = subprocess.run(["git", "-C", str(root), *arguments], capture_output=True, text=True, check=False)
      return result.stdout.strip() if result.returncode == 0 else None
    except OSError:
      return None

  cpu = platform.processor()
  if Path("/proc/cpuinfo").exists():
    cpu = next((line.split(":", 1)[1].strip() for line in Path("/proc/cpuinfo").read_text().splitlines()
                if line.startswith("model name")), cpu)
  sources = sorted(path for path in (root / "msgq").rglob("*") if path.suffix in {".py", ".pyx", ".pxd", ".cc", ".h"})
  digest = hashlib.sha256()
  for path in sources:
    digest.update(str(path.relative_to(root)).encode() + b"\0" + path.read_bytes())
  import msgq.ipc_pyx
  binary = Path(msgq.ipc_pyx.__file__)
  return {"utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "platform": platform.platform(),
          "python": platform.python_version(), "cpu": cpu, "cpu_count": os.cpu_count(),
          "affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
          "versions": {name: version(name) for name in ("msgq-ipc", "pyzmq", "eclipse-zenoh", "lcm", "matplotlib")},
          "libzmq": zmq.zmq_version(), "msgq_binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
          "msgq_prealloc": os.environ.get("MSGQ_PREALLOC"), "cereal_fake": os.environ.get("CEREAL_FAKE"),
          "git_revision": git("rev-parse", "HEAD"), "git_dirty": git("status", "--porcelain"),
          "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), "msgq_source_sha256": digest.hexdigest()}


def save_report(report, path):
  if path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n")


def main():
  parser = argparse.ArgumentParser(description="Verified cross-process ping-pong; includes Python and payload validation overhead.")
  parser.add_argument("--iterations", type=positive_int, default=1000, help="minimum round trips per sample (default: %(default)s)")
  parser.add_argument("--seconds", type=positive_seconds, default=2, help="minimum seconds per sample (default: %(default)s)")
  parser.add_argument("--repeat", type=positive_int, default=5, help="samples per backend and size (default: %(default)s)")
  parser.add_argument("--timeout", type=positive_seconds, default=60, help="whole-trial timeout in seconds (default: %(default)s)")
  parser.add_argument("--seed", type=int, default=0, help="shuffle seed (default: %(default)s)")
  for flag in ("zmq", "pipe", "zenoh", "lcm"):
    parser.add_argument(f"--{flag}", action=argparse.BooleanOptionalAction, default=True, help=f"benchmark {flag}")
  parser.add_argument("--zenoh-tuned", action=argparse.BooleanOptionalAction, default=True,
                      help="tune Zenoh for latency; use normal transport for 64 KiB (default: %(default)s)")
  parser.add_argument("--plot", type=Path, help="save a 1 KiB chart")
  parser.add_argument("--json", type=Path, help="save raw samples and environment metadata")
  args = parser.parse_args()
  if "CEREAL_FAKE" in os.environ:
    parser.error("Unset CEREAL_FAKE to benchmark the real MSGQ backend")
  names = [name for name, enabled in zip(BACKENDS, (True, args.zmq, args.pipe, args.zenoh, args.lcm), strict=True) if enabled]
  report = {"environment": metadata(), "settings": {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()},
            "method": "Two spawned processes, one request/reply at a time, blocking receives, full payload/sequence verification in both peers. " +
                      "Delivered messages/sec = 2 * verified round trips / elapsed seconds; not maximum streaming throughput. " +
                      "One publisher/subscriber per direction; includes Python allocation and validation. CPU is diagnostic.",
            "transports": {name: BACKENDS[name] for name in names}, "samples": []}
  if args.zenoh:
    report["transports"]["zenoh IPC"] += (
      "; QoS off, low-latency for 64 B/1 KiB, standard transport for 64 KiB" if args.zenoh_tuned else "; default transport settings")
  print(report["method"], flush=True)
  for name, transport in report["transports"].items():
    print(f"{name}: {transport}", flush=True)
  randomizer = random.Random(args.seed)
  for repeat in range(args.repeat):
    jobs = [(name, size) for name in names for size in (64, 1024, 65536)]
    randomizer.shuffle(jobs)
    for name, size in jobs:
      try:
        sample = measure(name, size, args)
      except Exception as error:
        report["failure"] = {"backend": name, "bytes": size, "repeat": repeat, "error": str(error)}
        save_report(report, args.json)
        raise
      sample["repeat"] = repeat
      report["samples"].append(sample)
      save_report(report, args.json)
      print(f"{name:<12} {size:>6} bytes: {sample['round_trips'] / sample['seconds'] * 2:>12,.0f} delivered messages/sec", flush=True)
  results = []
  for name in names:
    for size in (64, 1024, 65536):
      rates = [2 * sample["round_trips"] / sample["seconds"] for sample in report["samples"] if sample["backend"] == name and sample["bytes"] == size]
      results.append((name, size, statistics.median(rates)))
  report["medians"] = [{"backend": name, "bytes": size, "messages_per_second": rate} for name, size, rate in results]
  save_report(report, args.json)
  if args.plot:
    plot_results(results, args.plot)


def plot_results(results, path):
  from matplotlib.ticker import EngFormatter, MaxNLocator

  labels = {"msgq": "msgq", "pyzmq IPC": "pyzmq", "Pipe bytes": "multiprocessing.Pipe", "zenoh IPC": "zenoh", "LCM UDP": "LCM"}
  rows = sorted(((name, rate) for name, size, rate in results if size == 1024), key=lambda row: row[1])
  maximum = max(rate for _, rate in rows)
  formatter = EngFormatter(sep="", places=2)
  with plt.rc_context({"font.family": "sans-serif", "font.sans-serif": ["Helvetica Neue", "Arial", "DejaVu Sans"]}):
    fig, ax = plt.subplots(figsize=(10, max(2.2, len(rows) * 0.48 + 0.7)))
    fig.set_facecolor("white")
    ax.set(xlim=(0, maximum * 1.2), ylim=(len(rows) - 0.5, -0.5), yticks=[])
    # Keep grid lines out of the values on the right.
    ticks = MaxNLocator(nbins=4).tick_values(0, maximum)
    ax.set_xticks([tick for tick in ticks if 0 <= tick < maximum])
    ax.xaxis.set_major_formatter(EngFormatter(sep=""))
    ax.set_xlabel("Delivered messages/sec — higher is better", fontsize=14, labelpad=8, color="#172b3a")
    ax.tick_params(length=0, pad=6, labelsize=11, colors="#425b6c")
    ax.set_axisbelow(True)
    ax.grid(axis="x", color="#b7b7b7", linewidth=1.5)
    for side, spine in ax.spines.items():
      spine.set(visible=side == "left", color="#192a32", linewidth=2)
    for index, (name, rate) in enumerate(rows):
      gray = str(0.88 - 0.28 * index / max(len(rows) - 2, 1))
      ax.barh(index, rate, height=1, color="#31cf43" if name == "msgq" else gray, zorder=2)
      ax.text(maximum * 0.029, index, labels[name], va="center", fontsize=16, fontweight="bold", color="#172b3a")
      ax.text(maximum * 1.173, index, formatter(rate), ha="right", va="center", fontsize=16, fontweight="bold", color="#172b3a")
    fig.subplots_adjust(left=0.055, right=0.98, top=0.98, bottom=0.21)
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=180, facecolor=fig.get_facecolor())
    plt.close(fig)
  print(f"Chart saved to {path}")


if __name__ == "__main__":
  main()
