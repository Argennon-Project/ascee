#### ArgC Types

ArgC has two kinds of types: primitive types and class types. Class types are
struct like types which always are allocated on the stack. All ArgC types have
lower case names and use snake_case naming convention. Class types always end
with `_c` suffix. There are clear differences between primitive and class types:

Primitive types are always passed by value, class types are **always** passed by
reference.

A function can return a primitive type, but it can not return a class type.

```C
string_c convert(int32 x);    // compiler error!
```

Assignment operator `=` is not defined for class types.

All ArgC types must be explicitly initialized. (primitive or class type)

A class type can be initialized by either a constructor or a factory method:

```C
string_c str("constructor");           // initialization using a constructor
string_c str = to_string(msg_buffer);  // initialization using a factory method
```

All ArgC types (primitive or class) are allocated on stack and their lifetime
ends at the end of the scope in which they are defined. 