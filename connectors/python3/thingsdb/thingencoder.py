import json
from .thing import Thing


class ThingEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Thing):
            thing_dict = {
                '#': obj._id,
            }
            thing_dict.update(obj._props)
            return thing_dict
        return super().default(obj)
