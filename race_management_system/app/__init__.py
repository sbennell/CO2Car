from flask import Flask
from flask_socketio import SocketIO
from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager
from flask_migrate import Migrate
import os
import threading
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Initialize extensions
db = SQLAlchemy()
socketio = SocketIO()
login_manager = LoginManager()
migrate = Migrate()
flask_app = None  # Global app instance for use in background threads

def create_app():
    global flask_app
    app = Flask(__name__)
    flask_app = app  # Store app globally
    
    # Configure the Flask application
    app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', 'dev-key-for-development')
    app.config['SQLALCHEMY_DATABASE_URI'] = os.environ.get('DATABASE_URI', 'sqlite:///race_management.db')
    app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
    
    # Initialize extensions with the app
    db.init_app(app)
    socketio.init_app(app, cors_allowed_origins="*")
    login_manager.init_app(app)
    login_manager.login_view = 'auth.login'
    migrate.init_app(app, db)
    
    # Register blueprints
    from app.routes.main import main_bp
    from app.routes.api import api_bp
    from app.routes.auth import auth_bp
    from app.routes.export import export_bp
    from app.routes.check_in import check_in_bp
    from app.routes.countdown import countdown_bp
    from app.routes.hardware import hardware_bp
    
    app.register_blueprint(main_bp)
    app.register_blueprint(api_bp, url_prefix='/api')
    app.register_blueprint(auth_bp, url_prefix='/auth')
    app.register_blueprint(export_bp, url_prefix='/export')
    app.register_blueprint(check_in_bp)
    app.register_blueprint(countdown_bp)
    app.register_blueprint(hardware_bp)
    
    # Import models and register user loader
    from app.models.user import User
    
    @login_manager.user_loader
    def load_user(id):
        return User.query.get(int(id))
    
    # Create database tables
    with app.app_context():
        db.create_all()
    
    # Start the scheduler in a background thread if not in debug mode
    if not app.debug:
        def start_scheduler():
            from app.tasks.scheduler import run_scheduler
            run_scheduler()
        
        scheduler_thread = threading.Thread(target=start_scheduler)
        scheduler_thread.daemon = True
        scheduler_thread.start()
    
    # Initialize serial manager for ESP32 communication
    from app.utils.serial_manager import init_serial_manager
    init_serial_manager(socketio)
    
    return app 