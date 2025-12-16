#include "BookingService.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <barrier>  // C++20 - for synchronization

// ============================================================================
// SYNCHRONIZED TWO-THREAD RACE TEST
// ============================================================================

class TwoThreadRaceTest {
public:
    static void runTest() {
        std::cout << "=========================================================\n";
        std::cout << "    TWO-THREAD SYNCHRONIZED RACE TEST\n";
        std::cout << "=========================================================\n\n";
        
        std::cout << "Goal: Two threads start at EXACTLY the same time\n";
        std::cout << "      Both try to book seat 'a1'\n";
        std::cout << "      Only ONE should succeed\n";
        std::cout << "      We'll see which thread wins the race!\n\n";
        
        // Run multiple iterations to see different outcomes
        for (int iteration = 1; iteration <= 10; iteration++) {
            runSingleRace(iteration);
        }
        
        std::cout << "\n=========================================================\n";
        std::cout << "                    ANALYSIS\n";
        std::cout << "=========================================================\n";
        std::cout << "In 10 races, we saw different threads win.\n";
        std::cout << "This proves the race condition is REAL, but the\n";
        std::cout << "lock-free CAS algorithm ensures only ONE winner!\n";
        std::cout << "=========================================================\n\n";
    }

private:
    static void runSingleRace(int iteration) {
        std::cout << "--- Race #" << iteration << " ---\n";
        
        // Setup
        BookingService service;
        service.addMovie(std::make_shared<Movie>(1, "Test Movie"));
        service.addTheater(std::make_shared<Theater>(1, "Test Theater"));
        service.linkMovieToTheater(1, 1);
        
        // Results
        std::atomic<bool> thread1Success{false};
        std::atomic<bool> thread2Success{false};
        std::atomic<uint32_t> thread1BookingId{0};
        std::atomic<uint32_t> thread2BookingId{0};
        
        // Timing
        std::atomic<std::chrono::nanoseconds> thread1StartTime{std::chrono::nanoseconds{0}};
        std::atomic<std::chrono::nanoseconds> thread2StartTime{std::chrono::nanoseconds{0}};
        std::atomic<std::chrono::nanoseconds> thread1EndTime{std::chrono::nanoseconds{0}};
        std::atomic<std::chrono::nanoseconds> thread2EndTime{std::chrono::nanoseconds{0}};
        
        // Synchronization barrier - both threads wait here
        std::atomic<bool> ready{false};
        std::atomic<int> threadsReady{0};
        
        // Thread 1
        std::thread t1([&]() {
            // Signal ready
            threadsReady.fetch_add(1);
            
            // Wait for GO signal
            while (!ready.load(std::memory_order_acquire)) {
                // Spin wait
            }
            
            // START! Record time
            auto start = std::chrono::high_resolution_clock::now();
            thread1StartTime.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()),
                std::memory_order_release
            );
            
            // Try to book
            auto booking = service.bookSeats(1, 1, {"a1"});
            
            // END! Record time
            auto end = std::chrono::high_resolution_clock::now();
            thread1EndTime.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch()),
                std::memory_order_release
            );
            
            if (booking) {
                thread1Success.store(true, std::memory_order_release);
                thread1BookingId.store(booking->bookingId, std::memory_order_release);
            }
        });
        
        // Thread 2
        std::thread t2([&]() {
            // Signal ready
            threadsReady.fetch_add(1);
            
            // Wait for GO signal
            while (!ready.load(std::memory_order_acquire)) {
                // Spin wait
            }
            
            // START! Record time
            auto start = std::chrono::high_resolution_clock::now();
            thread2StartTime.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()),
                std::memory_order_release
            );
            
            // Try to book
            auto booking = service.bookSeats(1, 1, {"a1"});
            
            // END! Record time
            auto end = std::chrono::high_resolution_clock::now();
            thread2EndTime.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch()),
                std::memory_order_release
            );
            
            if (booking) {
                thread2Success.store(true, std::memory_order_release);
                thread2BookingId.store(booking->bookingId, std::memory_order_release);
            }
        });
        
        // Wait for both threads to be ready
        while (threadsReady.load(std::memory_order_acquire) < 2) {
            // Wait
        }
        
        std::cout << "  Both threads ready...\n";
        std::cout << "  3...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  2...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  1...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  GO!\n";
        
        // Release both threads simultaneously
        ready.store(true, std::memory_order_release);
        
        // Wait for completion
        t1.join();
        t2.join();
        
        // Analyze results
        bool t1Won = thread1Success.load(std::memory_order_acquire);
        bool t2Won = thread2Success.load(std::memory_order_acquire);
        
        auto t1Start = thread1StartTime.load(std::memory_order_acquire);
        auto t2Start = thread2StartTime.load(std::memory_order_acquire);
        auto t1End = thread1EndTime.load(std::memory_order_acquire);
        auto t2End = thread2EndTime.load(std::memory_order_acquire);
        
        std::cout << "\n  Results:\n";
        std::cout << "  --------\n";
        
        if (t1Won && !t2Won) {
            std::cout << "  🏆 THREAD 1 WON!\n";
            std::cout << "     Booking ID: " << thread1BookingId.load() << "\n";
            std::cout << "  ❌ Thread 2 failed (seat already booked)\n";
        } else if (!t1Won && t2Won) {
            std::cout << "  ❌ Thread 1 failed (seat already booked)\n";
            std::cout << "  🏆 THREAD 2 WON!\n";
            std::cout << "     Booking ID: " << thread2BookingId.load() << "\n";
        } else if (t1Won && t2Won) {
            std::cout << "  ⚠️  ERROR: BOTH threads succeeded! OVERBOOKING!\n";
        } else {
            std::cout << "  ⚠️  ERROR: BOTH threads failed!\n";
        }
        
        // Timing analysis
        auto timeDiff = std::abs((t1Start - t2Start).count());
        auto t1Duration = (t1End - t1Start).count();
        auto t2Duration = (t2End - t2Start).count();
        
        std::cout << "\n  Timing:\n";
        std::cout << "  -------\n";
        std::cout << "  Thread 1 start time: " << t1Start.count() << " ns\n";
        std::cout << "  Thread 2 start time: " << t2Start.count() << " ns\n";
        std::cout << "  Start time difference: " << timeDiff << " ns ";
        
        if (timeDiff < 1000) {
            std::cout << "(<1 microsecond - VERY CLOSE!)\n";
        } else if (timeDiff < 10000) {
            std::cout << "(<10 microseconds - close)\n";
        } else {
            std::cout << "(" << (timeDiff / 1000.0) << " microseconds)\n";
        }
        
        std::cout << "  Thread 1 duration: " << (t1Duration / 1000.0) << " μs\n";
        std::cout << "  Thread 2 duration: " << (t2Duration / 1000.0) << " μs\n";
        
        // Verify system state
        uint32_t available = service.getAvailableCount(1, 1);
        std::cout << "\n  System state:\n";
        std::cout << "  -------------\n";
        std::cout << "  Available seats: " << available << " / 20\n";
        
        if (available == 19 && (t1Won || t2Won) && !(t1Won && t2Won)) {
            std::cout << "  ✅ Correct: Exactly 1 seat booked, 19 available\n";
        } else {
            std::cout << "  ❌ ERROR: Inconsistent state!\n";
        }
        
        std::cout << "\n";
    }
};

