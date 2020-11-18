#include <iostream>
#include <chrono>
#include <cassert>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ipc.h"
#include "visionipc_server.h"


VisionIpcServer::VisionIpcServer(std::string name, std::vector<VisionStreamType> types, size_t num_buffers, bool opencl) : name(name) {
  // Create server mutex
  // Create map + alloc requested buffers

  // Start listener thread
  should_exit = false;
  listener_thread = std::thread(&VisionIpcServer::listener, this);

  // Create msgq publisher for each of the `name` + type combos
}

void VisionIpcServer::listener(){
  std::cout << "Starting listener for: " << name << std::endl;

  std::string path = "/tmp/visionipc_" + name;
  int sock = ipc_bind(path.c_str());
  assert(sock >= 0);

  while (!should_exit){
    // Wait for incoming connection
    struct pollfd polls[1] = {{0}};
    polls[0].fd = sock;
    polls[0].events = POLLIN;

    int ret = poll(polls, 1, 100);
    if (ret < 0) {
      if (errno == EINTR || errno == EAGAIN) continue;
      std::cout << "poll failed, stopping listener" << std::endl;
      break;
    }

    if (should_exit) break;
    if (!polls[0].revents) {
      continue;
    }

    // Handle incoming request
    int fd = accept(sock, NULL, NULL);
    assert(fd >= 0);

    std::cout << "Connection accepted" << std::endl;

    // Get stream type from client
    // Send back fds belonging to type

    close(fd);
  }

  std::cout << "Stopping listener for: " << name << std::endl;
  close(sock);
}



VisionBuf * VisionIpcServer::get_buffer(VisionStreamType type){
  // Lock mutex
  // Get next available buffer idx/buffer
  return nullptr;
}

void VisionIpcServer::send(VisionBuf * buf){
  // Sync buffer
  // Send over correct msgq socket
}

VisionIpcServer::~VisionIpcServer(){
  should_exit = true;
  listener_thread.join();
}
