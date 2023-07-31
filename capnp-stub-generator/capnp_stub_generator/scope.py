"""This module defines the scope, a unit of indented text."""
from __future__ import annotations

import dataclasses
import logging
from typing import Any
from typing import Literal

from .helper import TypeHintedVariable

logger = logging.getLogger(__name__)

INDENT_SPACES = 4


class NoParentError(Exception):
    """Raised, when the parent of a scope is not available."""


@dataclasses.dataclass
class Scope:
    """A scope within the output .pyi file.

    Scopes contain text and are indented by a certain amount. They often have parents, within which they are located.

    Args:
        name (str): The name of the scope. Use an empty name for the root scope ("").
        id (int): A numerical identifier of the scope.
        parent (Scope | None): The direct parent scope of this scope, if there is any.
        return scope (Scope | None): The scope to which to return, when closing this one.
        lines (list[str]): The list of text lines in this scope.
    """

    name: str
    id: int
    parent: Scope | None
    return_scope: Scope | None
    lines: list[str] = dataclasses.field(default_factory=list)

    def __post_init__(self):
        """Assures that, if this is the root scope, its name is empty."""
        assert (self.is_root) == (self.name == "")

    @property
    def parents(self) -> list[Scope]:
        """A list of all parent scopes of this scope, starting from the first parent.

        If the returned list is empty, this scope has no parents. The first parent in the list has no further
        parents, it is the root scope.
        """
        parents: list[Scope] = []
        scope: Scope | None = self.parent

        while scope is not None:
            parents.append(scope)
            scope = scope.parent

        parents.reverse()

        return parents

    @property
    def trace(self) -> list[Scope]:
        """A list of all scopes that lead to this scope, starting from the first parent.

        The first parent has no further parents.
        """
        return self.parents + [self]

    @property
    def root(self) -> Scope:
        """Get the root scope that has no further parents."""
        if not self.parents:
            return self

        else:
            return self.parents[0]

    @property
    def is_root(self) -> bool:
        """Determine, whether this is the root scope."""
        return self.root == self

    @property
    def indent_spaces(self) -> int:
        """The number of spaces by which this scope is indented."""
        return len(self.parents) * INDENT_SPACES

    def add(self, content: str | TypeHintedVariable = ""):
        """Add content to this scope, taking into account the current indent spaces.

        Args:
            content (str | HintedVariable): The line or variable to add. Optional, defaults to "".
        """
        if isinstance(content, TypeHintedVariable):
            content = str(content)

        if not content:
            self.lines.append("")

        else:
            self.lines.append(" " * self.indent_spaces + content)

    def trace_as_str(self, delimiter: Literal[".", "_"] = ".") -> str:
        """A string representation of this scope's relative trace.

        Follow the trace of the scope, and connect parent scopes with a delimiter.
        The root scope is not included in this trace string.

        Args:
            delimiter (Literal[".", "_"]): The delimiter to join the scope names with.
        """
        return delimiter.join(scope.name for scope in self.trace if (not scope.is_root) and (scope.name))

    def __repr__(self) -> str:
        """A string representation of this scope.

        Follow the path of scopes, and connect parent scopes with '.'.
        """
        return self.trace_as_str(".")


@dataclasses.dataclass
class CapnpType:
    """Represents a type that is extracted from a .capnp schema.

    Args:
        schema (Any):
        name (str):
        scope (Scope):
        generic_params (list[str]):
    """

    schema: Any
    name: str
    scope: Scope
    generic_params: list[str] = dataclasses.field(default_factory=list)

    @property
    def scoped_name(self) -> str:
        """Extract the name of a type, taking into account its containing scope.

        Returns:
            str: The scoped type name.
        """
        if not self.scope.is_root:
            return f"{self.scope}.{self.name}"

        else:
            return self.name
