# tests/integration_test.py

import socket
import unittest
import subprocess
import time

# Helper function to create RESP commands
def to_resp_command(command_array):
    resp = f"*{len(command_array)}\r\n"
    for arg in command_array:
        resp += f"${len(arg)}\r\n{arg}\r\n"
    return resp.encode('utf-8')

class TestKacheServer(unittest.TestCase):
    server_process = None

    @classmethod
    def setUpClass(cls):
        # This method runs once before any tests start.
        # It builds and starts the C++ server as a background process.
        print("Starting Kache server...")
        # Assuming the executable is in ../build/
        cls.server_process = subprocess.Popen(["./build/kache_server"])
        time.sleep(1) # Give the server a moment to start up

    @classmethod
    def tearDownClass(cls):
        # This method runs once after all tests are finished.
        # It stops the C++ server.
        print("Stopping Kache server...")
        cls.server_process.terminate()
        cls.server_process.wait()

    def setUp(self):
        # This method runs before each individual test.
        # It creates a new connection for each test to ensure they are isolated.
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client.connect(('127.0.0.1', 6379))

    def tearDown(self):
        # This method runs after each individual test.
        self.client.close()

    def _send_command(self, command_list):
        resp_command = to_resp_command(command_list)
        self.client.sendall(resp_command)
        response = self.client.recv(1024).decode('utf-8').strip()
        return response

    def test_01_set_and_get(self):
        """Tests basic SET and GET commands."""
        response = self._send_command(["SET", "mykey", "myvalue"])
        self.assertEqual(response, "OK")

        response = self._send_command(["GET", "mykey"])
        self.assertEqual(response, "myvalue")

    def test_02_get_nonexistent(self):
        """Tests GET on a key that does not exist."""
        response = self._send_command(["GET", "nosuchkey"])
        self.assertEqual(response, "(nil)")

    def test_03_value_with_spaces(self):
        """Ensures the RESP parser correctly handles values with spaces."""
        value = "hello world from kache"
        response = self._send_command(["SET", "spacekey", value])
        self.assertEqual(response, "OK")

        response = self._send_command(["GET", "spacekey"])
        self.assertEqual(response, value)

    def test_04_delete(self):
        """Tests the DELETE command."""
        self._send_command(["SET", "todelete", "somevalue"])
        
        # This test will find your typo!
        # It expects '(integer) 1' but your code sends something else.
        response = self._send_command(["DELETE", "todelete"])
        self.assertEqual(response, "(integer) 1")

        response = self._send_command(["EXISTS", "todelete"])
        self.assertEqual(response, "(integer) 0")

if __name__ == '__main__':
    # This makes the script runnable.
    unittest.main()