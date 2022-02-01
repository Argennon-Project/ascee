#### Chunk

A chunk can be considered as a continuous array of bytes. Every chunk has a
size: `chunkSize` and a size upper bound :`sizeUpperBound`. The value
of `chunkSize` can be determined uniquely at the start of every execution
session, and it may be updated during the session like a normal memory location.
On the other hand `sizeUpperBound` is a value that is constant for every block
of the blockchain and is proposed by the block proposer.

The address space of the chunk starts from zero and only offsets lower
than `sizeUpperBound` (`offset < sizeUpperBound`)
are valid. Trying to access any offset higher
than `sizeUpperBound` (`offset >= sizeUpperBound)` will always result in a
revert.

The value of `chunkSize` at the **end of the execution session** will determine
if the memory location at an offset is persistent or not. Offsets lower than
this value (`offset < chunkSize`) are persistent, and offsets higher
than `chunkSize` (`offset >= chunkSize`) are not persistent. Non-persistent
locations will be re-initialized with zero, **at the start of every execution
session**.

Usually an application should not have any assumption about the value of memory
locations that are outside the chunk. While these locations are zero initialized
at the start of every execution session, it should be noted that multiple
invocations of an application may happen in a single execution session and if
one of them modifies a location outside the chunk, the changes can be seen by
next invocations.

There is no way for an application to query the chunk capacity. As a result in
the view of an application, accessing offsets higher than `chunkSize` results in
undefined behaviour, while the behaviour is well-defined for validators.

*Rationale: Validators can determine validity of an offset at the start of the
block validation and the procedure can be parallelized.*

While an application can use `chunkSize` to determine if an offset is persistent
or not, that is not considered a good practice. Reading `chunkSize`
decreases transaction parallelization, and should be avoided. Instead,
applications should use `is_valid` function for checking the persistence status
of memory addresses.

A chunk can be loaded using `load_chunk` function. This function never fails and
even non-existent chunks can be loaded by this function. For a non-existent
chunk the value of `chunkSize` is always zero.

Reading the value of `chunkSize` is like reading a normal memory location of the
chunk, and it needs to be pre-declared in the `resourseCap` field of the
request. On the other hand, for using `is_valid` function there is no need for
declaring `chunkSize` as a read location of the request. It should be noted that
a request still needs to declare access to the required chunk for
using `is_valid` function.

*Rationale: `is_valid(offset)` only checks the condition `offset < chunkSize`.
As a result, as long as modifying the chunk size does not change the result of
this check `Scheduler` can parallelize requests using `is_valid` with requests
that modify the chunk size.*

#### Chunk Resizing

The value of `chunkSize` can be modified during an execution session. However,
it can only be increased or decreased. More precisely, if a request has declared
that it wants to expand (shrink) a chunk it can only increase (decrease) the
value of `chunkSize` and any specified value for `chunkSize` during the
execution session, needs to be greater (smaller) than the size of the chunk at
the start of the session (`initialSize`).

Any request that wants to expand (shrink) a chunk needs to specify
a `maxSize` (`minSize`). The value of `chunkSize` can not be set higher (lower)
than that value. (`chunkSize > maxSize` is not allowed)

*Rationale:*

#### Chunk size bounds

A block proposer is required to propose two values for
**every** chunk that will be **resized** during a block: `sizeLowerBound`
, `sizeUpperBound`. These values must meet the following requirements:

- For every chunk expansion declaration we have `maxSize <= sizeUpperBound` for
  that chunk.
- For every chunk shrinkage declaration we have `minSize >= sizeLowerBound` for
  that chunk.
- For every chunk we
  have `chunkSize <= sizeUpperBound && chunkSize >= sizeLowerBound`,
  where `chunkSize`
  is the size of the chunk at the **end of the previous block**.

For non-existent chunks that are accessed but are not allocated, no bounds
should be proposed.

For chunks that there is no request for expanding them we
define `sizeUpperBound = sizeLowerBound = chunkSize`

*Rationale: A validator can use `sizeUpperBound` to allocate memory for chunks.
The scheduler uses `sizeLowerBound` to determine concurrent-safe memory
locations of a chunk. For validating chunk bounds there is no need for
calculating the maximum (minimum) of declared max (min) sizes,
and `maxSize <= sizeUpperBound` (`minSize >= sizeLowerBound`)
check suffices. This way, the process is a read-only process without any shared
memory and can be easily parallelized.*

**Collision detection:** We assume a request that wants to expand a chunk is a
writer for all offsets such that: `offset < maxSize && offset >= sizeLowerbound`
. And a request that wants to shrink a chunk is a writer
for: `offset < sizeUpperBound && offset >= minSize`

#### Page

Every page has a native chunk that has the same id as the page. In addition to
the native chunk, a page can host any number of migrant chunks.

When a page contains migrants, its native chunk can not be migrated. If the page
does not have any migrants, its native can be migrated and after that, in the
merkle tree the page will be converted into a special `<<moved>>` page.
(we don't need to implement this here) When a non-native chunk is migrated to
another page, it must be removed from the page. (it must NOT be replaced with a
zero size chunk.)

When a migrant chunk is removed, it will always be replaced with a zero size
chunk. When a native chunk is removed, if its page does not contain any migrants
the page will be converted into a special `<<nil>>` page. If the page contains
other chunks the native chunk will be replaced with a zero size chunk.

**Important results:**

- A zero size chunk proves non-existence of a chunk.
- A page containing a zero size native chunk can be safely converted into
  a `<<nil>>` page if it doesn't have migrants.

#### ArgC Types

ArgC has two kinds of types: primitive types and class types. Class types are
struct like types which always are allocated on the stack. All ArgC types have
lower case names and use snake_case naming convention. Class types always end
with `_c` suffix. There are clear differences between primitive and class types:

Primitive types can be passed by value or reference, class types are **always**
passed by reference.

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