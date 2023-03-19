# Signal filter.

import logging

logger = logging.getLogger(__name__)


class Filter:

    def __init__(self, k: float):
        self.__value = 0
        self.__k1 = k
        self.__k2 = (1 - k)
        self.__empty = True

    def value(self) -> float:
        return self.__value

    def add(self, new_value: float):
        if self.__empty:
            self.__value = new_value
            self.__empty = False
        else:
            self.__value = (new_value * self.__k1) + (self.__value * self.__k2)
