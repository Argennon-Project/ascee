Chunk: A chunk can be considered as an unlimited continuous array of bytes.
Every chunk has a size: `chunkSize`. The address space of the chunk starts from
zero and memory offsets (addresses) below `chunkSize` (`offset < chunkSize`) are
persistent. On the other hand, offsets higher
than `chunkSize` (`offset >= chunkSize`) are not persistent and **at the start
of every execution session** they will be re-initialized with zero.

A chunk can be loaded using `load_chunk` function. This function never fails and
even non-existent chunks can be loaded by this function. A non-existent chunk
will always be loaded with `chunkSize == 0`.

While an application can use `chunkSize` to determine if an offset is persistent
or not, that is not considered a good practice. Reading `chunkSize`
decreases transaction parallelization, and should be avoided. Instead,
applications should use `is_valid` function for checking the persistence status
of memory addresses and use `chunk_exists` function for checking the existence
of a chunk.

Reading the value of `chunkSize` is like reading a normal memory location of the
chunk, and it needs to be pre-declared in the `resourseCap` of the request. On
the other hand using `is_valid` and `chunk_exists` functions do not need any
pre-declaration. However, they can only be used for already loaded chunks. (
chunks that a request has declared for loading.)

Chunk Resizing: The value of `chunkSize` can be modified in an execution
session. However, it can only be increased or decreased during the session. More
precisely, if a request has declared that it wants to expand (shrink) a chunk it
can only increase (decrease) the value of `chunkSize` and any specified value
for `chunkSize` during the execution session, needs to be greater (smaller) than
the previous value.

*rationale:*

Moreover, if a request wants to expand (shrink) a chunk it needs to pre-declare
an upper-bound (lower-bound), the value of `chunkSize` can not be set higher (
lower) than that value.

*rationale:*

Chunk Capacity: `chunkCapacity` is a value proposed by the block proposer, which
must meet the following requirements:

- For every chunk such that `sizeUpperBound > chunkSize` we
  have `chunkCapacity >= sizeUpperBound`
- For any chunk which is accessed by a request and `sizeUpperBound == 0` we
  have `chunkCapacity == 0` (i.e. for non-existent chunks that are accessed by
  at least one request `chunkCapacity` must be defined and be equal to zero.)

For chunks that there is no request for expanding them we
define `sizeUpperBound = chunkSize`

Rationale: *A validator can use `chunkCapacity` to both build the chunk index
and allocate memory for chunks. For validating chunk capacities there is no need
for calculating the maximum of upper bounds and `sizeUpperBound < chunkCapacity`
check suffices. This way, the process is a read-only process and can be easily
parallelized.*