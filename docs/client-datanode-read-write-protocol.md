# Client-Datanode Read-Write Protocol

## Overview

+ NameNode: A highly available metadata service with no single point of failure built on a persistent, consensus-backed KV store.

  - Responsibilities:
    - File-to-block mappings.
    - Block metadata: `block_id`, replica locations, and block status: in-progress (`UNFINALIZED`) or complete (`FINALIZED`).

  - Interfaces:
    - `AddBlock() returns (b,[d])`: Creates a new block, assigns DataNodes for replication, and returns the `block_id` (`b`) along with a list of assigned DataNode IDs (`[d]`).
    - `FinalizeBlock(b,length) returns success`: Finalizes a block with the specified length during normal operation.
    - `FinalizeBlock(b) returns (success,length)`: Used during recovery to finalize a block based on consistent data from DataNodes.
    - `GetBlock(b) returns ([d],is_finalized,length)`.

+ DataNode Stores block data as sequential chunks.
  - Interfaces:
    - `Write(b,c,data) returns success`: Writes `data` to chunk `c` of block `b`. Rejects the write if `c` is not the next expected chunk, enforcing sequential writes.
    - `Read(b,offset=0,max_length=-1) returns (gs,[c0,c1,...])`: Returns the highest-numbered generation it has participated in and the most recently written chunks, starting at `offset` and up to `max_length` bytes.

+ Client Interacts with NameNode and DataNodes to write and read data. The write workflow is as follows:

  - Call `AddBlock()` to obtain the block ID and DataNodes.
  - Write chunks sequentially (`c=0,1,...`) to all DataNodes.
  - On success: Call `FinalizeBlock(b,length)` to commit the block.

  - On failure:
    - Self-recovery: If a client fails to write a chunk, it can call `FinalizeBlock(b)` to finalize the previous block and start writing the next one.
    - External recovery: Another client or the NameNode can invoke `FinalizeBlock(b)` to recover orphaned blocks.

## Protocol Design Goals

The protocol aims to satisfy three key correctness properties:

### Agreement

Agreement ensures consistency by preventing data corruption or loss, even in the presence of client or DataNode failures.

1. Once a chunk is written, it cannot be lost or modified.
2. Only one finalized version of a block exists.

### Progress

Blocks are eventually finalized, even if clients fail or DataNodes crash. Without this guarantee, the system could stall indefinitely.

### Linearizability

Once a chunk is successfully written, it becomes immediately readable. Linearizability ensures strong consistency for reads, making the system predictable and easy to reason about.

## How It Works

This protocol is inspired by Paxos, a consensus protocol that ensures agreement even in the presence of partial failures. By adapting Paxos, the protocol achieves robust guarantees of agreement and progress while introducing sequential constraints to simplify the recovery process.

### Normal Write Process

1. The client sends a `NameNode.AddBlock()` request and waits for the response `(b,[d])`, which includes the block ID and a list of DataNodes for replication.
2. The client initializes its chunk ID to zero (`c.ChunkID(b)=0`) and increments it sequentially. For each chunk, the client sends `Write(b,gs=0,c=c.ChunkID(b),data)` to all DataNodes in `[d]`.
3. When a DataNode `d` receives `Write(b,gs,c,data)` from the client:
   1. It checks the following conditions:
      1. The write is part of the initial generation (`gs == 0`).
      2. The DataNode has not processed any higher generations for this block (`d.MaxGS(b) == 0`).
      3. The chunk ID matches expected value (`d.ChunkID(b) == c`).
   2. If all conditions are satisfied:
      1. It updates `d.ChunkID(b)`, `d.VotedGS(b,c)`, and stores `data` in `d.VotedData(b,c)`.
      2. It sends a `Vote(d,b,gs,c)` response to the client.
4. The client waits for `success=true` responses from all DataNodes in `[d]` before moving to the next chunk.
5. After writing all chunks, the client sends a `NameNode.FinalizeBlock(b,length)` request.
6. If the NameNode responds with `success=true`, the block is successfully written.

