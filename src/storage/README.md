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