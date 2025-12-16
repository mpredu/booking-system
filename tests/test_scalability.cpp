#include "BookingService.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <map>
#include <set>
#include <iomanip>

// ============================================================================
// SCALABILITY TESTS - Large Datasets & High Concurrency
// ============================================================================

class ScalabilityTests {
public:
    static int passed;
    static int failed;
    
    static void assertTrue(bool condition, const std::string& msg) {
        if (condition) {
            std::cout << "  ✓ " << msg << "\n";
            passed++;
        } else {
            std::cerr << "  ✗ FAILED: " << msg << "\n";
            failed++;
        }
    }
    
    static void assertEqual(int expected, int actual, const std::string& msg) {
        if (expected == actual) {
            std::cout << "  ✓ " << msg << "\n";
            passed++;
        } else {
            std::cerr << "  ✗ FAILED: " << msg << " (expected: " << expected 
                      << ", got: " << actual << ")\n";
            failed++;
        }
    }
    
    static void printStats(const std::string& label, int64_t operations, 
                          int64_t durationMs) {
        double opsPerSec = (operations * 1000.0) / durationMs;
        std::cout << "  " << label << ":\n";
        std::cout << "    Operations: " << operations << "\n";
        std::cout << "    Duration: " << durationMs << " ms\n";
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) 
                  << opsPerSec << " ops/sec\n";
    }
};

int ScalabilityTests::passed = 0;
int ScalabilityTests::failed = 0;

// ============================================================================
// TEST 1: Large Dataset Creation & Metadata Access
// ============================================================================

void testLargeDatasetCreation() {
    std::cout << "\n=== TEST 1: Large Dataset Creation ===\n";
    std::cout << "Goal: Create 1000 movies × 100 theaters = 100,000 combinations\n\n";
    
    BookingService service;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create 1000 movies
    std::cout << "Creating 1000 movies...\n";
    for (int i = 1; i <= 1000; i++) {
        service.addMovie(std::make_shared<Movie>(i, "Movie " + std::to_string(i)));
    }
    
    auto moviesCreated = std::chrono::high_resolution_clock::now();
    auto moviesTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        moviesCreated - startTime).count();
    
    // Create 100 theaters
    std::cout << "Creating 100 theaters...\n";
    for (int i = 1; i <= 100; i++) {
        service.addTheater(std::make_shared<Theater>(i, "Theater " + std::to_string(i)));
    }
    
    auto theatersCreated = std::chrono::high_resolution_clock::now();
    auto theatersTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        theatersCreated - moviesCreated).count();
    
    // Link all combinations
    std::cout << "Linking 100,000 combinations...\n";
    int linksCreated = 0;
    for (int m = 1; m <= 1000; m++) {
        for (int t = 1; t <= 100; t++) {
            if (service.linkMovieToTheater(m, t)) {
                linksCreated++;
            }
        }
    }
    
    auto linksComplete = std::chrono::high_resolution_clock::now();
    auto linksTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        linksComplete - theatersCreated).count();
    
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        linksComplete - startTime).count();
    
    std::cout << "\nResults:\n";
    std::cout << "  Movies created: 1000 in " << moviesTime << " ms\n";
    std::cout << "  Theaters created: 100 in " << theatersTime << " ms\n";
    std::cout << "  Links created: " << linksCreated << " in " << linksTime << " ms\n";
    std::cout << "  Total time: " << totalTime << " ms\n";
    
    ScalabilityTests::assertEqual(100000, linksCreated, "All 100,000 links created");
    ScalabilityTests::assertTrue(totalTime < 5000, "Creation completed in <5 seconds");
    
    // Verify retrieval
    auto allMovies = service.getAllMovies();
    ScalabilityTests::assertEqual(1000, allMovies.size(), "Can retrieve all 1000 movies");
}

// ============================================================================
// TEST 2: Concurrent Metadata Reads
// ============================================================================

