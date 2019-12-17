from ..protocol import Proto
import abc
from typing import Any


class Events(abc.ABC):

    def __init__(self):
        self._evmap = {
            Proto.ON_NODE_STATUS: self.on_node_status,
            Proto.ON_WARN: self.on_warning,
            Proto.ON_WATCH_INI: self.on_watch_init,
            Proto.ON_WATCH_UPD: self.on_watch_update,
            Proto.ON_WATCH_DEL: self.on_watch_delete,
        }

    def __call__(self, tp: Proto, data: Any) -> None:
        self._evmap.get(tp)(data)

    @abc.abstractmethod
    def on_reconnect(self) -> None:
        """On re-connect
        Called after a re-concect is finished (including authentication)
        """
        pass

    @abc.abstractmethod
    def on_node_status(self, status: str) -> None:
        """On node status
        status: String containing a `new` node status.
                Optional values:
                    - OFFLINE
                    - CONNECTING
                    - BUILDING
                    - SHUTTING_DOWN
                    - SYNCHRONIZING
                    - AWAY
                    - AWAY_SOON
                    - READY
        """
        pass

    @abc.abstractmethod
    def on_warning(self, warn: dict) -> None:
        """On warning
        warn: a dictionary with `warn_msg` and `warn_code`. for example:

        {
            "warn_msg": "some warning message"
            "warn_code": 1
        }
        """
        pass

    @abc.abstractmethod
    def on_watch_init(self, data: dict) -> None:
        """On watch init.
        Initial data from a single thing. for example:

        {
            "#": 123,
            "name": "ThingsDB!",
            ...
        }
        """
        pass

    @abc.abstractmethod
    def on_watch_update(self, data: dict) -> None:
        """On watch update.
        Updates for a thing with ID (#). One event may contain more than one
        job. for example:

        {

        }
        """
        pass

    @abc.abstractmethod
    def on_watch_delete(self, data: dict) -> None:
        """On watch delete.
        The thing is removed from the collection (and garbage collected).
        for example:

        {

        }
        """
        pass
