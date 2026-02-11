"""TypeDoc model."""

import logging
from typing import List, Dict

from modules.typedocjson import *
import copy

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Model:
    """
    Store parsed JSON data hierarchical. Add additional relational information.
    """

    INVALID_OBJECT_ID = -1

    def __init__(self, data_namespace: str) -> None:
        """ Construction """
        self.objects: Dict[int, JSONObject] = dict()  # data object ID -> object from JSON file
        self.parents: Dict[int, List[int]] = dict()  # object ID -> list of parent IDs in hierarchical order
        self.namespace: str = data_namespace  # meta info: data originating file ("namespace")
        self.root_object: JSONObject = None  # root object from imported json file ("project")

    def get_namespace(self) -> str:
        return self.namespace

    def count_objects(self) -> int:
        """
        Count objects total.
        """
        return len(self.objects)

    def get_object(self, object_id: int) -> JSONObject:
        """
        Return JSON object by ID.
        :param object_id  object ID from JSON file
        """
        return self.objects.get(object_id, None)

    def add_object(self, obj_id: int, obj: JSONObject) -> None:
        """
        Store object from JSON file by its ID.
        :param obj_id  JSON object ID, from 'id' attribute
        :param obj JSON object from JSON file
        """
        if obj_id in self.objects.keys():
            logging.warning(f"found redundant object id {obj_id}")

        self.objects[obj_id] = obj

    def get_object_children(self, obj: JSONObject) -> List[JSONObject]:
        """
        Get all children based on 'obj' list of children.
        Note that not all TypeDoc JSON objects have their
        children referenced as a list of IDs.
        """

        # get all children objects to be able to sort them by name
        children_ids = obj.get(TDAttribute.CHILDREN)
        children_objects = []
        for child_id in children_ids:
            children_objects.append(self.get_object(child_id))

        return children_objects

    def find_property_id(self, class_name: str, attribute: str) -> int:
        """
        Lookup class property ID.
        Restricted to classes, does not recursively search in extended classes.

        :param class_name  name of class object to lookup
        :param attribute  name of attribute
        :returns ID of property or INVALID_OBJECT_ID for no result
        """

        for obj in self.objects.values():
            if TDReflectionKind(obj.get(TDAttribute.REFLECTION_KIND)) is not TDReflectionKind.CLASS:
                continue
            if obj.get(TDAttribute.NAME) != class_name:
                continue

            # class found, check children for matching property
            children_objects = obj.get(TDAttribute.CHILDREN)
            if not children_objects:
                return Model.INVALID_OBJECT_ID

            for c in obj.get(TDAttribute.CHILDREN):
                if c.get(TDAttribute.REFLECTION_KIND) != TDReflectionKind.PROPERTY:
                    continue
                if c.get(TDAttribute.NAME) != attribute:
                    continue
                # match
                return c.get(TDAttribute.ID)

        # no result
        return Model.INVALID_OBJECT_ID

    def get_root_anchor(self) -> str:
        """ Get anchor for the index file. """
        return self.get_namespace() + "-home"

    @staticmethod
    def is_exported_to_separate_file(kind_id: TDReflectionKind) -> bool:
        """ Check for object kind that should be exported to an individual output file. """

        return kind_id in [
            TDReflectionKind.CLASS,
            TDReflectionKind.INTERFACE,
            TDReflectionKind.NAMESPACE,
            TDReflectionKind.ENUM,
            TDReflectionKind.VARIABLE,
            TDReflectionKind.FUNCTION,
            TDReflectionKind.TYPE_ALIAS
        ]

    def get_object_parent_ids(self, object_id: int) -> List[int]:
        return self.parents.get(object_id, list())

    def build_object_parents_dict(self):
        resolver = ObjectParentsResolver(model=self)
        self.parents = resolver.build()


class ObjectParentsResolver:
    """ Parse objects recursively, store a list of parents per object. Include the object in the list. """

    def __init__(self, model: Model) -> None:
        self.model: Model = model
        self.data: Dict[int, List[int]] = dict()  # object id -> list of parents id

    def _recurse(self, obj: JSONObject, id_list: List[int]) -> None:
        """ Reminder: when branching, use deep copies of the lists as the data dictionary stores pointers. """

        group_objects = obj.get(TDAttribute.GROUPS)
        if group_objects is None:
            return

        object_id = obj.get(TDAttribute.ID)

        # Design choice, include the object in the list.
        id_list.append(object_id)
        self.data[object_id] = copy.deepcopy(id_list)

        # Advance recursion, again: use list copies.
        for group_object in group_objects:
            children_objects = self.model.get_object_children(group_object)
            for child in children_objects:
                child_list = copy.deepcopy(id_list)
                self._recurse(child, child_list)

    def build(self):
        """ Start recursion at top level groups. Use a new list for each child. """

        self.data.clear()

        for group_object in self.model.root_object.get(TDAttribute.GROUPS):
            children_objects = self.model.get_object_children(group_object)
            for child in children_objects:
                self._recurse(child, list())

        return self.data
