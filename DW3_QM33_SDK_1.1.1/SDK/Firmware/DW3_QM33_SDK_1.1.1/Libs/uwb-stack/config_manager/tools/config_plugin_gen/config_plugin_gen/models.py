"""configuration DSL model."""

import typing
import annotated_types
from typing_extensions import TypeAliasType
from typing import (
    Any,
    Union,
    Dict,
    Optional,
    List,
)
from pathlib import Path
from pydantic import (
    Field,
    BaseModel,
    ValidationInfo,
    ValidationError,
    model_validator,
    field_validator,
    create_model,
    ConfigDict,
    conint,
    WithJsonSchema,
)
from typing_extensions import Annotated, Literal
from loguru import logger
from contextlib import contextmanager
from contextvars import ContextVar
import re
import enum
import math
from expr import evaluate
from tt import BooleanExpression
from contextvars import ContextVar
from typing import Iterator
from pydantic import BaseModel, ValidationInfo

_init_context_var = ContextVar("_init_context_var", default={})

""" Constraint types """
conint8 = Annotated[int, Field(strict=True, ge=-128, le=127)]
conuint8 = Annotated[int, Field(strict=True, ge=0, le=255)]
conuint32 = Annotated[int, Field(strict=True, ge=0, le=0xFFFFFFFF)]

_validation_context = ContextVar("_validation_context", default=None)


@contextmanager
def init_context(value: Dict[str, Any]) -> Iterator[None]:
    token = _validation_context.set(value)
    try:
        yield
    finally:
        _validation_context.reset(token)


class DescribedType(BaseModel):
    """Base class for all types that have a summary, and optionaly a description."""

    summary: Optional[str]
    description: Optional[str] = None

    # Optional field allowing to give a friendly name to the getter API for a parameter
    alias: Optional[str] = None


ConfigModel = typing.NewType("ConfigModel", Any)


class Define(DescribedType):
    value: int


class Bool(DescribedType):
    default: bool
    element_type: Literal["bool"] = Field(alias="type")

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Field:
        return bool


class EnumValue(DescribedType):
    value: Optional[conuint8]


class Enum(DescribedType):
    element_type: Literal["enum"] = Field(alias="type")
    values: Dict[str, EnumValue]

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Field:
        enum_values = {}
        enum_fields = {}
        for f_name, f_value in self.values.items():
            enum_fields[f_name] = f_name
            enum_values[f_name] = f_value.value
        return Annotated[
            str,
            ConfigDict(extra="allow"),
            WithJsonSchema(
                {
                    "type": "string",
                    "enum": list(enum_fields.keys()),
                    "underlying_enum": enum_values,
                }
            ),
        ]


class BitFieldBits(DescribedType):
    """Bits contained in a BitField."""

    size: conuint8
    default: Union[int, str]
    # Optional fields, in order to create checker functions
    num_range: str = Field(alias="range", default=None)
    element_type: str = Field(alias="type", default=None)

    @field_validator("num_range")
    def check_range(cls, v: str, info: ValidationInfo) -> str:
        pattern = r"^\d+\.\.\d+$"
        if not re.match(pattern, v):
            min_max = v.split("..")
            if len(min_max) != 2:
                raise ValidationError('Invalid range format. Expected "integer..integer".')
        return v

    @staticmethod
    def check_default(default: Union[int, str], enum: Enum, root: ConfigModel) -> None:
        if isinstance(default, str):
            if enum == None:
                if not check_define(default, root.defines):
                    raise Exception(f"Default {default} is not defined")
            else:
                if not check_enum_val(default, enum):
                    raise Exception(f"Default {default} is not defined in enum values")


class BitField(DescribedType):
    element_type: Literal["bitfield"] = Field(alias="type")
    bits: Dict[str, BitFieldBits]

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        fields = {}
        for name, b in self.bits.items():
            fields[name] = (conint(ge=0, le=math.pow(2, b.size) - 1), ...)

            enum = None
            if b.element_type is not None:
                param = find_type(b.element_type, root.types)
                if param is None or param.element_type != "enum":
                    raise Exception(f"Field type of BitFieldBits should be enum")
                enum = param

            b.check_default(b.default, enum, root)

        return create_model(
            type_name,
            __config__=ConfigDict(json_schema_extra={"underlying_type": "bitfield"}),
            **fields,
        )


class Array(DescribedType):
    element_type: Literal["array"] = Field(alias="type")
    item_type: str
    size: str
    num_range: str = Field(alias="range", default=None)

    # Default is optional because only needed for certain types of `item_type`.
    # Its presence is checked in convert_to_model.
    default: Optional[list[Union[int, str]]] = None

    #    @field_validator('num_range')
    #    def check_range(cls, v: str, info: ValidationInfo) -> str:
    #        pattern = r'^\d+\.\.\d+$'
    #        if not re.match(pattern, v):
    #            min_max = v.split('..')
    #            if len(min_max) != 2:
    #                raise ValidationError('Invalid range format. Expected "integer..integer".')
    #        return v

    @staticmethod
    def check_default(
        default: Union[list[Union[int, str]], None], item_type: str, size: int, root: ConfigModel
    ) -> None:
        param = find_type(item_type, root.types)
        is_enum = param is not None and param.element_type == "enum"

        if default == None:
            if is_numeric(item_type):
                raise ValueError(f"Default is mandatory for items of type numeric")
            elif item_type == "bool":
                raise ValueError(f"Default is mandatory for items of type bool")
            elif is_enum:
                raise ValueError(f"Default is mandatory for items of type enum")
        else:
            assert isinstance(default, list)
            if len(default) != size:
                raise ValueError(f"Default should contain {size} elements")
            for d in default:
                if isinstance(d, str):
                    if is_enum:
                        if not check_enum_val(d, param):
                            raise Exception(f"Default {d} is not defined in enum values")
                    else:
                        if not check_define(d, root.defines):
                            raise Exception(f"Default {d} is not defined")
                else:
                    if is_enum:
                        raise ValueError(f"Default should be an array of enum values")

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        if self.item_type not in root.types.keys():
            raise ValueError(f"Type {self.item_type} is not defined")

        size = None
        if self.size.isdigit():
            size = int(self.size)
        elif self.size in root.defines.keys():
            d = root.defines[self.size]
            if isinstance(d, Define):
                size = int(d.value)

        if size is None:
            raise ValueError(f"Index type {self.size} is not defined")

        self.check_default(self.default, self.item_type, size, root)

        if isinstance(size, Define):
            size = int(size.value)

        conv_item_type = None
        if typing.get_origin(converted_types[self.item_type]) is None:
            conv_item_type = converted_types[self.item_type]
        else:
            conv_item_type = converted_types[self.item_type]

        return Annotated[List[conv_item_type], annotated_types.Len(0, size)]


class ItemizedStructField(DescribedType):
    element_type: Literal["itemized"] = Field(alias="type")
    item_type: str
    indexes: str

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        fields = {}
        if self.item_type not in converted_types.keys():
            raise ValueError(f"Type {self.item_type} is not defined")

        index_type = None

        if (
            index_type := (
                None if self.indexes not in root.types.keys() else root.types[self.indexes]
            )
        ) is None and (
            index_type := (
                None if self.indexes not in root.defines.keys() else root.defines[self.indexes]
            )
        ) is None:
            raise ValueError(f"Index type {self.indexes} is not defined")

        # indexes_types in types.keys():
        conv_item_type = None
        if typing.get_origin(converted_types[self.item_type]) is None:
            conv_item_type = converted_types[self.item_type]
        else:
            conv_item_type = TypeAliasType(self.item_type, converted_types[self.item_type])

        if isinstance(index_type, Enum):
            for key, _ in index_type.values.items():
                fields.update({f"{type_name}{key}": conv_item_type})
        elif isinstance(index_type, Define):
            for i in range(0, int(index_type.value)):
                fields.update({f"{type_name}{i}": conv_item_type})
        else:
            raise ValueError(f"Index {self.indexes} is not an Enum or a Define")
        return fields


class Numeric(DescribedType):
    default: Union[int, str]
    num_range: str = Field(alias="range", default=None)

    @field_validator("num_range")
    def check_range(cls, v: str, info: ValidationInfo) -> str:
        pattern = r"^[A-Za-z\d_-]+\.\.[A-Za-z\d_-]+$"
        if not re.match(pattern, v):
            raise Exception(f"Invalid range {v}", 'Expected "integer..integer".')
        return v

    @staticmethod
    def evaluate_range_limit(val: str, defines: Dict[str, Define]) -> int:
        try:
            res = int(val)
        except ValueError:
            vars = {}
            for d_name, d in defines.items():
                vars[d_name] = d.value
            return int(evaluate(val, variables=vars))

        return res

    @staticmethod
    def get_min_max(element_type: str, min_max: Dict[str, tuple[int, int]]) -> tuple[int, int]:
        if element_type not in min_max.keys():
            raise Exception(f"Type {element_type} is not numeric")

        min, max = min_max[element_type]
        return min, max

    @staticmethod
    def parse_range_min_max(
        num_range: str,
        element_type: str,
        defines: Dict[str, Define],
        min_max: Dict[str, tuple[int, int]],
    ) -> tuple[int, int]:
        """Convert to a pydantic Field."""
        min, max = Numeric.get_min_max(element_type, min_max)
        if num_range is not None:
            r_min_str, r_max_str = num_range.split("..")

            r_min = Numeric.evaluate_range_limit(r_min_str, defines)
            r_max = Numeric.evaluate_range_limit(r_max_str, defines)

            if r_min < min or r_max > max:
                raise Exception(f"Range {num_range} is not valid")

            return r_min, r_max

        return min, max

    @staticmethod
    def check_default(
        default: Union[int, str],
        element_type: str,
        defines: Dict[str, Define],
        min_max: Dict[str, tuple[int, int]],
    ) -> None:
        if isinstance(default, str):
            if not check_define(default, defines):
                raise Exception(f"Default {default} is not defined")
            val_str = default
        else:
            val_str = str(default)
        val = Numeric.evaluate_range_limit(val_str, defines)
        min, max = Numeric.get_min_max(element_type, min_max)
        if val < min or val > max:
            raise Exception(f"Invalid range for default value {default}")

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        element_type = self.element_type
        num_range = self.num_range
        min, max = self.parse_range_and_default(num_range, self.default, element_type, root.defines)

        return Annotated[
            int,
            WithJsonSchema(
                {
                    "maximum": max,
                    "minimum": min,
                    "type": "integer",
                    "underlying_type": self.element_type,
                }
            ),
        ]


