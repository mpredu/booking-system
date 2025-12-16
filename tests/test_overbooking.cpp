#include "BookingService.h"
#include "SeatBitmask.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <set>
#include <map>
#include <random>
#include <algorithm>

// ============================================================================
// EXHAUSTIVE OVERBOOKING TESTS
// ============================================================================

class OverbookingTests {
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
};

int OverbookingTests::passed = 0;
int OverbookingTests::failed = 0;

// ============================================================================
// TEST 1: LINEAR OVERBOOKING - Sequential Attempts
// ============================================================================

void testLinearOverbookingSequential() {
    std::cout << "\n=== TEST 1: Linear Overbooking - Sequential ===\n";
    std::cout << "Goal: Verify that once a seat is booked, subsequent attempts fail\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test Movie"));
    service.addTheater(std::make_shared<Theater>(1, "Test Theater"));
    service.linkMovieToTheater(1, 1);
    
    // Test 1.1: Book same seat twice sequentially
    std::cout << "Test 1.1: Book same seat (a1) twice\n";
    auto booking1 = service.bookSeats(1, 1, {"a1"});
    OverbookingTests::assertTrue(booking1 != nullptr, "First booking of a1 succeeds");
    
    auto booking2 = service.bookSeats(1, 1, {"a1"});
    OverbookingTests::assertTrue(booking2 == nullptr, "Second booking of a1 fails (overbooking prevented)");
    
    // Test 1.2: Book overlapping seats
    std::cout << "\nTest 1.2: Book overlapping seats\n";
    auto booking3 = service.bookSeats(1, 1, {"a2", "a3", "a4"});
    OverbookingTests::assertTrue(booking3 != nullptr, "Booking a2-a4 succeeds");
    
    auto booking4 = service.bookSeats(1, 1, {"a3", "a5"});
    OverbookingTests::assertTrue(booking4 == nullptr, "Booking a3,a5 fails (a3 already booked)");
    
    // Verify a5 is still available
    auto booking5 = service.bookSeats(1, 1, {"a5"});
    OverbookingTests::assertTrue(booking5 != nullptr, "Booking a5 alone succeeds (wasn't booked)");
    
    // Test 1.3: Try to book all 20 seats individually
    std::cout << "\nTest 1.3: Book all 20 seats individually\n";
    BookingService service2;
    service2.addMovie(std::make_shared<Movie>(2, "Test2"));
    service2.addTheater(std::make_shared<Theater>(2, "Test2"));
    service2.linkMovieToTheater(2, 2);
    
    int successCount = 0;
    for (int i = 1; i <= 20; i++) {
        std::string seat = "a" + std::to_string(i);
        auto booking = service2.bookSeats(2, 2, {seat});
        if (booking) successCount++;
    }
    OverbookingTests::assertEqual(20, successCount, "All 20 seats booked exactly once");
    
    // Try to book any seat again - all should fail
    int failCount = 0;
    for (int i = 1; i <= 20; i++) {
        std::string seat = "a" + std::to_string(i);
        auto booking = service2.bookSeats(2, 2, {seat});
        if (!booking) failCount++;
    }
    OverbookingTests::assertEqual(20, failCount, "All 20 re-booking attempts fail (no overbooking)");
    
    // Test 1.4: Verify exact seat count
    uint32_t available = service2.getAvailableCount(2, 2);
    OverbookingTests::assertEqual(0, available, "Zero seats available after booking all 20");
}

// ============================================================================
// TEST 2: LINEAR OVERBOOKING - Batch Booking
// ============================================================================

