import time
import threading
from datetime import datetime
from flask_socketio import emit

class CountdownTimer:
    """Manages a countdown timer for a heat"""
    
    def __init__(self, heat_id, duration=300):
        self.heat_id = heat_id
        self.duration = duration  # Default 5 minutes (300 seconds)
        self.remaining = duration
        self.status = 'ready'  # ready, running, paused, completed
        self.start_time = None
        self.pause_time = None
        self.thread = None
        self.lock = threading.Lock()
    
    def start(self):
        """Start or resume the countdown"""
        with self.lock:
            if self.status == 'ready':
                # Start fresh countdown
                self.start_time = time.time()
                self.status = 'running'
            elif self.status == 'paused':
                # Resume from paused state
                self.start_time = time.time() - (self.duration - self.remaining)
                self.status = 'running'
            
            # Start the countdown thread if not already running
            if not self.thread or not self.thread.is_alive():
                self.thread = threading.Thread(target=self._run_countdown)
                self.thread.daemon = True
                self.thread.start()
    
    def pause(self):
        """Pause the countdown"""
        with self.lock:
            if self.status == 'running':
                self.status = 'paused'
                self.pause_time = time.time()
                self.remaining = max(0, self.duration - int(self.pause_time - self.start_time))
    
    def reset(self, duration=None):
        """Reset the countdown"""
        with self.lock:
            if duration is not None:
                self.duration = duration
            self.remaining = self.duration
            self.status = 'ready'
            self.start_time = None
            self.pause_time = None
    
    def get_status(self):
        """Get the current countdown status"""
        with self.lock:
            if self.status == 'running':
                # Calculate remaining time
                elapsed = time.time() - self.start_time
                self.remaining = max(0, self.duration - int(elapsed))
                
                # Check if countdown has completed
                if self.remaining <= 0:
                    self.status = 'completed'
                    self.remaining = 0
            
            return {
                'heat_id': self.heat_id,
                'status': self.status,
                'remaining': self.remaining,
                'duration': self.duration,
                'formatted_time': self._format_time(self.remaining)
            }
    
    def _run_countdown(self):
        """Run the countdown in a separate thread"""
        while True:
            with self.lock:
                if self.status != 'running':
                    break
                
                # Calculate remaining time
                elapsed = time.time() - self.start_time
                self.remaining = max(0, self.duration - int(elapsed))
                
                # Check if countdown has completed
                if self.remaining <= 0:
                    self.status = 'completed'
                    self.remaining = 0
                    break
            
            # Emit countdown update via WebSocket
            self._emit_update()
            
            # Sleep for a short time to avoid high CPU usage
            time.sleep(0.5)
    
    def _emit_update(self):
        """Emit countdown update via WebSocket"""
        from app import socketio
        
        status_data = self.get_status()
        socketio.emit('countdown_update', status_data, namespace='/')
    
    def _format_time(self, seconds):
        """Format seconds as MM:SS"""
        minutes = seconds // 60
        seconds = seconds % 60
        return f"{minutes}:{seconds:02d}"


class CountdownManager:
    """Manages countdown timers for all heats"""
    
    def __init__(self):
        self.timers = {}
        self.lock = threading.Lock()
    
    def get_timer(self, heat_id):
        """Get or create a countdown timer for a heat"""
        with self.lock:
            if heat_id not in self.timers:
                self.timers[heat_id] = CountdownTimer(heat_id)
            return self.timers[heat_id]
    
    def start_timer(self, heat_id):
        """Start or resume a countdown timer"""
        timer = self.get_timer(heat_id)
        timer.start()
        return timer.get_status()
    
    def pause_timer(self, heat_id):
        """Pause a countdown timer"""
        timer = self.get_timer(heat_id)
        timer.pause()
        return timer.get_status()
    
    def reset_timer(self, heat_id, duration=None):
        """Reset a countdown timer"""
        timer = self.get_timer(heat_id)
        timer.reset(duration)
        return timer.get_status()
    
    def get_timer_status(self, heat_id):
        """Get the status of a countdown timer"""
        timer = self.get_timer(heat_id)
        return timer.get_status()


# Create a singleton instance
countdown_manager = CountdownManager()
