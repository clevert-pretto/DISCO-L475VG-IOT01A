import pytest
import time
import re

# --- Basic Connectivity ---

def test_mcu_ping(device):
    """Scenario: Verify basic Request/Response loop."""
    response = device.write(b"\r\n")  # Clear any partial input
    #assert response.strip() == b">", f"Expected prompt, got: {response}"
    response = device.write(b"ping\r\n") #Extra new line and then the actual command to ensure clean parsing on MCU side
    assert b"pong" in response

def test_invalid_command(device):
    """Scenario: Verify error handling for unknown inputs."""
    response = device.write(b"garbage_cmd\r\n")
    assert b"Invalid command" in response

# --- System Information ---

def test_help_menu(device):
    """Scenario: Verify help menu displays all available commands."""
    response = device.write(b"help\r\n")
    assert b"--- Command Help Menu ---" in response
    # Check for specific command entries
    assert b"get_temp:" in response
    assert b"stack_health:" in response
    assert b"bulk_erase:" in response

def test_stack_health(device):
    """Scenario: Verify RTOS stack monitoring table."""
    response = device.write(b"stack_health\r\n")
    assert b"Task Stack High Water Marks (Words Remaining)" in response
    # Verify specific task names exist in the table
    assert b"Heartbeat" in response
    assert b"SensorRead" in response
    assert b"Logger" in response
    assert b"Command" in response
    assert b"SysManager" in response
    assert b"words" in response

# --- Sensor Data Scenarios ---

def test_get_temperature(device):
    """Scenario: Verify temperature reading and numeric format."""
    response = device.write(b"get_temp\r\n")
    assert b"Current Temperature:" in response
    # Regex to find a decimal number like 24.50
    assert re.search(rb"\d", response), f"No numeric value found in: {response}"

def test_get_humidity(device):
    """Scenario: Verify humidity reading and numeric format."""
    response = device.write(b"get_humidity\r\n")
    assert b"Current Humidity:" in response
    assert b"%" in response
    assert re.search(rb"\d", response), f"No numeric value found in: {response}"

# --- Logging Control Toggles ---

@pytest.mark.parametrize("cmd, expected_msg", [
    (b"enable_temp_log", b"Command: Enabled Temperature Logging"),
    (b"disable_temp_log", b"Command: Disabled Temperature Logging"),
    (b"enable_humidity_log", b"Command: Enabled Humidity Logging"),
    (b"disable_humidity_log", b"Command: Disabled Humidity Logging"),
])
def test_logging_toggles(device, cmd, expected_msg):
    """Scenario: Verify the 4 different logging control commands."""
    response = device.write(cmd + b"\r\n")
    assert expected_msg in response

# --- Storage Operations (Dangerous Operations) ---

def test_event_sector_erase(device):
    """
    Scenario: Comprehensive Sector Erase Test
    1. Enable logging and generate data.
    2. Verify initial Signature and Erase Count.
    3. Perform Sector Erase and verify completion.
    4. Verify logs are cleared and Erase Count incremented.
    """
    # 1. Prepare Data: Enable logging and wait for entries
    device.write(b"enable_temp_log\r\n")
    device.write(b"enable_humidity_log\r\n")
    print("Waiting 3 seconds for sensor data to be logged...")
    time.sleep(3) 

    # 2. Check initial state via dump_logs
    pre_log = device.write(b"dump_logs\r\n")
    assert b"Event log:" in pre_log, "Failed to generate initial log data"
    
    # Extract initial Erase Count (Signature 287454020 = 0x11223344)
    assert b"Signature: 287454020" in pre_log
    match = re.search(rb"Erase Count: (\d+)", pre_log)
    initial_count = int(match.group(1)) if match else 0

    # 3. Perform Sector Erase (Timeout 10s)
    response = device.write(b"event_sector_erase\r\n", timeout=20)
    assert b"Erase Complete" in response

    # 4. Final Verification
    post_log = device.write(b"dump_logs\r\n")
    
    # Verify Metadata
    assert b"Signature: 287454020" in post_log
    new_count_match = re.search(rb"Erase Count: (\d+)", post_log)
    new_count = int(new_count_match.group(1)) if new_count_match else -1
    assert new_count == initial_count + 1, f"Count didn't increment (Got {new_count})"
    
    # Verify logs are physically cleared (TS: timestamp entries should be gone)
    assert b"TS:" not in post_log, "Logs were not cleared after sector erase"


def test_bulk_erase(device):
    """
    Scenario: Comprehensive Bulk Erase Test
    1. Enable logging and generate data.
    2. Perform Bulk Erase and verify completion.
    3. Verify Signature is valid, Erase Count resets to 1, and logs are empty.
    """
    # 1. Prepare Data: Ensure logs exist
    device.write(b"enable_temp_log\r\n")
    device.write(b"enable_humidity_log\r\n")
    print("Waiting 3 seconds for sensor data to be logged...")
    time.sleep(3)

    # 2. Perform Bulk Erase (Timeout 40s)
    response = device.write(b"bulk_erase\r\n", timeout=50)
    assert b"Bulk Erase Complete" in response

    # 3. Final Verification
    post_log = device.write(b"dump_logs\r\n")
    
    # Verify Signature and Erase Count reset to 1
    assert b"Signature: 287454020" in post_log
    assert b"Erase Count: 1" in post_log, "Bulk erase should reset count to 1"
    
    # Verify no log entries remain
    assert b"TS:" not in post_log, "Flash not empty after bulk erase"