void testLinearOverbookingBatch() {
    std::cout << "\n=== TEST 2: Linear Overbooking - Batch Booking ===\n";
    std::cout << "Goal: Verify batch bookings don't allow partial overbooking\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    // Test 2.1: Book 10 seats, then try to book overlapping 10 seats
    std::cout << "Test 2.1: Overlapping batch bookings\n";
    std::vector<std::string> batch1;
    for (int i = 1; i <= 10; i++) {
        batch1.push_back("a" + std::to_string(i));
    }
    
    auto booking1 = service.bookSeats(1, 1, batch1);
    OverbookingTests::assertTrue(booking1 != nullptr, "First batch (a1-a10) succeeds");
    OverbookingTests::assertEqual(10, service.getAvailableCount(1, 1), "10 seats remain");
    
    std::vector<std::string> batch2;
    for (int i = 5; i <= 15; i++) {  // Overlaps with a5-a10
        batch2.push_back("a" + std::to_string(i));
    }
    
    auto booking2 = service.bookSeats(1, 1, batch2);
    OverbookingTests::assertTrue(booking2 == nullptr, "Second batch (a5-a15) fails (overlap)");
    OverbookingTests::assertEqual(10, service.getAvailableCount(1, 1), "Still 10 seats (no partial booking)");
    
    // Test 2.2: Verify the non-overlapping seats are still available
    std::cout << "\nTest 2.2: Non-overlapping seats still available\n";
    std::vector<std::string> batch3;
    for (int i = 11; i <= 20; i++) {
        batch3.push_back("a" + std::to_string(i));
    }
    
    auto booking3 = service.bookSeats(1, 1, batch3);
    OverbookingTests::assertTrue(booking3 != nullptr, "Non-overlapping batch (a11-a20) succeeds");
    OverbookingTests::assertEqual(0, service.getAvailableCount(1, 1), "All seats now booked");
    
    // Test 2.3: Try to book any combination - should all fail
    std::cout << "\nTest 2.3: All re-booking attempts fail\n";
    auto failBooking1 = service.bookSeats(1, 1, {"a1"});
    auto failBooking2 = service.bookSeats(1, 1, {"a10", "a11"});
    auto failBooking3 = service.bookSeats(1, 1, {"a1", "a5", "a10", "a15", "a20"});
    
    OverbookingTests::assertTrue(failBooking1 == nullptr, "Single seat re-booking fails");
    OverbookingTests::assertTrue(failBooking2 == nullptr, "Two-seat re-booking fails");
    OverbookingTests::assertTrue(failBooking3 == nullptr, "Multi-seat re-booking fails");
}

// ============================================================================
// TEST 3: CONCURRENT OVERBOOKING - Same Seat
// ============================================================================

