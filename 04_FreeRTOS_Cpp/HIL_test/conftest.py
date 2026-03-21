import pytest
import serial
import time
import os

# HIL_test/conftest.py

def is_hardware_available(port='/dev/ttyACM0'):
    """Checks if the STM32 is physically plugged in."""
    return os.path.exists(port)

class HILDevice:
    def __init__(self, ser):
        """
        Initialize a HILDevice object with a serial port object.

        :param ser: Serial port object (e.g. pyserial.Serial)
        """
        self.ser = ser

    def write(self, command_bytes, timeout=None):
        """
        Sends a command and waits for a response.
        :param timeout: Optional override for long-running operations.
        """
        # Store original timeout to restore it later
        original_timeout = self.ser.timeout
        if timeout:
            self.ser.timeout = timeout

        self.ser.flushInput()
        print(f"\nSending command: {command_bytes.decode().strip()} (Timeout: {self.ser.timeout}s)")
        
        self.ser.write(command_bytes)
        
        # Capture the echo, the response, and the prompt together
        full_buffer = self.ser.read_until(b">")
        
        # Restore the original session timeout
        self.ser.timeout = original_timeout
        
        response = full_buffer.replace(command_bytes.strip(), b"")
        time.sleep(0.05)
        self.ser.read_all() 
        
        print(f"Received response: {response.decode().strip()}")
        return response

@pytest.fixture(scope="session")
def device():
    port = '/dev/ttyACM0'
    
    #check if we should skip HIL (manual override or hardware missing)
    if os.getenv("SKIP_HIL_TESTS") == "1" or not is_hardware_available(port):
        pytest.skip("Skipping HIL tests due to SKIP_HIL_TESTS or hardware not available at port {port}.")
    
    try:
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(2)  # Wait for boot
        ser.flushInput()
        hil = HILDevice(ser)
        yield hil
        ser.close()
    except serial.SerialException as e:
        pytest.skip(f"SerialException: {e}. Skipping HIL tests. Could not connect to {port}.")