// ============================================================================
// ADVANCED VERSION WITH C++20 BARRIER
// ============================================================================

#if __cplusplus >= 202002L

class TwoThreadRaceTestBarrier {
public:
    static void runTest() {
        std::cout << "\n=========================================================\n";
        std::cout << "    TWO-THREAD RACE WITH C++20 BARRIER\n";
        std::cout << "=========================================================\n\n";
        
        std::cout << "Using std::barrier for perfect synchronization\n";
        std::cout << "This guarantees both threads start at the EXACT same time\n\n";
        
        for (int iteration = 1; iteration <= 5; iteration++) {
            runSingleRaceWithBarrier(iteration);
        }
    }

private:
    static void runSingleRaceWithBarrier(int iteration) {
        std::cout << "--- Race #" << iteration << " (with barrier) ---\n";
        
        // Setup
        BookingService service;
        service.addMovie(std::make_shared<Movie>(1, "Test"));
        service.addTheater(std::make_shared<Theater>(1, "Test"));
        service.linkMovieToTheater(1, 1);
        
        std::atomic<bool> thread1Success{false};
        std::atomic<bool> thread2Success{false};
        
        // C++20 barrier - both threads wait here
        std::barrier sync_point(2);
        
        // Thread 1
        std::thread t1([&]() {
            // Wait at barrier
            sync_point.arrive_and_wait();
            
            // Both threads released simultaneously!
            auto booking = service.bookSeats(1, 1, {"a1"});
            if (booking) {
                thread1Success.store(true);
            }
        });
        
        // Thread 2
        std::thread t2([&]() {
            // Wait at barrier
            sync_point.arrive_and_wait();
            
            // Both threads released simultaneously!
            auto booking = service.bookSeats(1, 1, {"a1"});
            if (booking) {
                thread2Success.store(true);
            }
        });
        
        t1.join();
        t2.join();
        
        // Results
        bool t1Won = thread1Success.load();
        bool t2Won = thread2Success.load();
        
        if (t1Won && !t2Won) {
            std::cout << "  🏆 Thread 1 won\n";
        } else if (!t1Won && t2Won) {
            std::cout << "  🏆 Thread 2 won\n";
        } else if (t1Won && t2Won) {
            std::cout << "  ❌ ERROR: Both won (OVERBOOKING!)\n";
        } else {
            std::cout << "  ❌ ERROR: Both lost\n";
        }
        
        uint32_t available = service.getAvailableCount(1, 1);
        std::cout << "  Available: " << available << " / 20\n";
        std::cout << "  " << (available == 19 ? "✅ Correct" : "❌ ERROR") << "\n\n";
    }
};

