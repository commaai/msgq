"""Helper functionality that is used in other modules of this package."""
from __future__ import annotations

from copy import copy
from dataclasses import dataclass
from dataclasses import field
from typing import Any

BUILDER_NAME = "Builder"
READER_NAME = "Reader"


def new_builder(type_name: str) -> str:
    """Converts a type name to its builder variant.

    E.g. `MyClass` becomes `MyClassBuilder`.

    Args:
        type_name (str): The original type name.

    Returns:
        str: The builder variant.
    """
    return f"{type_name}{BUILDER_NAME}"


def new_reader(type_name: str) -> str:
    """Converts a type name to its reader variant.

    E.g. `MyClass` becomes `MyClassTeader`.

    Args:
        type_name (str): The original type name.

    Returns:
        str: The reader variant.
    """
    return f"{type_name}{READER_NAME}"


@dataclass
class TypeHint:
    """A class that captures a type hint."""

    name: str
    scopes: list[str] = field(default_factory=list)
    affix: str = ""
    primary: bool = False

    def __str__(self) -> str:
        """The string representation of the type hint.

        This is composed of the scopes (if any), the name of the hint, and the affix (if any).
        """
        if not self.scopes:
            return f"{self.name}{self.affix}"

        else:
            return f"{'.'.join(self.scopes)}.{self.name}{self.affix}"


@dataclass
class TypeHintedVariable:
    """A class that represents a type hinted variable."""

    name: str
    type_hints: list[TypeHint]
    default: str = ""
    nesting_depth: int = 0

    def __post_init__(self):
        """Sanity check for provided type hints."""
        primary_type_hint_count = 0

        for type_hint in self.type_hints:
            if type_hint.primary:
                primary_type_hint_count += 1

        if primary_type_hint_count != 1:
            raise ValueError(f"There can only be exactly one primary type hint. Found {primary_type_hint_count}")

    def __str__(self) -> str:
        """String representation of this object.

        Returns:
            str: The string representation.
        """
        return self.typed_variable_with_full_hints

    def _nest(self, unnested_type_name: str) -> str:
        if self.nesting_depth > 0:
            return f'{self.nesting_depth * "Sequence["}{unnested_type_name}{self.nesting_depth * "]"}'

        else:
            return unnested_type_name

    @property
    def typed_variable_with_full_hints(self) -> str:
        """Returns the typed variable string, hinted will all available type hints."""
        return self._generate_typed_variable(self.full_type)

    @property
    def typed_variable_with_primary_hint(self) -> str:
        """Returns the typed variable string, hinted only with the primary type hint."""
        if self.primary_type_hint is None:
            raise ValueError("No primary type hint available.")

        return self._generate_typed_variable(str(self.primary_type_hint))

    @property
    def primary_type_hint(self) -> TypeHint:
        """Returns the primary type hint."""
        for type_hint in self.type_hints:
            if type_hint.primary:
                return type_hint

        raise RuntimeError("Primary type hint not found.")

    def _generate_typed_variable(self, type_name: str) -> str:
        """Generate the typed variable string for a chosen type name.

        Args:
            type_name (str): The type name to use.

        Returns:
            str: The typed variable string.
        """
        nested_type_name = self._nest(type_name)
        typed_variable = f"{self.name}: {nested_type_name}"

        if self.default:
            return f"{typed_variable} = {self.default}"

        else:
            return typed_variable

    def _get_type_hints_for_affixes(self, affixes: list[str]) -> list[TypeHint]:
        return [self.get_type_hint_for_affix(affix) for affix in affixes]

    def get_typed_variable_with_affixes(self, affixes: list[str] | None) -> str:
        """Gets a typed version of this variable with the selected type hint affixes.

        For example, when selecting the affixes "Reader" and "Builder" for the primary type "Something" and the variable
        named "VariableName", this will return "VariableName: SomethingReader | SomethingBuilder".

        Args:
            affixes (list[str] | None): The affixes to select for type hints.

        Returns:
            str: The typed variable in string format.
        """
        type_hints: list[TypeHint] = []

        if affixes is not None:
            type_hints_for_affixes = self._get_type_hints_for_affixes(affixes)

            if type_hints_for_affixes is not None:
                type_hints.extend(type_hints_for_affixes)

        return self._generate_typed_variable(self._join_type_hints(type_hints))

    def _join_type_hints(self, type_hints: list[TypeHint]) -> str:
        return " | ".join(str(type_hint) for type_hint in type_hints)

    @property
    def full_type(self) -> str:
        """The full type string of the hinted variable."""
        return self._join_type_hints(self.type_hints)

    def add_type_hint(self, new_type_hint: TypeHint):
        """Add a type hint to the hinted variable.

        Args:
            new_type_hint (TypeHint): The type hint to add.
        """
        for type_hint in self.type_hints:
            if type_hint == new_type_hint:
                raise ValueError("Type hint already exists.")

        if new_type_hint.primary:
            raise ValueError("There can only be one primary type.")

        self.type_hints.append(new_type_hint)

    def add_type_scope(self, scope: str):
        """Add a scope to the type name.

        Args:
            scope (str): The scope name to prefix to the type name.
        """
        for type_hint in self.type_hints:
            type_hint.scopes.append(scope)

    def get_type_hint_for_affix(self, affix: str) -> TypeHint:
        """Looks for a type hint that has the provided affix and returns it.

        Args:
            affix (str | None): The affix to look for.

        Returns:
            TypeHint: The type hint that was found.
        """
        for type_hint in self.type_hints:
            if type_hint.affix == affix:
                return type_hint

        raise KeyError(f"Affix '{affix}' is not present in any recorded type hint.")

    def has_type_hint_with_affix(self, affix: str) -> bool:
        """Assess, whether or not the variable has a type hint with the provided affix."""
        try:
            self.get_type_hint_for_affix(affix)

        except KeyError:
            return False

        else:
            return True

    @property
    def has_type_hint_with_builder_affix(self) -> bool:
        """Whether the variable holds a type hint with a builder affix."""
        return self.has_type_hint_with_affix(BUILDER_NAME)

    @property
    def has_type_hint_with_reader_affix(self) -> bool:
        """Whether the variable holds a type hint with a reader affix."""
        return self.has_type_hint_with_affix(READER_NAME)

    def add_builder_from_primary_type(self):
        """Add a type hint with builder affix, based on the primary type."""
        self.add_type_hint(TypeHint(self.primary_type_hint.name, copy(self.primary_type_hint.scopes), BUILDER_NAME))

    def add_reader_from_primary_type(self):
        """Add a type hint with builder affix, based on the primary type."""
        self.add_type_hint(
            TypeHint(
                self.primary_type_hint.name,
                copy(self.primary_type_hint.scopes),
                READER_NAME,
            )
        )


