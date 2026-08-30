"""generator core."""

from config_plugin_gen.models import *
from loguru import logger
import jinja2
import os
import pathlib
import typing
import errno, sys

from typing import (
    Any,
    Union,
    Dict,
    List,
)


class Generator:
    """Main class of the generator."""

    def __init__(self) -> None:
        template_loader = jinja2.PackageLoader(
            package_name="config_plugin_gen",
            package_path="templates",
        )
        self.__template_env = jinja2.Environment(
            loader=template_loader,
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True,
        )

    def generate(
        self,
        config_model: ConfigModel,
        output_dir: pathlib.Path,
    ) -> None:
        """Generate config_manager plugin C interface."""

        self.config_model = config_model

        """ Provide variables to jinja2 templates. """
        self.__template_env.globals["model"] = config_model
        self.__template_env.globals[
            "generated"
        ] = "This file is generated automatically, edit with care."

        """ Provide functions to jinja2 templates. """
        self.__template_env.globals["raise_error"] = self.__raise_error
        self.__template_env.globals["is_builtin"] = self.__is_builtin
        self.__template_env.globals["is_numeric"] = self.__is_numeric
        self.__template_env.globals["is_numeric_unsigned"] = self.__is_numeric_unsigned
        self.__template_env.globals["get_base_type"] = self.__get_base_type
        self.__template_env.globals["get_full_type"] = self.__get_full_type
        self.__template_env.globals["find_type"] = self.__find_type
        self.__template_env.globals["get_itemized_size"] = self.__get_itemized_size
        self.__template_env.globals["get_itemized_size_name"] = self.__get_itemized_size_name
        self.__template_env.globals["is_itemized_indexes_enum"] = self.__is_itemized_indexes_enum
        self.__template_env.globals["get_getter_func"] = self.__get_getter_func
        self.__template_env.globals["get_range"] = self.__get_range
        self.__template_env.globals["has_checker_func"] = self.__has_checker_func
        self.__template_env.globals["get_checker_func"] = self.__get_checker_func
        self.__template_env.globals["get_nb_keys"] = self.__get_nb_keys
        self.__template_env.globals["get_nb_root_key_sections"] = self.__get_nb_root_key_sections

        self.plugin_name = "l1_config"
        generated_files_src = [
            "_keys.c",
            "_keys.h",
        ]
        generated_files_include = [
            "_types.h",
            "_getter.h",
        ]
        generated_files_dict: Dict[str, List[str]] = {
            "src": generated_files_src,
            "include": generated_files_include,
        }

        for sub_dir, files in generated_files_dict.items():
            full_output_sub_dir = str(output_dir) + "/" + sub_dir

            # Create the output directory if it does not exist
            if not os.path.exists(full_output_sub_dir):
                os.makedirs(full_output_sub_dir)

            # Generate all files for that output directory sub
            for gen_file_suffix in files:
                gen_file = self.plugin_name + gen_file_suffix
                template_file = gen_file + ".j2"
                template = self.__template_env.get_template(template_file)
                rendered = template.render()
                full_filename = full_output_sub_dir + "/" + gen_file
                with open(full_filename, "w") as f:
                    f.write(rendered)

    def __raise_error(self, msg: str) -> None:
        raise Exception(msg)

    def __is_builtin(self, typename: str) -> bool:
        builtin_types = (
            "uint8_t",
            "uint16_t",
            "uint32_t",
            "uint64_t",
            "int8_t",
            "int16_t",
            "int32_t",
            "int64_t",
            "bool",
        )
        return typename in builtin_types

    def __is_numeric_unsigned(self, element_type: str) -> bool:
        """Return: True if parameter is of type numeric."""
        annotation = NumericUnsigned.model_fields["element_type"].annotation
        if element_type in typing.get_args(annotation):
            return True
        else:
            return False

    def __is_numeric_signed(self, element_type: str) -> bool:
        """Return: True if parameter is of type numeric."""
        annotation = NumericSigned.model_fields["element_type"].annotation
        if element_type in typing.get_args(annotation):
            return True
        else:
            return False

    def __is_numeric(self, element_type: str) -> bool:
        """Return: True if parameter is of type numeric."""
        if self.__is_numeric_unsigned(element_type) or self.__is_numeric_signed(element_type):
            return True
        else:
            return False

    def __get_range(self, param: Any) -> Any:
        """Return: range, if defined."""
        if self.__is_numeric(param.element_type):
            return param.num_range
        elif param.element_type == "array" and self.__is_numeric(param.item_type):
            return param.num_range
        else:
            return None

    def __is_native_type(self, element_type: str) -> bool:
        """Return: True if type is native (i.e. enum, struct, numeric etc), or not."""
        for a in typing.get_args(TypeUnion):
            annotation = a.model_fields["element_type"].annotation
            if element_type in typing.get_args(annotation):
                return True
        for a in typing.get_args(StructFieldTypeUnion):
            annotation = a.model_fields["element_type"].annotation
            if element_type in typing.get_args(annotation):
                return True
        return False

    def __get_base_type(self, element_type: str) -> tuple[str, bool]:
        """Return: Look for the base type, i.e. enum, struct, uint8_t etc."""
        if self.__is_native_type(element_type):
            return element_type, True
        else:
            for type_name, type_obj in self.config_model.types.items():
                if type_name == element_type:
                    element_type = type_obj.element_type
                    return element_type, False
            logger.error(f"Type {element_type} is undefined")
            sys.exit(errno.EINVAL)

    def __get_full_type(self, element_type: str) -> str:
        """Return: Full type, i.e. base type keyword if any (struct, enum) + type name."""
        full_type, is_native_type = self.__get_base_type(element_type)
        if not is_native_type:
            # When an array is defined as a type, a typedef is created.
            # In that case, no identifier is expected, only the typedef name.
            if full_type == "array":
                full_type = element_type
            else:
                # Full type is type identifier (struct, enum, etc.) + name.
                if full_type == "enum":
                    # Replace enum by uint8_t to optimize memory.
                    return "uint8_t"
                if full_type == "bitfield":
                    # Bitfield is coded as a struct.
                    full_type = "struct"
                full_type = full_type + " " + element_type
        return full_type

    def __find_type(self, type_name: str) -> TypeUnion:
        """Look for custom type (struct, bitfield, etc.) in the whole model."""
        for name, param in self.config_model.types.items():
            if name == type_name:
                return param
        logger.error(f"Type {type_name} not found")
        sys.exit(errno.EINVAL)

    # Check if indexes of an itemized are defined by an enum, or by a max value.
    def __is_itemized_indexes_enum(
        self, itemized: ItemizedStructField
    ) -> tuple[bool, Union[TypeUnion, None]]:
        for name, param in self.config_model.types.items():
            if itemized.indexes == name and param.element_type == "enum":
                return True, param
        return False, None

    def __get_itemized_size_name(self, name: str, itemized: ItemizedStructField) -> str:
        # Check if indexes are given by an enum.
        is_enum, enum = self.__is_itemized_indexes_enum(itemized)
        if is_enum == True:
            enum_name = itemized.indexes
            return enum_name.upper() + "_NUM"
        # Indexes not defined by an enum, so it must be a range defined by its max value
        return itemized.indexes.upper()

    def __get_define_value(self, name: str) -> int:
        for define_name, define in self.config_model.defines.items():
            if name == define_name:
                return int(define.value)
        logger.error(f"Define {name} not found")
        sys.exit(errno.EINVAL)

    def __get_itemized_idx_list(self, itemized: ItemizedStructField) -> list[str]:
        # Check if indexes are given by an enum.
        is_enum, enum = self.__is_itemized_indexes_enum(itemized)
        if is_enum == True:
            index_str_list = []
            assert isinstance(enum, Enum)
            for value_name, value in enum.values.items():
                index_str_list.append(value_name)
            return index_str_list
        # Indexes not defined by an enum, so it must be a range defined by its max value.
        range_max = self.__get_define_value(itemized.indexes)
        index_str_list = [*range(0, range_max)]
        return index_str_list

    def __get_itemized_size(self, itemized: ItemizedStructField) -> int:
        return len(self.__get_itemized_idx_list(itemized))

    def __get_getter_func(self, name: str, param: Any, parent_name: str) -> str:
        getter_func = self.plugin_name + "_read_"
        if param.alias:
            # Use the alias, if defined in the model.
            getter_func += param.alias
        else:
            if parent_name != "":
                getter_func += parent_name + "_"
            getter_func += name
        return getter_func

    def __has_checker_func(self, name: str, param: Any) -> bool:
        if self.__get_range(param) is not None:
            return True
        base_type, is_native_type = self.__get_base_type(param.element_type)
        if not is_native_type:
            param = self.__find_type(param.element_type)
        if base_type == "enum":
            return True
        elif base_type == "bool":
            return True
        elif base_type == "bitfield":
            for bits_name, bits in param.bits.items():
                if bits.num_range is not None:
                    return True
                elif bits.element_type is not None:
                    return True
        elif base_type == "struct":
            for field_name, field in param.fields.items():
                if self.__has_checker_func(field_name, field):
                    return True
        elif base_type == "array":
            if not self.__is_native_type(param.item_type):
                item = self.__find_type(param.item_type)
                return self.__has_checker_func(param.item_type, item)
        return False

    def __get_checker_func(self, name: str, param: Any) -> str:
        if not self.__has_checker_func(name, param):
            return "NULL"
        key_policy_check = self.plugin_name + "_policy_check"
        base_type, is_native_type = self.__get_base_type(param.element_type)
        if is_native_type:
            type_name = name
        else:
            type_name = param.element_type
        if base_type == "bool":
            key_policy_check += "_bool"
        else:
            key_policy_check += "_" + type_name
        return key_policy_check

    def __get_nb_keys_recursive(self, name: str, param: Any, nb_keys: int) -> int:
        base_type, is_native_type = self.__get_base_type(param.element_type)
        if base_type == "itemized":
            struct = self.__find_type(param.item_type)
            itemized_size = self.__get_itemized_size(param)
            nb_itemized_keys = 0
            for field_name, field in struct.fields.items():
                nb_itemized_keys = self.__get_nb_keys_recursive(field_name, field, nb_itemized_keys)
            nb_keys += itemized_size * nb_itemized_keys
        elif base_type == "struct":
            struct = self.__find_type(param.element_type)
            for field_name, field in struct.fields.items():
                nb_keys = self.__get_nb_keys_recursive(field_name, field, nb_keys)
        else:
            nb_keys += 1
        return nb_keys

    def __get_nb_keys(self) -> int:
        nb_keys = 0
        root_struct = self.__find_type(self.config_model.root)
        for name, param in root_struct.fields.items():
            nb_keys = self.__get_nb_keys_recursive(name, param, nb_keys)
        return nb_keys

    def __get_nb_root_key_sections_recursive(self, name: str, param: Any, nb_keys: int) -> int:
        base_type, is_native_type = self.__get_base_type(param.element_type)
        if base_type == "struct":
            struct = self.__find_type(param.element_type)
            for field_name, field in struct.fields.items():
                nb_keys = self.__get_nb_keys_recursive(field_name, field, nb_keys)
        else:
            nb_keys += 1
        return nb_keys

    def __get_nb_root_key_sections(self) -> int:
        nb_keys = 0
        root_struct = self.__find_type(self.config_model.root)
        for name, param in root_struct.fields.items():
            nb_keys = self.__get_nb_root_key_sections_recursive(name, param, nb_keys)
        return nb_keys