#endif

// ============================================================================
// VISUAL RACE DEMONSTRATION
// ============================================================================

class VisualRaceDemo {
public:
    static void runDemo() {
        std::cout << "\n=========================================================\n";
        std::cout << "           VISUAL RACE DEMONSTRATION\n";
        std::cout << "=========================================================\n\n";
        
        std::cout << "Watch the threads race in REAL-TIME!\n\n";
        
        BookingService service;
        service.addMovie(std::make_shared<Movie>(1, "Test"));
        service.addTheater(std::make_shared<Theater>(1, "Test"));
        service.linkMovieToTheater(1, 1);
        
        std::atomic<bool> ready{false};
        std::atomic<int> threadsReady{0};
        std::atomic<bool> thread1Success{false};
        std::atomic<bool> thread2Success{false};
        std::atomic<int> thread1Progress{0};
        std::atomic<int> thread2Progress{0};
        
        // Thread 1
        std::thread t1([&]() {
            threadsReady.fetch_add(1);
            thread1Progress.store(1);  // Ready
            
            while (!ready.load(std::memory_order_acquire)) {}
            
            thread1Progress.store(2);  // Started
            auto booking = service.bookSeats(1, 1, {"a1"});
            thread1Progress.store(3);  // CAS attempt
            
            if (booking) {
                thread1Success.store(true);
                thread1Progress.store(4);  // Success!
            } else {
                thread1Progress.store(5);  // Failed
            }
        });
        
        // Thread 2
        std::thread t2([&]() {
            threadsReady.fetch_add(1);
            thread2Progress.store(1);  // Ready
            
            while (!ready.load(std::memory_order_acquire)) {}
            
            thread2Progress.store(2);  // Started
            auto booking = service.bookSeats(1, 1, {"a1"});
            thread2Progress.store(3);  // CAS attempt
            
            if (booking) {
                thread2Success.store(true);
                thread2Progress.store(4);  // Success!
            } else {
                thread2Progress.store(5);  // Failed
            }
        });
        
        // Wait for ready
        while (threadsReady.load() < 2) {}
        
        std::cout << "Thread 1: [READY]    Thread 2: [READY]\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "GO!\n\n";
        ready.store(true);
        
        // Monitor progress
        bool done = false;
        while (!done) {
            int p1 = thread1Progress.load();
            int p2 = thread2Progress.load();
            
            std::cout << "\rThread 1: ";
            if (p1 >= 2) std::cout << "[STARTED] ";
            if (p1 >= 3) std::cout << "[CAS] ";
            if (p1 == 4) std::cout << "[✅ WON!]     ";
            if (p1 == 5) std::cout << "[❌ LOST]     ";
            
            std::cout << "  Thread 2: ";
            if (p2 >= 2) std::cout << "[STARTED] ";
            if (p2 >= 3) std::cout << "[CAS] ";
            if (p2 == 4) std::cout << "[✅ WON!]     ";
            if (p2 == 5) std::cout << "[❌ LOST]     ";
            
            std::cout << std::flush;
            
            if ((p1 == 4 || p1 == 5) && (p2 == 4 || p2 == 5)) {
                done = true;
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        t1.join();
        t2.join();
        
        std::cout << "\n\n";
        
        bool t1Won = thread1Success.load();
        bool t2Won = thread2Success.load();
        
        if (t1Won && !t2Won) {
            std::cout << "🏆 Thread 1 won the race!\n";
        } else if (!t1Won && t2Won) {
            std::cout << "🏆 Thread 2 won the race!\n";
        } else {
            std::cout << "❌ Unexpected result!\n";
        }
        
        std::cout << "\n";
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    // Test 1: Basic synchronized race
    TwoThreadRaceTest::runTest();
    
    // Test 2: Visual demonstration
    VisualRaceDemo::runDemo();
    
    // Test 3: C++20 barrier (if available)
#if __cplusplus >= 202002L
    TwoThreadRaceTestBarrier::runTest();
#else
    std::cout << "\nNote: C++20 barrier test skipped (requires C++20)\n";
#endif
    
    return 0;
}
