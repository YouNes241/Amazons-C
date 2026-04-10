#ifndef NETWORK_TEST_FIXTURE_H
#define NETWORK_TEST_FIXTURE_H

#include <chrono>
#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_includes/network_handlers.h"
#include "../../src/network/network_includes/network_internal.h"
#include "../../src/network/network_includes/network_packet.h"
#include "game_engine.h"
#include "network_protocol.h"
}

extern "C" void mock_on_player_joined (const char *name);
extern "C" void mock_on_player_disconnected (const char *reason);
extern "C" void mock_on_timeout (void);
extern "C" void mock_on_error (const char *err);
extern "C" void mock_on_invite (int id);

extern int cb_joined_count;
extern int cb_disconnected_count;
extern int cb_timeout_count;
extern int cb_error_count;
extern int cb_invite_count;
extern int current_test_port;

class NetworkManagerTest : public ::testing::Test
{
protected:
  NetworkManager server_mgr;
  NetworkManager client_mgr;
  NetworkEvents test_callbacks = {};
  int test_port;

  void SetUp () override;
  void TearDown () override;
};

#endif // NETWORK_TEST_FIXTURE_H