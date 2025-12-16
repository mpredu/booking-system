#ifndef SEAT_BITMASK_H
#define SEAT_BITMASK_H

#include <cstdint>
#include <atomic>
#include <vector>
#include <string>

/**
 * @brief Lock-free seat representation using bitmask
 * 
 * Each seat is represented by a bit in uint32_t:
 * - Bit 0 = seat a1
 * - Bit 1 = seat a2
 * - ...
 * - Bit 19 = seat a20
 * 
 * We use std::atomic<uint32_t> for lock-free operations.
 * Booking is done through Compare-And-Swap (CAS).
 */
class SeatBitmask {
public:
    static constexpr uint32_t MAX_SEATS = 20;
    
    /**
     * @brief Constructor - all seats available
     */
    SeatBitmask() : occupied_(0) {}
    
    /**
     * @brief Converts seat number (1-20) to bit position (0-19)
     */
    static uint32_t seatNumberToBit(uint32_t seatNum) {
        return seatNum - 1;
    }
    
    /**
     * @brief Converts bit position (0-19) to seat number (1-20)
     */
    static uint32_t bitToSeatNumber(uint32_t bit) {
        return bit + 1;
    }
    
    /**
     * @brief Converts seat ID (e.g., "a5") to bit position
     * @return Bit position (0-19) or -1 if invalid
     */
    static int seatIdToBit(const std::string& seatId);
    
    /**
     * @brief Converts bit position to seat ID (e.g., "a5")
     */
    static std::string bitToSeatId(uint32_t bit);
    
    /**
     * @brief Creates bitmask for list of seat IDs
     * @param seatIds Vector of seat IDs (e.g., ["a1", "a5", "a10"])
     * @return Bitmask with corresponding bits set
     */
    static uint32_t createMask(const std::vector<std::string>& seatIds);
    
    /**
     * @brief Attempts to book specified seats (LOCK-FREE)
     * 
     * This method is thread-safe and lock-free.
     * Uses Compare-And-Swap to guarantee atomicity.
     * 
     * @param seatMask Bitmask with desired seats
     * @return true if all seats were booked, false if at least one was occupied
     */
    bool tryBook(uint32_t seatMask);
    
    /**
     * @brief Checks if seats are available (lock-free read)
     * @param seatMask Bitmask with seats to check
     * @return true if ALL seats are available
     */
    bool areAvailable(uint32_t seatMask) const;
    
    /**
     * @brief Gets current bitmask of occupied seats (lock-free)
     */
    uint32_t getOccupied() const {
        return occupied_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Gets list of available seat IDs
     */
    std::vector<std::string> getAvailableSeats() const;
    
    /**
     * @brief Gets number of available seats
     */
    uint32_t getAvailableCount() const;
    
    /**
     * @brief Validates that seat ID is valid (a1-a20)
     */
    static bool isValidSeatId(const std::string& seatId);

private:
    // Atomic bitmask: each bit = one seat (0 = available, 1 = occupied)
    std::atomic<uint32_t> occupied_;
    
    // Mask for all 20 seats
    static constexpr uint32_t ALL_SEATS_MASK = (1u << MAX_SEATS) - 1;
};

#endif // SEAT_BITMASK_H
