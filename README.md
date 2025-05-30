# HTTP Forward Caching Proxy

This project is an HTTP forward proxy written in C. The proxy sits between a client and an origin server, forwarding HTTP `GET` requests and caching server responses using configurable cache replacement policies. Builds upon [Multithreaded Server](https://github.com/Avtesh29/Multithreaded-Server).

---

## 📌 Overview

An HTTP forward proxy receives requests from clients and forwards them to remote servers on the client’s behalf. To improve performance and reduce load on remote servers, this proxy supports two caching policies:

- **FIFO (First-In First-Out)**
- **LRU (Least Recently Used)**

---

## 💡 Technologies and Concepts

**Technologies:** `C`, `Socket Programming`, `POSIX Threads`, `GCC`/`Clang`, `Makefile`, `File Descriptors`, `HTTP/1.1`, `getopt`, `regex.h`, `Dynamic Memory Allocation`

**Concepts:** `Forward Proxying`, `HTTP Parsing`, `Header Rewriting`, `In-Memory Caching`, `FIFO/LRU Eviction`, `Cache Limits`, `Response Injection`, `Error Handling`, `CLI Validation`

---

## 🚀 Features

- Accepts and forwards well-formed `GET` requests
- Parses HTTP/1.1 request headers
- Maintains an in-memory cache of server responses
- Adds a `Cached: True` header for cache hits
- Supports two caching algorithms (FIFO and LRU)
- Handles cache eviction when full
- Proper error reporting and argument validation
- Clean shutdown and minimal memory usage

---

## 🧠 How It Works
1. Accepts a client connection
2. Parses the request (expects format: GET http:<area>//host:port/uri HTTP/1.1\r\n\r\n)
4. Checks cache:
   - If hit: returns cached response with Cached: True header
   - If miss: connects to destination server, forwards request, stores and returns response
5. Eviction:
   - If cache is full, evicts according to FIFO or LRU
6. Returns the response to the client

---

## 🏗️ Implementation

### 🗃️ Cache

The caching functionality for the HTTP Forward Proxy is implemented in `cache.c` and `cache.h`. These files define and manage the in-memory cache used to store server responses, enforcing size constraints and supporting cache eviction policies. It is a fully associative cache which utilizes temporal locality.

### 📦 Data Structures

In terms of Data Structures, the cache is implemented as a combination of:

- **Queue**  
  The Cache is essentially a Queue which allows the cache to push to the front and evict from the back.

- **Doubly Linked List**  
  The Queue used by the Cache is implemented with a doubly linked list which maintains insertion/access order for supporting FIFO or LRU eviction policies.

A Queue implemented with a Doubly Linked List is more optimal than an Array-Based Queue in this instance because of LRU policies. When the LRU Cache detects a hit, the node that represents that request is moved to the front of the cache. This is much less efficient when using an Array-Based Queue.

Data Structure code for Cache entries can be found in `cache.h`

---


## 🛠️ Compilation

> [!NOTE]  
> The library implementation for the header files in this respository is kept private, meaning the program will not compile. A public version is in production and will be added when available.

To build the proxy, run:

```bash
make
```
This compiles the code using the Makefile and produces an executable named httpproxy. More `make` commands are available in the Makefile.

---

## ▶️ Run the Proxy

```bash
./httpproxy <port> <mode> <n>
```
- <port>: Port number to listen on (1–65535)
- <mode>: Cache eviction mode — FIFO or LRU
- <n>: Max number of cache entries (0–1024)

Passing n = 0 disables caching altogether.

---

## ⚙️ Example
```bash
./httpproxy 1234 FIFO 3
```
This runs the proxy on port 1234 using a FIFO cache of size 3.

---

## 📄 Cache Behavior Examples

#### 🧪 Example 1: FIFO (First-In First-Out)
Proxy initialized with FIFO cache of size 3:
```
Request to h1/foo.txt     → MISS (added)
Request to h1/qux.txt     → MISS (added)
Request to h1/foo.txt     → HIT  (Cached: True)
Request to h2/foo.txt     → MISS (added)
Request to h2/qux.txt     → MISS (evicts h1/foo.txt)
Request to h1/foo.txt     → MISS (evicts h1/qux.txt)
```

#### 🧪 Example 2: LRU (Least Recently Used)
Proxy initialized with LRU cache of size 3:
```
Request to h1/foo.txt     → MISS (added)
Request to h1/qux.txt     → MISS (added)
Request to h1/foo.txt     → HIT  (move to most recent)
Request to h2/foo.txt     → MISS (added)
Request to h2/qux.txt     → MISS (evicts h1/qux.txt)
Request to h1/foo.txt     → HIT  (move to most recent)
```

---









