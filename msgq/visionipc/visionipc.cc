#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <afunix.h>
#include <windows.h>
static struct WsaInit { WsaInit() { WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); } } wsa_init;
#else
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#if defined(__APPLE__) || defined(_WIN32)
#define getsocket() socket(AF_UNIX, SOCK_STREAM, 0)
#else
#define getsocket() socket(AF_UNIX, SOCK_SEQPACKET, 0)
#endif

#include "msgq/visionipc/visionipc.h"

int ipc_connect(const char* socket_path) {
  int err;

  int sock = getsocket();

  if (sock < 0) return -1;
  struct sockaddr_un addr = {
    .sun_family = AF_UNIX,
  };
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);
  err = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if (err != 0) {
    ipc_close(sock);
    return -1;
  }

  return sock;
}

int ipc_bind(const char* socket_path) {
  int err;

  unlink(socket_path);

  int sock = getsocket();

  struct sockaddr_un addr = {
    .sun_family = AF_UNIX,
  };
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);
  err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  assert(err == 0);

  err = listen(sock, 3);
  assert(err == 0);

  return sock;
}

void ipc_close(int fd) {
#ifdef _WIN32
  closesocket(fd);
#else
  close(fd);
#endif
}


#ifdef _WIN32
// No SCM_RIGHTS on Windows: the sender duplicates the buffers' section handles into the peer process, whose pid
// the socket reports, and sends the handle values after the length-prefixed message (a stream has no record boundaries)
int ipc_sendrecv_with_fds(bool send, int fd, void *buf, size_t buf_size, int* fds, int num_fds,
                          int *out_num_fds) {
  uint32_t len = buf_size, n = num_fds;
  if (send) {
    if (::send(fd, (char*)&len, sizeof(len), 0) < 0 || ::send(fd, (char*)buf, len, 0) < 0 || ::send(fd, (char*)&n, sizeof(n), 0) < 0) return -1;
    if (n > 0) {
      DWORD peer_pid = 0, bytes = 0;
      if (WSAIoctl(fd, SIO_AF_UNIX_GETPEERPID, NULL, 0, &peer_pid, sizeof(peer_pid), &bytes, NULL, NULL) != 0) return -1;
      HANDLE peer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, peer_pid);
      if (peer == NULL) return -1;
      for (uint32_t i = 0; i < n; i++) {
        HANDLE dup = NULL;
        BOOL ok = DuplicateHandle(GetCurrentProcess(), (HANDLE)(intptr_t)fds[i], peer, &dup, 0, FALSE, DUPLICATE_SAME_ACCESS);
        uint32_t handle = (uint32_t)(uintptr_t)dup;  // kernel handles fit in 32 bits
        if (!ok || ::send(fd, (char*)&handle, sizeof(handle), 0) < 0) {
          CloseHandle(peer);
          return -1;
        }
      }
      CloseHandle(peer);
    }
    return len;
  }

  if (recv(fd, (char*)&len, sizeof(len), MSG_WAITALL) != sizeof(len) || len > buf_size ||
      recv(fd, (char*)buf, len, MSG_WAITALL) != (int)len || recv(fd, (char*)&n, sizeof(n), MSG_WAITALL) != sizeof(n)) {
    errno = ECONNRESET;
    return -1;
  }
  assert(n == 0 || (fds && (int)n <= num_fds));
  for (uint32_t i = 0; i < n; i++) {
    uint32_t handle = 0;
    if (recv(fd, (char*)&handle, sizeof(handle), MSG_WAITALL) != sizeof(handle)) {
      errno = ECONNRESET;
      return -1;
    }
    fds[i] = handle;
  }
  if (fds) {
    assert(out_num_fds);
    *out_num_fds = n;
  }
  return len;
}
#else
int ipc_sendrecv_with_fds(bool send, int fd, void *buf, size_t buf_size, int* fds, int num_fds,
                          int *out_num_fds) {
  char control_buf[CMSG_SPACE(sizeof(int) * num_fds)];
  memset(control_buf, 0, CMSG_SPACE(sizeof(int) * num_fds));

  struct iovec iov = {
    .iov_base = buf,
    .iov_len = buf_size,
  };
  struct msghdr msg = {
    .msg_iov = &iov,
    .msg_iovlen = 1,
  };

  if (num_fds > 0) {
    assert(fds);

    msg.msg_control = control_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int) * num_fds);
  }

  if (send) {
    if (num_fds) {
      struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
      assert(cmsg);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(int) * num_fds);
      memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * num_fds);
    }
    return sendmsg(fd, &msg, 0);
  } else {
    int r = recvmsg(fd, &msg, 0);
    if (r < 0) return r;

    int recv_fds = 0;
    if (msg.msg_controllen > 0) {
      struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
      assert(cmsg);
      assert(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS);
      recv_fds = (cmsg->cmsg_len - CMSG_LEN(0));
      assert(recv_fds > 0 && (recv_fds % sizeof(int)) == 0);
      recv_fds /= sizeof(int);

      assert(fds && recv_fds <= num_fds);
      memcpy(fds, CMSG_DATA(cmsg), sizeof(int) * recv_fds);
    }

    if (msg.msg_flags) {
      for (int i=0; i<recv_fds; i++) {
        close(fds[i]);
      }
      return -1;
    }

    if (fds) {
      assert(out_num_fds);
      *out_num_fds = recv_fds;
    }
    return r;
  }
}
#endif
