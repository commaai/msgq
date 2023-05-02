#include <cstddef>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <signal.h>

#include "cereal/messaging/messaging.h"
#include "cereal/messaging/impl_fake.h"

#define MSGS 1e1

void daemon_repub_process_run() {
  printf("*** daemon child start. pid: %d\n", getpid());

  Context * c = Context::create();
  PubSocket * pub_sock = PubSocket::create(c, "ubloxGnss");
  sleep(1);
  SubSocket * sub_sock = SubSocket::create(c, "carState");

  char data[8];
  int status;
  uint64_t ii;

  while (true) {
    printf("*** Thread attempt to receive message\n");
    Message * msg = sub_sock->receive();
    ii = *(uint64_t*)msg->getData();
    delete msg;
    printf("*** Thread done to receive message: %lu\n", ii);

    *(uint64_t*)data = ii;
    int n_messages = ii % 3;
    for (int n = 0; n < n_messages; n++) {
      printf("*** Thread attempt to send message[s]: %lu\n", ii);
      status = pub_sock->send(data, 8);
      printf("*** Thread done to send message[s] %lu, status: %d, errno: %d\n", ii, status, errno);
    }
  }
}

int main() {
  FakeEvent::toggle_fake_events(true);
  FakeEvent * carState_recv_called = FakeEvent::create_and_register("carState", FakeEventPurpose::RECV_CALLED);
  FakeEvent * carState_recv_ready = FakeEvent::create_and_register("carState", FakeEventPurpose::RECV_READY);

  printf("carState fds: %d, %d\n", carState_recv_called->fd(), carState_recv_ready->fd());

  int pid = fork();
  if (pid == 0) {
    daemon_repub_process_run();
    return 0;
  }

  printf("+++ Parent pid %d\n", getpid());

  Context * c = Context::create();
  PubSocket * pub_sock = PubSocket::create(c, "carState");
  sleep(1);
  SubSocket * sub_sock = SubSocket::create(c, "ubloxGnss");

  char data[8];

  for (uint64_t i = 0; i < MSGS; i++) {
    printf("+++ Sending %lu\n", i);
    *(uint64_t*)data = i;
    int status = pub_sock->send(data, 8);
    printf("+++ DONE Sending %lu, status: %d, errno: %d\n", i, status, errno);

    printf("+++ Waiting for next recv cycle\n");
    carState_recv_called->wait();

    int counter = 0;
    while (true) {
      printf("+++ Start receiving %lu\n", i);
      uint64_t ii;
      Message * m = sub_sock->receive(true);
      if (m == nullptr) {
        printf("+++ Message is null. Probably the end of the cycle.\n");
        break;
      }

      ii = *(uint64_t*)m->getData();
      assert(i == ii);
      delete m;
      printf("+++ DONE Receive %lu\n", ii);
      counter++;
    }

    printf("+++ Received %d messages\n\n", counter);
  }

  printf("+++ Killing child %d\n", pid);
  kill(pid, SIGKILL);

  delete sub_sock;
  delete pub_sock;
  delete c;
  delete carState_recv_called;
  delete carState_recv_ready;

  return 0;
}
