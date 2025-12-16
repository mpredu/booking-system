#include "BookingService.h"
#include "SeatBitmask.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>

class TestFramework {
public:
    static void assertTrue(bool condition, const std::string& msg) {
        if (condition) {
            std::cout << "✓ PASSED: " << msg << "\n";
            passed_++;
        } else {
            std::cerr << "✗ FAILED: " << msg << "\n";
            failed_++;
        }
    }
    
    static void assertEqual(int expected, int actual, const std::string& msg) {
        if (expected == actual) {
            std::cout << "✓ PASSED: " << msg << "\n";
            passed_++;
        } else {
            std::cerr << "✗ FAILED: " << msg << " (expected: " << expected 
                      << ", got: " << actual << ")\n";
            failed_++;
        }
    }
    
    static void printSummary() {
        std::cout << "\n=================================\n";
        std::cout << "Test Summary\n";
        std::cout << "=================================\n";
        std::cout << "Passed: " << passed_ << "\n";
        std::cout << "Failed: " << failed_ << "\n";
        std::cout << "Total:  " << (passed_ + failed_) << "\n";
        std::cout << "=================================\n";
    }
    
    static int getFailedCount() { return failed_; }

private:
    static int passed_;
    static int failed_;
};

int TestFramework::passed_ = 0;
int TestFramework::failed_ = 0;

// ===== Teste pentru SeatBitmask =====

void testBitmaskBasics() {
    std::cout << "\n--- Test: Bitmask Basics ---\n";
    
    // Test conversii
    TestFramework::assertEqual(0, SeatBitmask::seatNumberToBit(1), "Seat 1 -> bit 0");
    TestFramework::assertEqual(19, SeatBitmask::seatNumberToBit(20), "Seat 20 -> bit 19");
    TestFramework::assertEqual(1, SeatBitmask::bitToSeatNumber(0), "Bit 0 -> seat 1");
    
    // Test seat ID conversions
    TestFramework::assertEqual(0, SeatBitmask::seatIdToBit("a1"), "a1 -> bit 0");
    TestFramework::assertEqual(4, SeatBitmask::seatIdToBit("a5"), "a5 -> bit 4");
    TestFramework::assertEqual(19, SeatBitmask::seatIdToBit("a20"), "a20 -> bit 19");
    TestFramework::assertEqual(-1, SeatBitmask::seatIdToBit("a21"), "a21 invalid");
    TestFramework::assertEqual(-1, SeatBitmask::seatIdToBit("b1"), "b1 invalid");
    
    // Test mask creation
    std::vector<std::string> seats = {"a1", "a5", "a10"};
    uint32_t mask = SeatBitmask::createMask(seats);
    TestFramework::assertTrue((mask & (1u << 0)) != 0, "Mask includes bit 0 (a1)");
    TestFramework::assertTrue((mask & (1u << 4)) != 0, "Mask includes bit 4 (a5)");
    TestFramework::assertTrue((mask & (1u << 9)) != 0, "Mask includes bit 9 (a10)");
    TestFramework::assertTrue((mask & (1u << 1)) == 0, "Mask excludes bit 1 (a2)");
}

void testBitmaskLockFreeBooking() {
    std::cout << "\n--- Test: Lock-Free Booking with Bitmask ---\n";
    
    SeatBitmask mask;
    
    // Initial: all seats available
    TestFramework::assertEqual(20, mask.getAvailableCount(), "Initially 20 seats available");
    
    // Reserve a1, a2, a3
    std::vector<std::string> seats1 = {"a1", "a2", "a3"};
    uint32_t mask1 = SeatBitmask::createMask(seats1);
    bool success1 = mask.tryBook(mask1);
    TestFramework::assertTrue(success1, "First booking succeeds");
    TestFramework::assertEqual(17, mask.getAvailableCount(), "17 seats after booking 3");
    
    // Încearca sa rezerve din nou a1 (ar trebui sa esueze)
    std::vector<std::string> seats2 = {"a1", "a4"};
    uint32_t mask2 = SeatBitmask::createMask(seats2);
    bool success2 = mask.tryBook(mask2);
    TestFramework::assertTrue(!success2, "Booking a1 again fails");
    TestFramework::assertEqual(17, mask.getAvailableCount(), "Still 17 seats (no partial booking)");
    
    // Rezerva a4, a5 (ar trebui sa reuseasca)
    std::vector<std::string> seats3 = {"a4", "a5"};
    uint32_t mask3 = SeatBitmask::createMask(seats3);
    bool success3 = mask.tryBook(mask3);
    TestFramework::assertTrue(success3, "Booking a4,a5 succeeds");
    TestFramework::assertEqual(15, mask.getAvailableCount(), "15 seats remaining");
}

