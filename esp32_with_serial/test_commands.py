import serial
import json
import time

def wait_for_esp32(ser, timeout=5.0):
    """Wait for ESP32 to initialize and respond."""
    print("Waiting for ESP32 to initialize...")
    start_time = time.time()
    ready = False
    
    while (time.time() - start_time) < timeout:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"ESP32: {line}")
            if "System Ready" in line:
                ready = True
                # Continue reading for a short while to clear any remaining messages
                time.sleep(0.5)
                while ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    print(f"ESP32: {line}")
                print("ESP32 initialized successfully")
                return True
        time.sleep(0.1)
    
    print("ESP32 initialization timeout")
    return False

def send_command(ser, command, expected_type=None, max_retries=3, timeout=2.0):
    """Send a JSON command to the ESP32 and return the response."""
    start_time = time.time()
    
    for attempt in range(max_retries):
        # Clear any pending data
        ser.reset_input_buffer()
        
        # Convert command to JSON string and add newline
        cmd_str = json.dumps(command) + '\n'
        
        # Send command
        print(f"\nAttempt {attempt + 1}/{max_retries}")
        print(f"Sending: {cmd_str.strip()}")
        ser.write(cmd_str.encode())
        
        # Wait for response with timeout
        response_start = time.time()
        response_buffer = []
        
        while (time.time() - response_start) < timeout:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"Raw response: {line}")
                    
                    # Skip initialization messages
                    if line.startswith("---") or line.startswith("✔"):
                        continue
                    
                    response_buffer.append(line)
                    
                    try:
                        response_json = json.loads(line)
                        print(f"Parsed JSON: {json.dumps(response_json, indent=2)}")
                        
                        # Verify response type if expected
                        if expected_type and response_json.get('type') != expected_type:
                            print(f"Warning: Expected response type '{expected_type}' but got '{response_json.get('type')}'")
                            if attempt < max_retries - 1:
                                print("Retrying...")
                                break
                        
                        return response_json
                    except json.JSONDecodeError:
                        # Check if this is a valid text response
                        if line.startswith("🚦") or line.startswith("⏱"):
                            print(f"Valid text response: {line}")
                            return line
                        
                        print(f"Not valid JSON: {line}")
                        continue
            
            time.sleep(0.1)
        
        if response_buffer:
            print(f"Response buffer contents: {response_buffer}")
        print(f"Command timeout after {timeout} seconds")
    
    print("All retries exhausted")
    return None

def verify_race_state(ser):
    """Verify the current race state."""
    print("\n=== Verifying Race State ===")
    status = send_command(ser, {"cmd": "status"}, expected_type="status", timeout=3.0)
    
    if status is None:
        print("Error: Failed to get race state")
        return None
        
    if isinstance(status, dict):
        print("Current Race State:")
        print(f"  Race Started: {status.get('race_started', False)}")
        print(f"  Cars Loaded: {status.get('cars_loaded', False)}")
        print(f"  Car 1 Finished: {status.get('car1_finished', False)}")
        print(f"  Car 2 Finished: {status.get('car2_finished', False)}")
        print(f"  Car 1 Time: {status.get('car1_time', 0)}")
        print(f"  Car 2 Time: {status.get('car2_time', 0)}")
    return status

def main():
    # Configure serial port
    try:
        print(f"Opening COM5 at 115200 baud...")
        ser = serial.Serial(
            port='COM5',
            baudrate=115200,
            timeout=1,
            write_timeout=1
        )
        
        # Reset ESP32 by toggling DTR
        print("Resetting ESP32...")
        ser.dtr = False
        time.sleep(0.1)
        ser.dtr = True
        
        # Wait for ESP32 to initialize
        if not wait_for_esp32(ser, timeout=10.0):  # Increased timeout
            print("Failed to initialize ESP32")
            return
        
        # Additional delay after initialization
        print("Waiting for system to stabilize...")
        time.sleep(2)
        
        try:
            # Initial state check
            print("\n=== Initial State ===")
            initial_state = verify_race_state(ser)
            if initial_state is None:
                print("Failed to get initial state, exiting")
                return
            
            # Test load_cars command
            print("\n=== Testing load_cars command ===")
            response = send_command(ser, {"cmd": "load_cars"}, timeout=2.0)
            if response:
                time.sleep(1)
                verify_race_state(ser)
            
            # Test start_race command
            print("\n=== Testing start_race command ===")
            response = send_command(ser, {"cmd": "start_race"}, expected_type="status", timeout=2.0)
            if response:
                time.sleep(1)
                verify_race_state(ser)
            
            # Wait for race to complete or timeout
            print("\n=== Waiting for race completion ===")
            start_time = time.time()
            while time.time() - start_time < 10:  # 10 second timeout
                state = verify_race_state(ser)
                if isinstance(state, dict) and state.get('car1_finished', False) and state.get('car2_finished', False):
                    print("Race completed!")
                    break
                time.sleep(1)
            
            # Test reset_timer command
            print("\n=== Testing reset_timer command ===")
            response = send_command(ser, {"cmd": "reset_timer"}, expected_type="status", timeout=2.0)
            if response:
                time.sleep(1)
                verify_race_state(ser)
            
            # Final status check
            print("\n=== Final State ===")
            final_state = verify_race_state(ser)
            
        except KeyboardInterrupt:
            print("\nTest interrupted by user")
        except Exception as e:
            print(f"\nError during test: {str(e)}")
            import traceback
            traceback.print_exc()
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {str(e)}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main() 