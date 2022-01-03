Chunk: A chunk can be considered as a continuous array of bytes. Every chunk has
a size: `chunkSize` and a capacity: `capacity`. The address space of the chunk
starts from zero and only positive offsets lower
than `chunkCapacity` (`offset < capacity`) are valid. Trying to access any
offset higher than `capacity` (`offset >= capacity)` will result in a revert.
Only offsets below `chunkSize` (`offset < chunkSize`) are persistent. Offsets
higher than `chunkSize` (`offset >= chunkSize`) are not persistent and **at the
start of every execution session** they will be re-initialized with zero.

As we will see later, the chunk capacity for expandable chunks can be different
for each block and is proposed by the leader. There is no way for an application
to query the chunk capacity.

*Rationale: Validators can determine validity of an offset at the start of the
block validation and the procedure can be parallelized.*

A chunk can be loaded using `load_chunk` function. This function never fails and
even non-existent chunks can be loaded by this function. For a non-existent
chunk the value of `chunkSize` is zero.

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

*Rationale:*

Chunk Resizing: The value of `chunkSize` can be modified in an execution
session. However, it can only be increased or decreased during the session. More
precisely, if a request has declared that it wants to expand (shrink) a chunk it
can only increase (decrease) the value of `chunkSize` and any specified value
for `chunkSize` during the execution session, needs to be greater (smaller) than
the size of the chunk at the start of the session (`initialSize`).

*Rationale:*

Moreover, if a request wants to expand (shrink) a chunk it needs to pre-declare
an upper-bound (lower-bound), the value of `chunkSize` can not be set higher (
lower) than that value.

*Rationale:*

Chunk Capacity: `chunkCapacity` is a value proposed by the block proposer, which
must meet the following requirements:

- For every chunk such that `sizeUpperBound > chunkSize` we
  have `chunkCapacity >= sizeUpperBound`
- For any chunk which is accessed by a request and `sizeUpperBound == 0` we
  have `chunkCapacity == 0` (i.e. for non-existent chunks that are accessed by
  at least one request `chunkCapacity` must be defined and be equal to zero.)

For chunks that there is no request for expanding them we
define `sizeUpperBound = chunkSize`

*Rationale: A validator can use `chunkCapacity` to both build the chunk index
and allocate memory for chunks. For validating chunk capacities there is no need
for calculating the maximum of upper bounds and `sizeUpperBound < chunkCapacity`
check suffices. This way, the process is a read-only process and can be easily
parallelized.*