void testConcurrentLockFreeBooking() {
    std::cout << "\n--- Test: 1000 Threads Lock-Free Booking ---\n";
    
    SeatBitmask mask;
    const int numThreads = 1000;
    std::atomic<int> successCount(0);
    std::atomic<int> failureCount(0);
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Fiecare thread incearca sa rezerve cate un scaun
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&mask, &successCount, &failureCount, i]() {
            // Cicleaza prin scaune (a1-a20)
            int seatNum = (i % 20) + 1;
            std::vector<std::string> seats = {"a" + std::to_string(seatNum)};
            uint32_t seatMask = SeatBitmask::createMask(seats);
            
            if (mask.tryBook(seatMask)) {
                successCount++;
            } else {
                failureCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Time: " << duration.count() << " ms\n";
    std::cout << "  Success: " << successCount.load() << "\n";
    std::cout << "  Failures: " << failureCount.load() << "\n";
    std::cout << "  Throughput: " << (numThreads * 1000 / duration.count()) << " ops/sec\n";
    
    // Ar trebui sa avem exact 20 de succese (cate un scaun)
    TestFramework::assertEqual(20, successCount.load(), "Exactly 20 successful bookings");
    TestFramework::assertEqual(980, failureCount.load(), "980 failed bookings");
}

void testLockFreeServiceBasics() {
    std::cout << "\n--- Test: Lock-Free Service Basics ---\n";
    
    BookingService service;
    
    auto movie = std::make_shared<Movie>(1, "Inception");
    auto theater = std::make_shared<Theater>(1, "IMAX");
    
    service.addMovie(movie);
    service.addTheater(theater);
    service.linkMovieToTheater(1, 1);
    
    // Verifica disponibilitate
    auto available = service.getAvailableSeats(1, 1);
    TestFramework::assertEqual(20, available.size(), "20 seats available initially");
    
    // Booking
    std::vector<std::string> seats = {"a1", "a2", "a3"};
    auto booking = service.bookSeats(1, 1, seats);
    TestFramework::assertTrue(booking != nullptr, "Booking succeeds");
    TestFramework::assertEqual(1, booking->bookingId, "Booking ID is 1");
    
    // Verifica scaune ramase
    uint32_t remaining = service.getAvailableCount(1, 1);
    TestFramework::assertEqual(17, remaining, "17 seats remaining");
    
    // Ocupare procentuala
    double occupancy = service.getOccupancyPercentage(1, 1);
    TestFramework::assertTrue(occupancy > 14.9 && occupancy < 15.1, "15% occupancy");
}

void testMassiveConcurrentBooking() {
    std::cout << "\n--- Test: 10,000 Threads Concurrent Booking ---\n";
    
    BookingService service;
    
    // Setup: 500 movies x 500 theaters
    const int numMovies = 500;
    const int numTheaters = 500;
    
    std::cout << "  Setting up " << numMovies << " movies and " << numTheaters << " theaters...\n";
    
    for (int i = 1; i <= numMovies; ++i) {
        service.addMovie(std::make_shared<Movie>(i, "Movie " + std::to_string(i)));
    }
    
    for (int i = 1; i <= numTheaters; ++i) {
        service.addTheater(std::make_shared<Theater>(i, "Theater " + std::to_string(i)));
    }
    
    // Link fiecare movie la primul sau theater
    for (int i = 1; i <= numMovies; ++i) {
        int theaterId = ((i - 1) % numTheaters) + 1;
        service.linkMovieToTheater(i, theaterId);
    }
    
    std::cout << "  Launching 10,000 concurrent booking threads...\n";
    
    const int numThreads = 10000;
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&service, &successCount, i]() {
            // Fiecare thread incearca sa rezerve pentru un movie/theater aleatoriu
            int movieId = (i % 500) + 1;
            int theaterId = ((movieId - 1) % 500) + 1;
            int seatNum = (i % 20) + 1;
            
            std::vector<std::string> seats = {"a" + std::to_string(seatNum)};
            auto booking = service.bookSeats(movieId, theaterId, seats);
            
            if (booking) {
                successCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  ✓ Completed in " << duration.count() << " ms\n";
    std::cout << "  ✓ Successful bookings: " << successCount.load() << "\n";
    std::cout << "  ✓ Throughput: " << (numThreads * 1000 / duration.count()) << " ops/sec\n";
    
    TestFramework::assertTrue(successCount.load() > 0, "Some bookings succeeded");
}

void benchmarkLockFreeVsOthers() {
    std::cout << "\n--- Benchmark: Lock-Free Performance ---\n";
    
    BookingService service;
    
    auto movie = std::make_shared<Movie>(1, "Test Movie");
    auto theater = std::make_shared<Theater>(1, "Test Theater");
    service.addMovie(movie);
    service.addTheater(theater);
    service.linkMovieToTheater(1, 1);
    
    const int iterations = 100000;
    std::atomic<int> successCount(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Test with 100 de threads concurente
    std::vector<std::thread> threads;
    const int numThreads = 100;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&service, &successCount, iterations, numThreads, t]() {
            for (int i = 0; i < iterations / numThreads; ++i) {
                auto seats = service.getAvailableSeats(1, 1);
                if (!seats.empty()) {
                    successCount++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avgMicroseconds = static_cast<double>(duration.count()) / iterations;
    
    std::cout << "  ✓ " << iterations << " lock-free reads\n";
    std::cout << "  ✓ Time: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "  ✓ Average: " << avgMicroseconds << " μs per operation\n";
    std::cout << "  ✓ Throughput: " << static_cast<int>(1000000.0 / avgMicroseconds) 
              << " ops/second\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "Lock-Free Booking System - Tests\n";
    std::cout << "=========================================\n";
    
    testBitmaskBasics();
    testBitmaskLockFreeBooking();
    testConcurrentLockFreeBooking();
    testLockFreeServiceBasics();
    testMassiveConcurrentBooking();
    benchmarkLockFreeVsOthers();
    
    TestFramework::printSummary();
    
    return TestFramework::getFailedCount() > 0 ? 1 : 0;
}
