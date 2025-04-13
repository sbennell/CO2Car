import os
import json
from datetime import datetime
import pytz

class ConfigManager:
    """Configuration manager for storing application settings"""
    
    def __init__(self, config_file='config.json'):
        """Initialize the configuration manager"""
        self.config_file = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), config_file)
        self.config = self._load_config()
    
    def _load_config(self):
        """Load configuration from file"""
        if os.path.exists(self.config_file):
            try:
                with open(self.config_file, 'r') as f:
                    return json.load(f)
            except (json.JSONDecodeError, IOError):
                return self._create_default_config()
        else:
            return self._create_default_config()
    
    def _create_default_config(self):
        """Create default configuration"""
        config = {
            'check_in': {
                'deadlines': {},
                'notifications': {}
            }
        }
        self._save_config(config)
        return config
    
    def _save_config(self, config=None):
        """Save configuration to file"""
        if config is None:
            config = self.config
        
        try:
            with open(self.config_file, 'w') as f:
                json.dump(config, f, indent=4)
        except IOError:
            print(f"Error: Unable to save configuration to {self.config_file}")
    
    def get_check_in_deadline(self, event_id):
        """Get check-in deadline for an event"""
        event_id = str(event_id)
        deadlines = self.config.get('check_in', {}).get('deadlines', {})
        
        if event_id in deadlines:
            deadline_str = deadlines[event_id]
            try:
                return datetime.fromisoformat(deadline_str)
            except ValueError:
                return None
        
        return None
    
    def set_check_in_deadline(self, event_id, deadline):
        """Set check-in deadline for an event"""
        event_id = str(event_id)
        
        if 'check_in' not in self.config:
            self.config['check_in'] = {}
        
        if 'deadlines' not in self.config['check_in']:
            self.config['check_in']['deadlines'] = {}
        
        if isinstance(deadline, datetime):
            deadline_str = deadline.isoformat()
        else:
            deadline_str = deadline
        
        self.config['check_in']['deadlines'][event_id] = deadline_str
        self._save_config()
    
    def get_notification_status(self, event_id):
        """Get notification status for an event"""
        event_id = str(event_id)
        notifications = self.config.get('check_in', {}).get('notifications', {})
        
        return notifications.get(event_id, False)
    
    def set_notification_status(self, event_id, status=True):
        """Set notification status for an event"""
        event_id = str(event_id)
        
        if 'check_in' not in self.config:
            self.config['check_in'] = {}
        
        if 'notifications' not in self.config['check_in']:
            self.config['check_in']['notifications'] = {}
        
        self.config['check_in']['notifications'][event_id] = status
        self._save_config()

# Create a singleton instance
config_manager = ConfigManager()