### Failure Recovery Process

1. If a chunk write times out, the client calls `NameNode.FinalizeBlock(b)` without specifying a block length.
2. Upon receiving `FinalizeBlock(b)`, the NameNode increments its `n.GS(b)` and sends `NextGS(b,gs)` to all DataNodes storing the block.
3. Upon receiving `NextGS(b,gs)`, DataNode `d` updates `d.MaxGS(b)` to `gs` if `gs>d.MaxGS(b)` and responds with `LastVote(d,b,gs,c,vgs,vdata)`.
4. The NameNode waits for `LastVote` responses from a majority quorum of DataNodes. If a quorum is reached, it determines the data to propose:
   - Chooses the chunk with the highest-numbered `c` , then choose the highest-numbered `vgs` and the corresponding `vdata` for that chunk.
   - If no valid data exists, it finalized the block with zero length.
5. The NameNode sends `write(b,gs=n.GS(b),c,vdata)` to all DataNodes.
6. Upon receiving `Write(b,gs,c,vdata)`, DataNode `d` processes the request as follows: 
   1. If `d.MaxGS(b)==gs && (c>=d.ChunkID(b)-1 && c<=d.ChunkID(b)+1)` 
      1. Updates `d.ChunkID(b)` to `max(c,d.ChunkID(b))`, sets `d.VotedGS(b,c)` to `gs`, and stores `vdata` in `d.VotedData(b,c)`.
      2. Responds with `Vote(d,b,gs,c)`.
7. The NameNode finalizes the block after receiving `Vote` responses from a majority quorum and returns the block's length to the client.

### Reading Unfinalized Blocks

1. The client sends a `GetBlock(b)` request to the NameNode.
2. The NameNode responds with `([d],is_finalized=false,length)`.
3. The client sends a `Read(b)` request to all DataNodes in `[d]`.
4. After receiving responses `(gs,[c0,c1,...])` from the DataNodes:
   1. If all `gs` values are `0`, the client determines the chunk to read by selecting the smallest highest-numbered voted chunk ID across all DataNodes. For example, if `d1` returns `(gs=0,[c0,c1])`, `d2` returns `(gs=0,[c0,c1,c2])`, and `d3` returns `(gs=0,[c0,c1,c2])`, then `c1` is selected.
   2. If any `gs` value is not `0`, the client switches to reading the finalized block (see the finalized block reading process below).

### Reading Finalized Blocks

1. The client sends a `GetBlock(b)` request to the NameNode.

2. The NameNode responds with `([d],is_finalized=true,length)`.

3. The client sends a `Read(b)` request to one of the DataNodes in `[d]`.

4. Upon receiving the response, the client verifies the block’s length:

   - If the length matches the value provided by the NameNode, the client returns the block.

   - If the length does not match, the client retries with the next DataNode in `[d]`.

## Why This Is Correct

To ensure correctness, this protocol adapts Paxos and introduces additional constraints. Below, we map the protocol terminology to Paxos and demonstrate how it satisfies Agreement, Progress, and Linearizability.

