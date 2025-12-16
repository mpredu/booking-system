# Quick Start - Booking System

### Option 1: One script (All-in-One)

```bash
# Run everything automatically
./do_all.sh
```
This script will:
 1. Create build directory and compile the project
 2. Generate documentation (Doxygen)
 3. Run all tests
 4. Optionally build and run Docker container

### Option 2: Build and Run

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Run CLI application
./booking_lockfree

# Run CLI application
./booking_lockfree

# Run tests
./test_lockfree
./test_overbooking
./test_scalability
./test_two_thread_race
```

### Option 3: Docker 

```bash
# Build image
docker build -t booking-lockfree .

# Run CLI
docker run -it booking-lockfree

# Run tests
docker run booking-lockfree test_lockfree
docker run booking-lockfree test_lockfree
docker run booking-lockfree test_overbooking
docker run booking-lockfree test_scalability
docker run booking-lockfree test_two_thread_race
```

## What to Try in CLI

1. **Option 1** - View all movies
2. **Option 4** - Book seats 
   - Movie ID: `1`
   - Theater ID: `1`
   - Seats: `a1,a2,a3`
3. **Option 6** - View occupancy statistics

## 🧪 Available Tests

```bash
./test_lockfree
./test_overbooking
./test_two_thread_race
```

**Expected output example:**
```
✓ 27 tests passed
✓ 1000 concurrent threads
✓ Throughput: 965,250 ops/sec
✓ Zero deadlocks
```

##  Key Features

- ✅ **Bitmask** for 20 seats (4 bytes)
- ✅ **Lock-Free** booking with Compare-And-Swap
- ✅ **~1000k ops/sec** throughput for reads
- ✅ **Zero blocking** - no mutex for booking
- ✅ **Linear scalability** with number of CPU cores

## 📁 Project Structure

```
movie-booking-lockfree/
├── CMakeLists.txt             # Build configuration
├── Dockerfile                 # Container definition
├── Doxyfile                   # Documentation config
├── README.md                  # Complete documentation
├── QUICKSTART.md              # This file
├── include/
│   ├── SeatBitmask.h          # Atomic bitmask for seats
│   └── BookingService.h       # Main service
├── src/
│   ├── SeatBitmask.cpp
│   ├── BookingService.cpp
│   └── main.cpp               # CLI application
└── tests/
    ├── test_lockfree.cpp       # Lock-free unit tests
    ├── test_overbooking.cpp    # Overbooking scenario tests
    └── test_two_thread_race.cpp # Two-thread race condition tests
    └── test_scalability.cpp    # Scalability tests
    
```

## 📚 Read More

- [README.md](README.md) - Complete documentation
- Headers in `include/` - Documented API

## ⚡ Pro Tip

For detailed debugging:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

For maximum performance:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

---
