using Cxx = import "./include/c++.capnp";
$Cxx.namespace("cereal");

@0xb526ba661d550a59;

# custom.capnp: a home for empty structs reserved for custom forks
# These structs are guaranteed to remain reserved and empty in mainline
# openpilot, so use these if you want custom events in your fork.
# This way we will always be able to read your custom fork's logs,
# and your fork will always remain backwards compatible with logs
# from your own fork when you rebase.


struct CustomReserved0 {
}

struct CustomReserved1 {
}

struct CustomReserved2 {
}

struct CustomReserved3 {
}

struct CustomReserved4 {
}

