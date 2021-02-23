import logging
from abc import ABC,abstractmethod
from typing import Any


class TiHandler(ABC):

    @abstractmethod
    async def on_error(self, e: Exception):
        logging.error(e)

    @abstractmethod
    async def on_config(self, data: Any):
        pass

    @abstractmethod
    async def on_request(self, data: dict):
        pass