class NumericUnsigned(Numeric):
    element_type: Literal[
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
    ] = Field(alias="type")

    def parse_range_and_default(
        self,
        num_range: str,
        default: Union[int, str],
        element_type: str,
        defines: Dict[str, Define],
    ) -> tuple[int, int]:
        """Convert to a pydantic Field."""
        min_max = {
            "uint8_t": (0, 255),
            "uint16_t": (0, 65535),
            "uint32_t": (0, 4294967295),
            "uint64_t": (0, 18446744073709551615),
        }
        min, max = Numeric.parse_range_min_max(num_range, element_type, defines, min_max)
        Numeric.check_default(default, element_type, defines, min_max)
        return min, max


class NumericSigned(Numeric):
    element_type: Literal[
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
    ] = Field(alias="type")

    def parse_range_and_default(
        self,
        num_range: str,
        default: Union[int, str],
        element_type: str,
        defines: Dict[str, Define],
    ) -> tuple[int, int]:
        """Convert to a pydantic Field."""
        min_max = {
            "int8_t": (-128, 127),
            "int16_t": (-32768, 32767),
            "int32_t": (-2147483648, 2147483647),
            "int64_t": (-9223372036854775808, 9223372036854775807),
        }
        min, max = Numeric.parse_range_min_max(num_range, element_type, defines, min_max)
        Numeric.check_default(default, element_type, defines, min_max)
        return min, max