| Protocol Terminology           | Paxos Terminology           | Description                                                  |
| ------------------------------ | --------------------------- | ------------------------------------------------------------ |
| `BlockInputStream`             | A series of Paxos instances | A sequence of chunks within the same block.                  |
| Chunk                          | Paxos instance              | When a user calls `flush` or the batch size reaches a threshold, the client sends the data as a chunk. Each chunk must achieve consensus among DataNodes. |
| Generation                     | Round                       |                                                              |
| Initial Generation             |                             | The generation where `stamp = 0`. The client acts as the proposer to propose data directly to DataNodes. |
| Recovery Generation            |                             | The generation where `stamp > 0`. The NameNode acts as the proposer. |
|                                | Proposer                    |                                                              |
| DataNode                       | Acceptor                    | Votes on proposed chunks and stores the data.                |
|                                | Learner                     | Learns the final value during normal operations or recovery. |
| `d.ChunkID(b)`                 |                             |                                                              |
| `d.MaxGS(b)`                   | `val(a)`                    | The highest-numbered generation in which the DataNode `d` has participated for block `b`. |
| `d.VotedGS(b,c)`               | `vrnd(a)`                   | The highest-numbered generation in which the DataNode `d` has cast a vote for block `b` chunk `c`. |
| `d.VotedData(b,c)`             | `vval(a)`                   | The chunk data that the DataNode `d` voted to accept in `d.VotedGS(b,c)`. |
| `n.GS(b)`                      | `crnd(c)`                   | The highest-numbered generation that the NameNode has initiated for block `b`. |
| `n.Data(b,c)`                  | `cval(c)`                   | The chunk data that the NameNode has picked for block `b` chunk `c` in generation `n.GS(b,c)`. |
| `NextGS(b,gs)`                 | Phase 1a message            | A prepare message sent by the NameNode to DataNodes, requesting their participation in generation `gs` for block `b`. |
| `LastVote(d,b,gs,c,vgs,vdata)` | Phase 1b message            | A response from DataNode `d` to the proposer, containing their last vote generation `vgs` and data `vdata` of last chunk `c`. |
| `Write(b,gs,c,data)`           | Phase 2a message            | A message sent by the client or the NameNode to DataNodes, requesting them to vote on the proposed data. |
| `Vote(d,b,gs,c)`               | Phase 2b message            | A message from a DataNode `d` announcing its vote for the proposed value in the given generation `gs`. |

### Terminology Used in Proofs

**Register**: For each chunk `c`, each DataNode `d`, and each generation `gs`, there exists exactly one **write-once** register. This register represents a **DataNode's immutable vote record** for the specific chunk `c` in the given generation `gs`.

**Quorum**: A quorum is a set of DataNodes whose votes are sufficient to determine the value of a chunk in a given generation `gs`. For a chunk `c` to be decided in generation `gs`, a quorum of DataNodes must agree on the same data.

**Decided Chunk**: A chunk `c` is considered decided if the same data has been voted on by a quorum in a given generation.

### Proofs for Agreement

#### Proofs for Chunks `0` to `c-2`

+ **Claim**: These chunks are decided in the initial generation (`gs=0`), and recovery generations (`gs>0`) will neither repropose nor redetermine their values.

+ Reasoning:

  + **Client-Enforced Sequential Writes**: The client writes chunks strictly in sequential order (`c=0,1,...`). A chunk `c` can only be proposed if chunk `c-1` has already been decided.

  + **DataNode Reporting**: During recovery, each DataNode reports only its highest-numbered voted chunk (`c'`) using `LastVote(d,b,gs,c',vgs,vdata)`. Due to client-enforced sequential writes, the highest-numbered chunk on any DataNode must be either chunk `c-1` or chunk `c`. Therefore, it holds that `c'>=c-1` for all DataNodes.

  + **Recovery Scope Determined by NameNode Behavior**: The NameNode collects `LastVote` responses from a quorum of DataNodes and identifies the chunk with the highest-numbered `chunk_id` to recover. Since no DataNode in the quorum reports chunks with `chunk_id` less than `c-1`, the NameNode focuses recovery efforts exclusively on chunk `c-1` or chunk `c`.

#### Proofs for Chunks `c-1` and `c`

+ **Claim**: These chunks are resolved using a Paxos-like consensus algorithm, guaranteeing that at most one value is chosen.