def replace_capnp_suffix(original: str) -> str:
    """If found, replaces the .capnp suffix in a string with _capnp.

    For example, `some_module.capnp` becomes `some_module_capnp`.

    Args:
        original (str): The string to replace the suffix in.

    Returns:
        str: The string with the replaced suffix.
    """
    if original.endswith(".capnp"):
        return original.replace(".capnp", "_capnp")

    else:
        return original


def join_parameters(parameters: list[TypeHintedVariable] | list[str] | None) -> str:
    """Joins parameters by means of ', '.

    Args:
        parameters (list[HintedVariable] | list[str] | None): The parameters to join.

    Returns:
        str: The joined parameters.
    """
    if parameters:
        return ", ".join(str(p) for p in parameters if p)

    else:
        return ""


def new_type_alias(alias: str, type_name: str) -> str:
    """Generate a string for a type-alias.

    For example, for a type alias `SomeType` with a type `TheType` this gives `SomeType = TheType`.

    Args:
        alias (str): The alias name.
        type_name (str): The aliased type.

    Returns:
        str: The type alias.
    """
    return f"{alias} = {type_name}"


def new_group(name: str, members: list[str]) -> str:
    """Create a string for a group name and its members.

    For example, when the group name is 'Type', and the parameters are 'str', and 'int',
    the output will be 'Type[str, int]'.

    Args:
        name (str): The name of the group.
        members (list[str]): The members of the group

    Returns:
        str: The resulting group string.
    """
    return f"{name}[{join_parameters(members)}]"


def new_type_group(name: str, types: list[str]) -> str:
    """Create a string for a parameter with types.

    Uses `new_group` internally.

    Args:
        name (str): The name of the parameter.
        types (list[str]): The list of types to that this parameter can have.

    Returns:
        str: The resulting parameter string.
    """
    return new_group(name, types)


def new_function(
    name: str, parameters: list[TypeHintedVariable] | list[str] | None = None, return_type: str | None = None
) -> str:
    """Create a string for a function.

    Args:
        name (str): The function name.
        parameters (list[HintedVariable] | list[str] | None, optional): The function parameters, if any. Defaults to None.
        return_type (str | None, optional): The function's return type. Defaults to None.

    Returns:
        str: The function string.
    """
    if return_type is None:
        return_type = "None"

    arguments = join_parameters(parameters)
    return f"def {name}({arguments}) -> {return_type}: ..."


def new_decorator(name: str, parameters: list[TypeHintedVariable] | list[str] | None = None) -> str:
    """Create a new decorator.

    Args:
        name (str): The name of the decorator.
        parameters (list[HintedVariable] | list[str] | None, optional): The parameters (args, kwargs) of the decorator,
            if any. Defaults to None.

    Returns:
        str: The decorator string.
    """
    if parameters:
        return f"@{name}({join_parameters(parameters)})"

    else:
        return f"@{name}"


def new_constructor(kwargs: list[str] | None = None) -> str:
    """Creates a new constructor.

    Args:
        kwargs (list[str] | None, optional): The keyword arguments for the constructor. Defaults to None.

    Returns:
        str: The constructor string.
    """
    constructor_args = "self"

    kwargs_ = ""

    if kwargs:
        # Prepend '*' for catching any positional arguments, before the keyword arguments.
        kwargs_ = join_parameters(["*"] + [f"{kwarg} = ..." for kwarg in kwargs])

    return new_function("__init__", parameters=[constructor_args, kwargs_])


def new_class_declaration(name: str, parameters: list[str] | None = None) -> str:
    """Creates a string for declaring a class.

    For example, for a name of 'SomeClass' and a list of parameters that is 'str, Type[str, int]', the output
    will be 'SomeClass (str, Type[str, int]):'.

    If no parameters are provided, the output is just 'SomeClass:'.

    Args:
        name (str): The class name.
        parameters (list[str] | None, optional):
            A list of parameters that are part of the class declaration. Defaults to None.

    Returns:
        str: The class declaration.
    """
    if parameters:
        return f"class {name}({join_parameters(parameters)}):"

    else:
        return f"class {name}:"


def get_display_name(schema: Any) -> str:
    """Extract the display name from a schema.

    Args:
        schema (Any): The schema to get the display name from.

    Returns:
        str: The display name of the schema.
    """
    return schema.node.displayName[schema.node.displayNamePrefixLength :]
