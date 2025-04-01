# Data Structures for Managing Directory Trees in Key-Value Databases

## The Design Goals

The goals of designing a data structure for storing directory trees in a key-value database are as follows, ordered by priority from high to low:

1. **Functionality**: The data structure must support the full range of features provided by POSIX, HDFS, and S3. For instance, not all data structures can handle hard links.
2. **Efficiency**: The data structure should minimize the number of key-value database operations required to complete a user request. This is especially important for **frequently used read operations**, which dominate workloads in file systems and object stores. Reducing operation overhead ensures high performance and low latency.
3. **Normalization**: Strive to comply with **BCNF** to reduce data duplication.

## The API Requirements of libFUSE

When the Linux kernel receives a request involving a path (e.g., `touch /a/b/c` from bash), [it performs path resolution itself](https://github.com/torvalds/linux/blob/7d06015d936c861160803e020f68f413b5c3cd9d/fs/namei.c#L2418). It then forwards only two types of requests to libFUSE for locating the target object:

1. APIs targeting directory entries: These take a parent inode ID and a child name as input and ask libFUSE to return properties of the corresponding sub-inode (e.g., its inode ID or attributes).
2. APIs targeting inodes: These take an inode ID as input and ask libFUSE to return properties of the inode (e.g., metadata like attributes, permissions, or file content).

The following tables list operations for each type. APIs are formatted as `api(input,...):(output,...)`. For example, `lookup(parent,name):(ino,attr)` indicates the `lookup` API takes `parent` and `name` as inputs and returns `ino` and `attr` as outputs. `ino` is the inode ID. `attr` is a `struct stat` object that contains metadata such as the inode ID, UID, GID, permissions, and timestamps (atime, mtime, ctime).

| APIs targeting directory entries           |
| ------------------------------------------ |
| `lookup(parent,name):(ino,attr)`           |
| `mknod(parent,name):(ino,attr)`            |
| `unlink(parent,name):()`                   |
| `mkdir(parent,name):(ino,attr)`            |
| `rmdir(parent,name):()`                    |
| `link(ino,newparent,newname):(ino,attr)`   |
| `rename(parent,name,newparent,newname):()` |

| APIs targeting inodes             |
| --------------------------------- |
| `getattr(ino):(attr)`             |
| `setattr(ino,attr):()`            |
| `open(ino):(ino,attr,fh,flags)`   |
| `release(ino):()`                 |
| `read(ino,size,off):(data)`       |
| `write(ino,buf,size,off):(count)` |

## The API Requirements of HDFS

The APIs in [ClientNamenodeProtocol.proto](https://github.com/apache/hadoop/blob/5ccb0dc6a91cf0d98a6b8b9cffc17f361faced6d/hadoop-hdfs-project/hadoop-hdfs-client/src/main/proto/ClientNamenodeProtocol.proto) can be categorized into two types:

1. APIs targeting paths: These take a path as input.
2. APIs targeting both paths and inode IDs: These take both a path and inode IDs as input.

| APIs targeting paths                                |
| --------------------------------------------------- |
| `getBlockLocations(src,offset,length):([location])` |
| `create(src):()`                                    |
| `rename(src,dst):()`                                |
| `delete(src):()`                                    |
| `getListing(src,startAfter):([dir])`                |

| APIs targeting both paths and inode IDs   |
| ----------------------------------------- |
| `addBlock(src,clientName,fileId):(block)` |
| `abandonBlock(b,src,fileId):()`           |
| `fsync(src,fileId):()`                    |
| `complete(src,fileId):()`                 |

The API requirements of HDFS differ from those of libFUSE in two key aspects:

- In HDFS, the NameNode performs path resolution internally. It receives the src parameter, which is a path string, as input and handles the resolution itself.
- HDFS does not support hard links.

## Designing SQL Tables to Represent a Directory Tree Structure

### Hard Links and BCNF Normalization

POSIX supports hard links for files but does not allow hard links for directories. If we record directories in a `DirTable` and files in a `WideFileTable`, this limitation impacts normalization: while the `DirTable` can be expressed in BCNF, the `WideFileTable` cannot.

Consider a directory tree where the root inode ID is 1, and /a/c, /a/d are hard links to /a/b, meaning they all share the same inode:

```text
root (inode 1)
|- a (inode 2)
   |- b (inode 3)
   |- c (inode 3) [hard link to /a/b]
   |- d (inode 3) [hard link to /a/b]
   |- e (inode 4)
```

We can represent this structure in a `WideFileTable` that includes all fields required by libFUSE APIs:

| Parent Inode ID | Name | Inode ID | Attr |
| --------------- | ---- | -------- | ---- |
| 2               | b    | 3        | y    |
| 2               | c    | 3        | y    |
| 2               | d    | 3        | y    |
| 2               | e    | 4        | z    |

### Consequences of BCNF Violation

The above table does not follow BCNF because of the following reasons:

- Since /a/b, /a/c, and /a/d are hard links, they share the same inode ID and therefore the same attr. This creates a functional dependency: Inode ID -> Attr. That is, given an inode ID, there is precisely one attr associated with it.
- The combination of parent inode ID and name uniquely determines the inode ID: (Parent Inode ID, Name) -> Inode ID.
- There is a transitive dependency: (Parent Inode ID, Name) -> Inode ID -> Attr.

Violating BCNF causes data duplication and write amplification. For example, if the attribute of inode 3 changes, three rows must be updated to maintain consistency, leading to write amplification.

### Normalizing `WideFileTable` into BCNF

To address these issues, we can normalize the `WideFileTable` into two separate tables – `FileTable` and `HardLinkTable` – to comply with BCNF:

`FileTable` stores unique inode IDs and their attributes:

| Inode ID | Attr |
| -------- | ---- |
| 3        | y    |
| 4        | z    |

`HardLinkTable` stores the mapping of parent inode IDs and names to inode IDs:

| Parent Inode ID | Name | Inode ID |
| --------------- | ---- | -------- |
| 2               | b    | 3        |
| 2               | c    | 3        |
| 2               | d    | 3        |
| 2               | e    | 4        |

### Final Schema

In conclusion, to model a directory tree that properly supports hard links while complying with BCNF, we need three tables: `DirTable`, `FileTable`, and `HardLinkTable`. Additionally, the common fields of `DirTable` and `HardLinkTable` can be unified into a `DirEntryView`, which represents directory entries within a parent directory. The schema is as follows:

```sql
CREATE TABLE DirTable (
  parent_id INTEGER NOT NULL,
  name VARCHAR(255) NOT NULL,
  -- The id serves as a candidate key.
  id INTEGER NOT NULL,
  attr TEXT NOT NULL,
  PRIMARY KEY (id),
  -- The combination of (parent_id, name) also serves as a candidate key.
  UNIQUE (parent_id, name),
  INDEX (parent_id, name),
  FOREIGN KEY (parent_id) REFERENCES DirTable(id)
);

CREATE TABLE FileTable (
  id INTEGER NOT NULL,
  attr TEXT NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE HardLinkTable (
  parent_id INTEGER NOT NULL,
  name VARCHAR(255) NOT NULL,
  id INTEGER NOT NULL,
  PRIMARY KEY (parent_id, name),
  FOREIGN KEY (parent_id, name) REFERENCES DirTable(parent_id, name),
  FOREIGN KEY (id) REFERENCES FileTable(id)
);

-- The combination of (parent_id, name) serves as a primary key.
CREATE VIEW DirEntryView AS
SELECT
  parent_id,
  name,
  id
FROM
  DirTable
UNION ALL
SELECT
  parent_id,
  name,
  id
FROM
  HardLinkTable
```

## Designing Key-Value ColumnFamilies to Represent a Directory Tree Structure

A column family in a KV database is analogous to a table in an SQL database. For example: `DirTable` becomes `DirColumnFamily`, `FileTable` becomes `FileColumnFamily`, and `HardLinkTable` becomes `HardLinkColumnFamily`.

When transitioning from an SQL-based engine to a KV engine to represent a directory tree structure, there are fundamental differences between the two systems that influence the design. Below, we outline these differences and the resulting design decisions for the KV-based implementation.

- KV engines only support keys as the primary index and do not provide secondary indexes. As a result:
  - We need to create a `DirIndexColumnFamily` (key = `parent_id` + `name`, value = `id`) to serve as a secondary index.
  - This results in two lookup operations when querying a directory by `parent_id` and `name`:
    - One lookup would involve querying `DirIndexColumnFamily` with `parent_id` + `name` to retrieve the `id`.
    - A second lookup would involve querying `DirColumnFamily` with the retrieved `id` to fetch the directory details.
    - To avoid this inefficiency, we redundantly store the complete directory fields in both `DirColumnFamily` and `DirIndexColumnFamily`. Prioritize efficiency by sacrificing normalization and introducing redundancy.
- KV engines store values as binary arrays and do not enforce a strong schema or data types.
  - Since `DirColumnFamily` and `FileColumnFamily` have the same key format (key = `id`), they can be combined into a single column family, `InodeColumnFamily`.
  - Similarly, since `DirIndexColumnFamily` and `HardLinkColumnFamily` share the same key format (key = `parent_id` + `name`), they can be combined into a single column family, `DirEntryColumnFamily`. Instead of expressing directory entries as a "view" (as in SQL), they are represented as a real column family in the KV engine.

Consider the following example:

```text
root (inode 1)
|- a (inode 2)
   |- b (inode 3)
   |- c (inode 3) [hard link to /a/b]
   |- d (inode 3) [hard link to /a/b]
   |- e (inode 4)
```

The `InodeColumnFamily` would be structured as follows:

| Key (Inode ID) | Value                                       |
| -------------- | ------------------------------------------- |
| 1              | `{type:dir,parent_id:1,name:,id:1,attr:w}`  |
| 2              | `{type:dir,parent_id:1,name:a,id:2,attr:x}` |
| 3              | `{type:file,attr:y}`                        |
| 4              | `{type:file,attr:z}`                        |

The `DirEntryColumnFamily` would be structured as follows:

| Key (Parent Inode ID, Name) | Value                                       |
| --------------------------- | ------------------------------------------- |
| 1,a                         | `{type:dir,parent_id:1,name:a,id:2,attr:x}` |
| 2,b                         | `{id:3}`                                    |
| 2,c                         | `{id:3}`                                    |
| 2,d                         | `{id:3}`                                    |
| 2,e                         | `{id:4}`                                    |

## Reference

- [Wikipedia: Functional dependency](https://en.wikipedia.org/wiki/Functional_dependency)
- [GeeksforGeeks: Types of Functional dependencies in DBMS](https://www.geeksforgeeks.org/types-of-functional-dependencies-in-dbms/)
- [GeeksforGeeks: Number of Possible Super Keys in DBMS](https://www.geeksforgeeks.org/number-of-possible-superkeys-in-dbms/)
- [GeeksforGeeks: Candidate Key in DBMS](https://www.geeksforgeeks.org/candidate-key-in-dbms/)
- [Wikipedia: Third normal form](https://en.wikipedia.org/wiki/Third_normal_form)
- [Wikipedia: Boyce–Codd normal form](https://en.wikipedia.org/wiki/Boyce%E2%80%93Codd_normal_form)
- [GeeksforGeeks: Boyce-Codd Normal Form (BCNF)](https://www.geeksforgeeks.org/boyce-codd-normal-form-bcnf/)
- [FUSE: include/fuse_lowlevel.h](https://github.com/libfuse/libfuse/blob/a25fb9bd49ef56a2223262784f18dd9bbc2601dc/include/fuse_lowlevel.h)
- [Apache Hadoop: hadoop-hdfs-project/hadoop-hdfs-client/src/main/proto/ClientNamenodeProtocol.proto](https://github.com/apache/hadoop/blob/5ccb0dc6a91cf0d98a6b8b9cffc17f361faced6d/hadoop-hdfs-project/hadoop-hdfs-client/src/main/proto/ClientNamenodeProtocol.proto)
