# SE3331 Computer System Engineering

## Course Overview

This course, SE3331, dives deep into the fundamentals and advanced topics of computer system engineering. It covers a wide array of subjects essential for understanding the complexities and intricacies of modern computing systems.

## Topics Covered

- Basic (inode-based) / Distributed File System
- Key-Value Store/Storage (KVS)
- Consistency: from Strict to Eventual
- Atomicity: All-or-Nothing, Before-or-After
- Paxos and Raft Consensus Algorithms
- Network: Routing, DNS, and P2P Network
- Distributed Computing: MapReduce and DAG Computation Graph
- Security: Control Flow Integrity (CFI), Return Oriented Programming (ROP)

## Labs Overview

### Lab 1: Inode Based Basic File System 
[Read more](./docs/lab1/lab1.md)

In this project, I successfully developed a basic single-machine inode-based filesystem. My work encompassed three distinct layers:

1) **Block Layer**: Manages data blocks, including allocating/deallocating (create/remove), and reading/writing data to these blocks.
2) **Inode Layer**: Handles files and directories, covering tasks like allocating/deallocating inodes (create/remove), managing the superblock (store filesystem information), and handling inode operations (read/write).
3) **Filesystem Layer**: Defines file and directory operations, such as creating and deleting files/directories, and reading/writing file content.


### Lab 2: Distributed File System
[Read more](./docs/lab2/lab2.md)

Building upon the foundations of Lab 1, I extended the single-machine filesystem into a robust distributed filesystem. The key features of this lab are:

* **Distributed Architecture**:
    
    The filesystem consists of three main components:
    1) Filesystem Client: Issues RPCs (Remote Procedure Calls) to other servers.
   2)  Metadata Server: Maintains all file system metadata, handling operations like file creation/deletion and querying block positions.
    3) Data Server: Stores file data blocks and manages read/write operations.

* **File Splitting**:
Files are divided into blocks stored across different data servers. The metadata server tracks each block's location and block ID.
* **Concurrency and Atomicity**:
    Implementation of a lock manager in the metadata server to handle concurrent requests and ensure atomicity in metadata operations.

* **Crash Recovery**:
    Incorporates a log manager in the metadata server to enable recovery from crashes, ensuring all-or-nothing atomicity of metadata operations.


### Lab 3: Implementation of Raft
[Read more](./docs/lab3/raft.md)

I successfully tackled the implementation of the Raft consensus algorithm within a distributed filesystem. My work can be summarized across four key areas:

* **Leader Election and Heartbeat Mechanism**:
Developed a robust system for electing a leader among server nodes and maintaining leadership through regular heartbeats.

* **Log Replication Protocol**: Engineered a protocol to replicate logs across servers, ensuring consistent state and order of commands.

* **Persistence of Raft Log and Snapshot Mechanism** implementations are not completed yet.  

This implementation not only strengthen my understanding of distributed systems but also honed my skills in asynchronous programming, RPC handling, and persistent data management.

### Lab 4: Implementation of MapReduce
[Read more](./docs/lab4/mr.md)

In Lab 4, I achieved the development of a MapReduce framework built upon a distributed filesystem. Key accomplishments include:

- **Sequential MapReduce**: Execution of a sequential MapReduce process, focusing on word count tasks.
- **Distributed MapReduce Framework**:
  - Coordinator process for efficient task distribution and management of worker processes.
  - Worker processes responsible for performing Map and Reduce tasks, utilizing the distributed filesystem for all file-related operations.

This lab  significantly advanced my expertise in distributed systems and large-scale data processing algorithms.

## Personal Insights

These labs were particularly challenging, with test cases designed to cover all important edge cases. The practical experience of implementing these systems yourself reveals the complexity of the underlying technologies. Despite the challenges, the learning outcome and the final product are truly remarkable.

## Acknowledgements

A special thanks to the faculty and teaching assistants at SJTU for their guidance and support throughout these courses. Their dedication has been instrumental in providing a comprehensive and practical understanding of computer systems and their practical implementations.