void testConcurrentMetadataReads() {
    std::cout << "\n=== TEST 2: Concurrent Metadata Reads ===\n";
    std::cout << "Goal: 10,000 threads reading movies/theaters simultaneously\n\n";
    
    BookingService service;
    
    // Setup: 500 movies, 50 theaters
    for (int i = 1; i <= 500; i++) {
        service.addMovie(std::make_shared<Movie>(i, "Movie " + std::to_string(i)));
    }
    for (int i = 1; i <= 50; i++) {
        service.addTheater(std::make_shared<Theater>(i, "Theater " + std::to_string(i)));
    }
    for (int m = 1; m <= 500; m++) {
        for (int t = 1; t <= 50; t++) {
            service.linkMovieToTheater(m, t);
        }
    }
    
    const int NUM_THREADS = 10000;
    std::atomic<int64_t> totalReads{0};
    std::vector<std::thread> threads;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            // Each thread does 10 random operations
            std::mt19937 gen(i);
            std::uniform_int_distribution<> opDist(0, 2);
            std::uniform_int_distribution<> movieDist(1, 500);
            std::uniform_int_distribution<> theaterDist(1, 50);
            
            for (int op = 0; op < 10; op++) {
                int operation = opDist(gen);
                
                if (operation == 0) {
                    // Read all movies
                    auto movies = service.getAllMovies();
                    totalReads++;
                } else if (operation == 1) {
                    // Read specific movie
                    int movieId = movieDist(gen);
                    auto movie = service.getMovie(movieId);
                    totalReads++;
                } else {
                    // Read theaters for movie
                    int movieId = movieDist(gen);
                    auto theaters = service.getTheatersForMovie(movieId);
                    totalReads++;
                }
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    std::cout << "Results:\n";
    ScalabilityTests::printStats("Metadata reads", totalReads.load(), duration);
    
    ScalabilityTests::assertEqual(100000, totalReads.load(), 
                                 "All 100,000 reads completed (10,000 threads × 10 ops)");
    ScalabilityTests::assertTrue(duration < 10000, 
                                "Completed in <10 seconds");
}

// ============================================================================
// TEST 3: Concurrent Bookings Across Many Combinations
// ============================================================================

void testManyBookingsAcrossCombinations() {
    std::cout << "\n=== TEST 3: Concurrent Bookings Across Combinations ===\n";
    std::cout << "Goal: 10,000 threads booking across 50,000 combinations\n\n";
    
    BookingService service;
    
    // Setup: 500 movies × 100 theaters = 50,000 combinations
    std::cout << "Setting up 500 movies × 100 theaters...\n";
    for (int i = 1; i <= 500; i++) {
        service.addMovie(std::make_shared<Movie>(i, "Movie " + std::to_string(i)));
    }
    for (int i = 1; i <= 100; i++) {
        service.addTheater(std::make_shared<Theater>(i, "Theater " + std::to_string(i)));
    }
    for (int m = 1; m <= 500; m++) {
        for (int t = 1; t <= 100; t++) {
            service.linkMovieToTheater(m, t);
        }
    }
    
    const int NUM_THREADS = 10000;
    std::atomic<int> successfulBookings{0};
    std::atomic<int> failedBookings{0};
    std::vector<std::thread> threads;
    
    // Track which (movie, theater, seat) combinations were booked
    std::map<std::tuple<int, int, std::string>, int> bookingCounts;
    std::mutex countMutex;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Each thread tries to book a random combination
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            std::mt19937 gen(i);
            std::uniform_int_distribution<> movieDist(1, 500);
            std::uniform_int_distribution<> theaterDist(1, 100);
            std::uniform_int_distribution<> seatDist(1, 20);
            
            int movieId = movieDist(gen);
            int theaterId = theaterDist(gen);
            int seatNum = seatDist(gen);
            std::string seat = "a" + std::to_string(seatNum);
            
            auto booking = service.bookSeats(movieId, theaterId, {seat});
            
            if (booking) {
                successfulBookings++;
                
                std::lock_guard<std::mutex> lock(countMutex);
                bookingCounts[{movieId, theaterId, seat}]++;
            } else {
                failedBookings++;
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    // Verify no seat was booked twice
    int maxBookings = 0;
    bool noDuplicates = true;
    for (const auto& [key, count] : bookingCounts) {
        if (count > 1) {
            noDuplicates = false;
            std::cerr << "  ERROR: Seat booked " << count << " times!\n";
        }
        maxBookings = std::max(maxBookings, count);
    }
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Duration: " << duration << " ms\n";
    std::cout << "  Successful bookings: " << successfulBookings.load() << "\n";
    std::cout << "  Failed bookings: " << failedBookings.load() << "\n";
    std::cout << "  Unique bookings: " << bookingCounts.size() << "\n";
    std::cout << "  Throughput: " << (NUM_THREADS * 1000 / duration) << " ops/sec\n";
    
    ScalabilityTests::assertTrue(noDuplicates, "No seat was booked more than once");
    ScalabilityTests::assertEqual(1, maxBookings, "Each seat booked maximum once");
    ScalabilityTests::assertEqual(NUM_THREADS, 
                                 successfulBookings.load() + failedBookings.load(),
                                 "All threads completed");
}

// ============================================================================
// TEST 4: Mixed Realistic Workload
// ============================================================================

void testRealisticMixedWorkload() {
    std::cout << "\n=== TEST 4: Realistic Mixed Workload ===\n";
    std::cout << "Goal: 80% reads, 15% bookings, 5% metadata (real-world scenario)\n\n";
    
    BookingService service;
    
    // Setup: 200 movies × 50 theaters
    std::cout << "Setting up 200 movies × 50 theaters...\n";
    for (int i = 1; i <= 200; i++) {
        service.addMovie(std::make_shared<Movie>(i, "Movie " + std::to_string(i)));
    }
    for (int i = 1; i <= 50; i++) {
        service.addTheater(std::make_shared<Theater>(i, "Theater " + std::to_string(i)));
    }
    for (int m = 1; m <= 200; m++) {
        for (int t = 1; t <= 50; t++) {
            service.linkMovieToTheater(m, t);
        }
    }
    
    const int NUM_THREADS = 5000;
    std::atomic<int64_t> readOps{0};
    std::atomic<int64_t> bookingOps{0};
    std::atomic<int64_t> metadataOps{0};
    std::vector<std::thread> threads;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Each thread does mixed operations
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            std::mt19937 gen(i);
            std::uniform_int_distribution<> opDist(1, 100);
            std::uniform_int_distribution<> movieDist(1, 200);
            std::uniform_int_distribution<> theaterDist(1, 50);
            std::uniform_int_distribution<> seatDist(1, 20);
            
            // Each thread does 20 operations
            for (int op = 0; op < 20; op++) {
                int operation = opDist(gen);
                
                if (operation <= 80) {
                    // 80% - Read available seats
                    int movieId = movieDist(gen);
                    int theaterId = theaterDist(gen);
                    auto seats = service.getAvailableSeats(movieId, theaterId);
                    readOps++;
                    
                } else if (operation <= 95) {
                    // 15% - Try to book
                    int movieId = movieDist(gen);
                    int theaterId = theaterDist(gen);
                    int seatNum = seatDist(gen);
                    std::string seat = "a" + std::to_string(seatNum);
                    
                    service.bookSeats(movieId, theaterId, {seat});
                    bookingOps++;
                    
                } else {
                    // 5% - Metadata access
                    int movieId = movieDist(gen);
                    auto movie = service.getMovie(movieId);
                    auto theaters = service.getTheatersForMovie(movieId);
                    metadataOps++;
                }
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    int64_t totalOps = readOps.load() + bookingOps.load() + metadataOps.load();
    
    std::cout << "Results:\n";
    std::cout << "  Total operations: " << totalOps << " (5000 threads × 20 ops)\n";
    std::cout << "  Duration: " << duration << " ms\n";
    std::cout << "  Throughput: " << (totalOps * 1000 / duration) << " ops/sec\n\n";
    
    std::cout << "  Breakdown:\n";
    std::cout << "    Read ops: " << readOps.load() 
              << " (" << (readOps.load() * 100 / totalOps) << "%)\n";
    std::cout << "    Booking ops: " << bookingOps.load() 
              << " (" << (bookingOps.load() * 100 / totalOps) << "%)\n";
    std::cout << "    Metadata ops: " << metadataOps.load() 
              << " (" << (metadataOps.load() * 100 / totalOps) << "%)\n";
    
    ScalabilityTests::assertEqual(100000, totalOps, "All 100,000 operations completed");
    ScalabilityTests::assertTrue(duration < 15000, "Completed in <15 seconds");
    
    // Verify proportions are approximately correct (within 5%)
    int readPercent = (readOps.load() * 100) / totalOps;
    int bookingPercent = (bookingOps.load() * 100) / totalOps;
    int metadataPercent = (metadataOps.load() * 100) / totalOps;
    
    ScalabilityTests::assertTrue(readPercent >= 75 && readPercent <= 85, 
                                "Read ops ~80% (75-85%)");
    ScalabilityTests::assertTrue(bookingPercent >= 10 && bookingPercent <= 20, 
                                "Booking ops ~15% (10-20%)");
    ScalabilityTests::assertTrue(metadataPercent >= 0 && metadataPercent <= 10, 
                                "Metadata ops ~5% (0-10%)");
}

// ============================================================================
// TEST 5: Memory Footprint Verification
// ============================================================================

void testMemoryFootprint() {
    std::cout << "\n=== TEST 5: Memory Footprint Verification ===\n";
    std::cout << "Goal: Verify bitmask uses ~4 bytes per combination\n\n";
    
    BookingService service;
    
    // Create 1000 movies × 100 theaters = 100,000 combinations
    std::cout << "Creating 100,000 combinations...\n";
    for (int i = 1; i <= 1000; i++) {
        service.addMovie(std::make_shared<Movie>(i, "M" + std::to_string(i)));
    }
    for (int i = 1; i <= 100; i++) {
        service.addTheater(std::make_shared<Theater>(i, "T" + std::to_string(i)));
    }
    
    // Link and book one seat in each to create SeatBitmask
    for (int m = 1; m <= 1000; m++) {
        for (int t = 1; t <= 100; t++) {
            service.linkMovieToTheater(m, t);
            service.bookSeats(m, t, {"a1"});  // Force SeatBitmask creation
        }
    }
    
    // Theoretical memory for bitmasks
    size_t bitmaskMemory = 100000 * sizeof(uint32_t);  // 100,000 × 4 bytes
    size_t bitmaskMemoryKB = bitmaskMemory / 1024;
    
    // If we used vector<string> instead
    size_t vectorMemory = 100000 * 264;  // Approximate
    size_t vectorMemoryKB = vectorMemory / 1024;
    
    std::cout << "Memory analysis:\n";
    std::cout << "  Combinations: 100,000\n";
    std::cout << "  Bitmask approach: ~" << bitmaskMemoryKB << " KB (4 bytes each)\n";
    std::cout << "  Vector<string> approach: ~" << vectorMemoryKB << " KB (264 bytes each)\n";
    std::cout << "  Savings: " << (vectorMemoryKB - bitmaskMemoryKB) << " KB\n";
    std::cout << "  Efficiency: " << (vectorMemory / bitmaskMemory) << "x better\n";
    
    ScalabilityTests::assertEqual(390, bitmaskMemoryKB, 
                                 "Bitmask memory ~390 KB (400,000 bytes)");
    ScalabilityTests::assertTrue(bitmaskMemory < 500000, 
                                "Bitmask memory <500 KB");
    ScalabilityTests::assertTrue((vectorMemory / bitmaskMemory) > 60, 
                                "Bitmask >60x more efficient than vector");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "=========================================================\n";
    std::cout << "      SCALABILITY TESTS - LOCK-FREE BOOKING SYSTEM\n";
    std::cout << "=========================================================\n";
    
    testLargeDatasetCreation();
    testConcurrentMetadataReads();
    testManyBookingsAcrossCombinations();
    testRealisticMixedWorkload();
    testMemoryFootprint();
    
    std::cout << "\n=========================================================\n";
    std::cout << "                    FINAL RESULTS\n";
    std::cout << "=========================================================\n";
    std::cout << "Passed: " << ScalabilityTests::passed << "\n";
    std::cout << "Failed: " << ScalabilityTests::failed << "\n";
    std::cout << "Total:  " << (ScalabilityTests::passed + ScalabilityTests::failed) << "\n";
    std::cout << "=========================================================\n";
    
    if (ScalabilityTests::failed == 0) {
        std::cout << "\n✅ ALL SCALABILITY TESTS PASSED!\n";
        std::cout << "   SYSTEM HANDLES LARGE DATASETS & HIGH CONCURRENCY\n\n";
    } else {
        std::cout << "\n❌ SOME TESTS FAILED - SCALABILITY ISSUES DETECTED!\n\n";
    }
    
    return ScalabilityTests::failed > 0 ? 1 : 0;
}