void testConcurrentOverbookingSameSeat() {
    std::cout << "\n=== TEST 3: Concurrent Overbooking - Same Seat ===\n";
    std::cout << "Goal: 1000 threads try to book the same seat - only 1 should succeed\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    const int NUM_THREADS = 1000;
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};
    std::vector<std::thread> threads;
    std::vector<uint32_t> successfulBookingIds;
    std::mutex idMutex;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // All threads try to book seat a1
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&service, &successCount, &failCount, &successfulBookingIds, &idMutex]() {
            auto booking = service.bookSeats(1, 1, {"a1"});
            if (booking) {
                successCount++;
                std::lock_guard<std::mutex> lock(idMutex);
                successfulBookingIds.push_back(booking->bookingId);
            } else {
                failCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Time: " << duration.count() << " ms\n";
    std::cout << "  Successful bookings: " << successCount.load() << "\n";
    std::cout << "  Failed bookings: " << failCount.load() << "\n";
    
    OverbookingTests::assertEqual(1, successCount.load(), "EXACTLY 1 thread succeeded");
    OverbookingTests::assertEqual(NUM_THREADS - 1, failCount.load(), "All other threads failed");
    OverbookingTests::assertEqual(1, successfulBookingIds.size(), "Only 1 booking ID created");
    
    // Verify the seat is actually booked
    uint32_t available = service.getAvailableCount(1, 1);
    OverbookingTests::assertEqual(19, available, "19 seats remain (1 booked)");
}

// ============================================================================
// TEST 4: CONCURRENT OVERBOOKING - All Seats
// ============================================================================

void testConcurrentOverbookingAllSeats() {
    std::cout << "\n=== TEST 4: Concurrent Overbooking - All 20 Seats ===\n";
    std::cout << "Goal: 1000 threads try to book all seats - exactly 20 bookings succeed\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    const int NUM_THREADS = 1000;
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    std::vector<std::string> bookedSeats;
    std::mutex seatsMutex;
    
    // Each thread tries to book a random seat
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            int seatNum = (i % 20) + 1;  // Cycles through a1-a20
            std::string seat = "a" + std::to_string(seatNum);
            
            auto booking = service.bookSeats(1, 1, {seat});
            if (booking) {
                successCount++;
                std::lock_guard<std::mutex> lock(seatsMutex);
                bookedSeats.push_back(seat);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Successful bookings: " << successCount.load() << "\n";
    
    OverbookingTests::assertEqual(20, successCount.load(), "EXACTLY 20 threads succeeded (one per seat)");
    
    // Verify no duplicate seats were booked
    std::set<std::string> uniqueSeats(bookedSeats.begin(), bookedSeats.end());
    OverbookingTests::assertEqual(20, uniqueSeats.size(), "All 20 unique seats booked (no duplicates)");
    
    // Verify all seats are now booked
    uint32_t available = service.getAvailableCount(1, 1);
    OverbookingTests::assertEqual(0, available, "Zero seats available");
}

// ============================================================================
// TEST 5: CONCURRENT OVERBOOKING - Random Patterns
// ============================================================================

void testConcurrentOverbookingRandom() {
    std::cout << "\n=== TEST 5: Concurrent Overbooking - Random Patterns ===\n";
    std::cout << "Goal: Threads book random seat combinations - verify no overbooking\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    const int NUM_THREADS = 500;
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    
    // Track which seats each successful booking got
    std::vector<std::vector<std::string>> allBookedSeats;
    std::mutex bookingsMutex;
    
    std::random_device rd;
    
    // Each thread tries to book 1-5 random seats
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            std::mt19937 gen(rd() + i);  // Unique seed per thread
            std::uniform_int_distribution<> seatDist(1, 20);
            std::uniform_int_distribution<> countDist(1, 5);
            
            int numSeats = countDist(gen);
            std::set<std::string> seatsToBook;
            
            for (int j = 0; j < numSeats; j++) {
                seatsToBook.insert("a" + std::to_string(seatDist(gen)));
            }
            
            std::vector<std::string> seatVector(seatsToBook.begin(), seatsToBook.end());
            auto booking = service.bookSeats(1, 1, seatVector);
            
            if (booking) {
                successCount++;
                std::lock_guard<std::mutex> lock(bookingsMutex);
                allBookedSeats.push_back(booking->seats);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Successful bookings: " << successCount.load() << "\n";
    
    // Critical test: Verify no seat was booked twice
    std::map<std::string, int> seatBookingCount;
    for (const auto& booking : allBookedSeats) {
        for (const auto& seat : booking) {
            seatBookingCount[seat]++;
        }
    }
    
    bool noDuplicates = true;
    int maxBookings = 0;
    std::string mostBookedSeat;
    
    for (const auto& [seat, count] : seatBookingCount) {
        if (count > maxBookings) {
            maxBookings = count;
            mostBookedSeat = seat;
        }
        if (count > 1) {
            std::cerr << "  ERROR: Seat " << seat << " was booked " << count << " times!\n";
            noDuplicates = false;
        }
    }
    
    OverbookingTests::assertTrue(noDuplicates, "NO seat was booked more than once");
    std::cout << "  Most booked seat: " << mostBookedSeat << " (booked " << maxBookings << " time)\n";
    std::cout << "  Total unique seats booked: " << seatBookingCount.size() << "\n";
}

// ============================================================================
// TEST 6: CONCURRENT OVERBOOKING - Overlapping Batches
// ============================================================================

void testConcurrentOverbookingOverlappingBatches() {
    std::cout << "\n=== TEST 6: Concurrent Overbooking - Overlapping Batches ===\n";
    std::cout << "Goal: Multiple threads try to book overlapping seat ranges\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    const int NUM_THREADS = 100;
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> successfulBookings;
    std::mutex bookingsMutex;
    
    // Each thread tries to book 5 consecutive seats (sliding window)
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            int startSeat = (i % 16) + 1;  // Start from a1-a16
            std::vector<std::string> seats;
            
            for (int j = 0; j < 5; j++) {
                seats.push_back("a" + std::to_string(startSeat + j));
            }
            
            auto booking = service.bookSeats(1, 1, seats);
            
            if (booking) {
                successCount++;
                std::lock_guard<std::mutex> lock(bookingsMutex);
                successfulBookings.push_back(booking->seats);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Successful bookings: " << successCount.load() << "\n";
    
    // Verify no overlapping bookings
    std::set<std::string> allSeats;
    int totalSeatsBooked = 0;
    
    for (const auto& booking : successfulBookings) {
        totalSeatsBooked += booking.size();
        for (const auto& seat : booking) {
            allSeats.insert(seat);
        }
    }
    
    OverbookingTests::assertEqual(totalSeatsBooked, allSeats.size(), 
                                 "No duplicate seats across all bookings");
    
    std::cout << "  Total seats booked: " << allSeats.size() << " / 20\n";
    std::cout << "  Average seats per successful booking: " 
              << (totalSeatsBooked / successCount.load()) << "\n";
}

// ============================================================================
// TEST 7: STRESS TEST - Maximum Concurrency
// ============================================================================

void testStressMaximumConcurrency() {
    std::cout << "\n=== TEST 7: STRESS TEST - Maximum Concurrency ===\n";
    std::cout << "Goal: 10,000 threads hammering the system\n\n";
    
    BookingService service;
    service.addMovie(std::make_shared<Movie>(1, "Test"));
    service.addTheater(std::make_shared<Theater>(1, "Test"));
    service.linkMovieToTheater(1, 1);
    
    const int NUM_THREADS = 10000;
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};
    std::vector<std::thread> threads;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Each thread tries to book a seat
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            int seatNum = (i % 20) + 1;
            std::string seat = "a" + std::to_string(seatNum);
            
            auto booking = service.bookSeats(1, 1, {seat});
            
            if (booking) {
                successCount++;
            } else {
                failCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Results:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Time: " << duration.count() << " ms\n";
    std::cout << "  Throughput: " << (NUM_THREADS * 1000 / duration.count()) << " ops/sec\n";
    std::cout << "  Successful: " << successCount.load() << "\n";
    std::cout << "  Failed: " << failCount.load() << "\n";
    
    OverbookingTests::assertEqual(20, successCount.load(), "EXACTLY 20 bookings succeeded");
    OverbookingTests::assertEqual(NUM_THREADS - 20, failCount.load(), "All other attempts failed");
    
    // Final verification
    uint32_t available = service.getAvailableCount(1, 1);
    OverbookingTests::assertEqual(0, available, "All seats booked, none available");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "=========================================================\n";
    std::cout << "      EXHAUSTIVE OVERBOOKING TESTS - LOCK-FREE\n";
    std::cout << "=========================================================\n";
    
    testLinearOverbookingSequential();
    testLinearOverbookingBatch();
    testConcurrentOverbookingSameSeat();
    testConcurrentOverbookingAllSeats();
    testConcurrentOverbookingRandom();
    testConcurrentOverbookingOverlappingBatches();
    testStressMaximumConcurrency();
    
    std::cout << "\n=========================================================\n";
    std::cout << "                    FINAL RESULTS\n";
    std::cout << "=========================================================\n";
    std::cout << "Passed: " << OverbookingTests::passed << "\n";
    std::cout << "Failed: " << OverbookingTests::failed << "\n";
    std::cout << "Total:  " << (OverbookingTests::passed + OverbookingTests::failed) << "\n";
    std::cout << "=========================================================\n";
    
    if (OverbookingTests::failed == 0) {
        std::cout << "\n✅ ALL OVERBOOKING TESTS PASSED!\n";
        std::cout << "   NO OVERBOOKING DETECTED IN ANY SCENARIO\n\n";
    } else {
        std::cout << "\n❌ SOME TESTS FAILED - OVERBOOKING DETECTED!\n\n";
    }
    
    return OverbookingTests::failed > 0 ? 1 : 0;
}
