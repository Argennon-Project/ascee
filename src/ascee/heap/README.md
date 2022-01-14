Chunk: A chunk can be considered as a continuous array of bytes. Every chunk has
a size: `chunkSize` and a size upper bound :`sizeUpperBound`. The value
of `chunkSize` can be determined uniquely at the start of every execution
session, and it may be updated during the session like a normal memory location.
On the other hand `sizeUpperBound` is a value that is constant for every block
of the blockchain and is proposed by the block proposer.

The address space of the chunk starts from zero and only offsets lower
than `sizeUpperBound` (`offset < sizeUpperBound`)
are valid. Trying to access any offset higher
than `sizeUpperBound` (`offset >= sizeUpperBound)` will always result in a
revert.

The value of `chunkSize` at the **end** of the execution session will determine
if the location at an offset is persistent or not. Offsets lower than this
value (`offset < chunkSize`) are persistent. On the other hand, offsets higher
than `chunkSize` (`offset >= chunkSize`) are not persistent and **at the start
of every execution session** they will be re-initialized with zero.

There is no way for an application to query the chunk capacity. As a result in
the view of an application, accessing offsets higher than `chunkSize` results in
undefined behaviour while the behaviour is well-defined for a validator.

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
chunk, and it needs to be pre-declared in the `resourseCap` of the request. On
the other for using `is_valid` function there is no need for
declaring `chunkSize` in the read locations of a request. It should be noted
that for using `is_valid` a request still needs to declare access to the
required chunk.

*Rationale: `is_valid(offset)` only checks the condition `offset < chunkSize`.
Therefore, as long as the result of the condition is the same scheduler can
parallelize requests that are using `is_valid` function with requests that
modify the chunk size.*

Chunk Resizing: The value of `chunkSize` can be modified in an execution
session. However, it can only be increased or decreased during the session. More
precisely, if a request has declared that it wants to expand (shrink) a chunk it
can only increase (decrease) the value of `chunkSize` and any specified value
for `chunkSize` during the execution session, needs to be greater (smaller) than
the size of the chunk at the start of the session (`initialSize`).

Any request that wants to expand (shrink) a chunk needs to specify
a `maxSize` (`minSize`). The value of `chunkSize` can not be set higher (lower)
than that value. (`chunkSize > maxSize` is not allowed)

*Rationale:*

Chunk's size bounds: A block proposer is required to propose two values for
**every chunk that will be resized** during a block: `sizeLowerBound`
, `sizeUpperBound`. These values must meet the following requirements:

- For every chunk expansion declaration we have `maxSize <= sizeUpperBound` for
  that chunk.
- For every chunk shrinkage declaration we have `minSize >= sizeLowerBound` for
  that chunk.
- For every chunk we
  have `chunkSize <= sizeUpperBound && chunkSize >= sizeLowerBound`. `chunkSize`
  is the size of the chunk at the end of the previous block.

For non-existent chunks that are accessed but are not allocated, no bounds
should be proposed.

For chunks that there is no request for expanding them we
define `sizeUpperBound = sizeLowerBound = chunkSize`

*Rationale: A validator can use `sizeUpperBound` to allocate memory for chunks.
The scheduler uses `sizeLowerBound` to determine concurrent-safe memory
locations of a chunk. For validating the chunk bounds there is no need for
calculating the maximum (minimum) of declared max (min) sizes,
and `maxSize <= sizeUpperBound` (`minSize >= sizeLowerBound`)
check suffices. This way, the process is a read-only process and can be easily
parallelized.*

Collision detection: