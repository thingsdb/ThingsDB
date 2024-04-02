from socketIO_client import SocketIO, LoggingNamespace
import logging


logging.getLogger('socketIO-client').setLevel(logging.DEBUG)
logging.basicConfig()


socketIO = SocketIO('https://localhost', 7681, LoggingNamespace, verify=False)