+ Reasoning:

  + **Current Decision (Within a Generation)**:
    + **Quorum Overlap**: Any two quorums in the same generation intersect at least one DataNode.
    + **Immutable Registers**: A DataNode votes at most once per generation.
    + Together, this guarantees that at most one piece of data can be decided per generation.

  + **Previous Decisions (Across Generations)**: The NameNode can propose data for generation `gs` only if it ensures no conflicting data can be decided in earlier generations (`stamp < gs`).
    + When the NameNode initiates recovery with `NextGS(b,gs)`:
      + **Fence**: DataNodes update `d.MaxGS(b) = gs`, blocking writes to generations less than `gs`.
      + **Collect and Analyze**: The NameNode collects `LastVote` responses from a quorum of DataNodes, identifies the highest-voted chunk `c'`, and selects the highest-numbered generation `vgs` associated with chunk `c'`.
        + If chunk `c` does not appear in the responses, it has not been voted on by a quorum in any generation. The NameNode can safely finalize the block, excluding chunk `c`.
        + If chunk `c` appears with the highest-numbered voted generation `vgs`:
          + For generations `[0,vgs)`, the proposer of `vgs` guarantees that no conflicting data can be decided.
          + In generation `vgs`, only one piece of data is proposed and can be decided.
          + For generations `(vgs,gs)`, no quorum votes for any data, so no data can be decided.

#### Proofs for Chunks `c+1` and Beyond

+ **Claim**: Chunks `c+1` and beyond remain undecided and are invisible during recovery.
+ Inductive Argument Reasoning:
  - Base Case: In the initial generation (`gs=0`), chunk `c+1` does not appear due to client-enforced sequential writes.
  - Inductive Step:
    - Assume that for generations `0,1,...,gs-1`, chunk `c+1` does not appear.
    - For generation `gs`, chunk `c+1` also does not appear. This follows from dataNode reporting and recovery scope determined by nameNode behavior.

### Proofs for Linearizable

Linearizability is a correctness property for concurrent systems. It satisfies two key properties:

+ **Atomicity**: Every operation appears to take place at a single, indivisible moment in time.
+ **Real-Time Ordering**: The operations are consistent with the real-time order in which they occur. If operation A completes before operation B begins, then B should logically take effect after A.

The moment of a write request for a chunk is defined as: **The moment when the chunk data is first decided by a quorum of votes.** This is the first time the chunk transitions from an undecided state to a decided state. Once a chunk is decided, it becomes immutable and visible to subsequent operations.

The read request's linearizability is analyzed in the following sections.

#### Case 1: Recovery Generation is Found

+ **Definition of Moment**: The moment of the read request is defined as the time when the block is finalized. Let `finalized` be the time when the NameNode confirms that the block is finalized. Then: `max(invocation, finalized) <= moment <= completion`.

+ **Proof of Moment Existence**:
  + `invocation <= completion`: The read request must be invoked before it completes.
  + `finalized <= completion`: The client waits for the block to be finalized before completing the read.

Thus, the read request has a well-defined atomic moment.

#### Case 2: No Recovery Generation Found

+ **Definition of Moment**: Define the atomic moment as `moment`, such that  `max(invocation, c is decided) <= moment <= min(completion, c+1 is decided)`.
+ **Proof of Moment Existence**:
  + `invocation <= completion`: The read request must be invoked before it completes.
  + `invocation <= c+1 is decided`: By contradiction:
    + Assume `c+1 is decided < invocation`.
    + If `c+1` had been decided before the read's invocation, the minimum of the highest-numbered chunk IDs across all DataNodes would be `>= c+1`, contradicting the client’s observation of `c` as the highest-numbered chunk.
  + `c is decided <= completion`: The client only reads chunk `c` after confirming it is decided by all DataNodes.
  + `r is decided < r+1 is proposed < r+1 is decided`: Guaranteed by client-enforced sequential writes.
+ **Real-Time Ordering**: The timeline [write chunk 0, write chunk 1, ..., write chunk c, read, write chunk c+1] respects real-time ordering.

## Reference

+ [Fast Paxos](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/tr-2005-112.pdf)
+ [A Generalised Solution to Distributed Consensus](https://arxiv.org/pdf/1902.06776)
+ [Linearizable Quorum Reads in Paxos](https://www.usenix.org/system/files/hotstorage19-paper-charapko.pdf)
