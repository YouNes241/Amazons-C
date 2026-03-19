#include "network_test_fixture.h"

int cb_joined_count = 0;
int cb_disconnected_count = 0;
int cb_timeout_count = 0;
int cb_error_count = 0;
int cb_invite_count = 0;
int current_test_port = 9400;

extern "C" void
mock_on_player_joined (const char *name)
{
  (void)name;
  cb_joined_count++;
}
extern "C" void
mock_on_player_disconnected (const char *reason)
{
  (void)reason;
  cb_disconnected_count++;
}
extern "C" void
mock_on_timeout ()
{
  cb_timeout_count++;
}
extern "C" void
mock_on_error (const char *err)
{
  (void)err;
  cb_error_count++;
}
extern "C" void
mock_on_invite (int id)
{
  (void)id;
  cb_invite_count++;
}

void
NetworkManagerTest::SetUp ()
{
  cb_joined_count = 0;
  cb_disconnected_count = 0;
  cb_timeout_count = 0;
  cb_error_count = 0;
  cb_invite_count = 0;
  test_port = current_test_port++;

  test_callbacks.OnPlayerJoined = mock_on_player_joined;
  test_callbacks.OnPlayerDisconnected = mock_on_player_disconnected;
  test_callbacks.OnTimeoutReceived = mock_on_timeout;
  test_callbacks.OnGameError = mock_on_error;
  test_callbacks.OnInvitationReceived = mock_on_invite;

  NetworkManager_Init (&server_mgr, test_callbacks);
  NetworkManager_Init (&client_mgr, test_callbacks);
}

void
NetworkManagerTest::TearDown ()
{
  NetworkManager_Cleanup (&client_mgr);
  NetworkManager_Cleanup (&server_mgr);
}