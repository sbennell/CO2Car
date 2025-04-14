import os
from app import create_app, socketio
import logging

# Set up logging for Flask-SocketIO
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('flask_socketio')
logger.setLevel(logging.DEBUG)
logger.info('Initializing Flask-SocketIO application')

# Create Flask app
app = create_app()

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    debug = os.environ.get('FLASK_ENV') == 'development'
    
    # Configure SocketIO with threading mode
    socketio.init_app(app, 
        cors_allowed_origins="*",
        logger=True,
        engineio_logger=True,
        async_mode='threading',  # Use threading mode
        ping_timeout=10,
        ping_interval=5,
        max_http_buffer_size=1e8,
        allow_upgrades=True
    )
    
    # Run the app with SocketIO
    logger.info(f"Starting server on port {port} with debug={debug}")
    try:
        socketio.run(app, 
            host='0.0.0.0', 
            port=port, 
            debug=debug,
            use_reloader=debug,
            log_output=True
        )
    except Exception as e:
        logger.error(f"Error starting server: {e}")
        raise 