class CustomStructField(DescribedType):
    element_type: str = Field(alias="type")

    # Default is optional because only needed for certain types (ex: enum).
    default: Optional[Union[int, str]] = None

    @staticmethod
    def check_default(
        default: Optional[Union[int, str]], element_type: str, root: ConfigModel
    ) -> None:
        param = find_type(element_type, root.types)
        if param is not None and param.element_type == "enum":
            if default == None:
                raise ValueError(f"Default is mandatory for enum struct field")

            if not isinstance(default, str):
                raise ValueError(f"Default should be one of the enum values for enum struct field")

            if not check_enum_val(default, param):
                raise Exception(f"Default {default} is not defined in enum values")

    def convert_to_model(
        self, type_name: str, converted_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        """Convert to a pydantic Field."""
        if self.element_type not in root.types.keys():
            raise ValueError(f"Type {self.element_type} is not defined")

        self.check_default(self.default, self.element_type, root)

        if typing.get_origin(converted_types[self.element_type]) is None:
            return converted_types[self.element_type]
        else:
            return TypeAliasType(type_name, converted_types[self.element_type])


StructFieldTypeUnion = Union[
    ItemizedStructField,
    NumericUnsigned,
    NumericSigned,
    Bool,
    Array,
    CustomStructField,
]


class Struct(DescribedType):
    """Struct type."""

    element_type: Literal["struct"] = Field(alias="type")
    fields: Dict[str, StructFieldTypeUnion] = Field(discriminator="element_type")

    def convert_to_model(
        self, type_name: str, converted_model_types: Dict[str, Any], root: ConfigModel
    ) -> Any:
        fields = {}
        for name, b in self.fields.items():
            if isinstance(b, ItemizedStructField):
                converted_type = b.convert_to_model(name, converted_model_types, root)
                for f_name, f_type in converted_type.items():
                    fields.update({f_name: (f_type, ...)})
            else:
                converted_type = (
                    b.convert_to_model(b.element_type, converted_model_types, root),
                    ...,
                )
                fields.update({name: converted_type})
        return create_model(type_name, **fields)


TypeUnion = Union[Enum, BitField, Struct, Bool, Array, NumericUnsigned, NumericSigned]


def is_numeric(element_type: str) -> bool:
    """Return: True if parameter is of type numeric."""
    annotation_u = NumericUnsigned.model_fields["element_type"].annotation
    annotation_s = NumericSigned.model_fields["element_type"].annotation
    if element_type in typing.get_args(annotation_u) or element_type in typing.get_args(
        annotation_s
    ):
        return True
    else:
        return False


def find_type(element_type: str, types: Dict[str, TypeUnion]) -> Any:
    for name, param in types.items():
        if name == element_type:
            return param
    return None


def check_enum_val(value: str, enum: Enum) -> bool:
    for key, _ in enum.values.items():
        if key == value:
            return True
    return False


def check_define(name: str, defines: Dict[str, Define]) -> bool:
    for d_name, d in defines.items():
        if d_name == name:
            return True
    return False


class ConfigModel(BaseModel):
    version: conuint32
    defines: Dict[str, Define]
    types: Dict[str, TypeUnion] = Field(discriminator="element_type")
    root: str

    def __init__(__pydantic_self__, **data: Any) -> None:
        __pydantic_self__.__pydantic_validator__.validate_python(
            data,
            self_instance=__pydantic_self__,
            context=_validation_context.get(),
        )

    def convert_to_model(self) -> Dict[str, Any]:
        converted_types: Dict[str, Any] = {}
        for t_name, t in self.types.items():
            converted_types[t_name] = t.convert_to_model(t_name, converted_types, self)

        if self.root not in converted_types.keys():
            raise ValueError(f"Struct {self.root} used as root is not defined")
        root = converted_types[self.root]

        return root

    @model_validator(mode="after")
    def check_types(self) -> "ConfigModel":
        builtin_types = {}
        builtin_types.update(
            {
                "uint8_t": NumericUnsigned.model_construct(
                    None, element_type="uint8_t", summary="uint8_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "uint16_t": NumericUnsigned.model_construct(
                    None, element_type="uint16_t", summary="uint16_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "uint32_t": NumericUnsigned.model_construct(
                    None, element_type="uint32_t", summary="uint32_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "uint64_t": NumericUnsigned.model_construct(
                    None, element_type="uint64_t", summary="uint64_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "int8_t": NumericSigned.model_construct(
                    None, element_type="int8_t", summary="int8_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "int16_t": NumericSigned.model_construct(
                    None, element_type="int16_t", summary="int16_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "int32_t": NumericSigned.model_construct(
                    None, element_type="int32_t", summary="int32_t", default=0
                )
            }
        )
        builtin_types.update(
            {
                "int64_t": NumericSigned.model_construct(
                    None, element_type="int64_t", summary="int64_t", default=0
                )
            }
        )
        builtin_types.update(
            {"bool": Bool.model_construct(None, element_type="bool", summary="bool")}
        )

        self.types = {**builtin_types, **self.types}
        for tk, t in self.types.items():
            if type(t) is Struct:
                for fk, f in t.fields.items():
                    if type(f) is CustomStructField:
                        if f.element_type not in self.types.keys():
                            raise ValueError(f"Type {f.element_type} is not defined")
                    if type(f) is ItemizedStructField:
                        if f.item_type not in self.types.keys():
                            raise ValueError(f"Type {f.item_type} is not defined")
                    if type(f) is Array:
                        if not is_numeric(f.item_type) and f.item_type not in self.types.keys():
                            raise ValueError(f"Type {f.item_type} is not defined")
        if self.root not in self.types.keys():
            raise ValueError(f"Struct {self.root} used as root is not defined")

        return self

    @model_validator(mode="before")
    @classmethod
    def apply_flag(cls, data: Any, info: Any) -> Any:
        defines = {}
        for d in info.context["defines"]:
            [d_name, d_val] = d.split("=")
            data["defines"].update({d_name: {"value": d_val, "summary": d_name}})
            defines[d_name] = d_val
        return ConfigModel.filter_dict(data, defines)

    @staticmethod
    def filter_dict(d: Any, context: Any) -> Any:
        filtered_dict = {}
        for key, value in d.items():
            if isinstance(value, dict):
                # If the value is a dictionary, filter it recursively
                filtered_value = ConfigModel.filter_dict(value, context)
                if filtered_value:  # Don't include empty dictionaries
                    filtered_dict[key] = filtered_value
            elif key == "flag":
                res = True
                try:
                    be = BooleanExpression(value)
                    missing_syms = [item for item in be.symbols if item not in context.keys()]
                    for ms in missing_syms:
                        context[ms] = 0
                    res = be.evaluate_unchecked(**context)
                except:
                    raise Exception(f"Invalid expression: {value}")

                if not res:
                    return {}
            else:
                # If the value is not a dictionary, include it if it matches the predicate
                filtered_dict[key] = value
        return filtered_dict
