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