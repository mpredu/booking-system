#include "SeatBitmask.h"
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>


int SeatBitmask::seatIdToBit(const std::string& seatId) {
    if (seatId.length() < 2 || seatId.length() > 3) {
        return -1;
    }
    
    // Case-insensitive check for 'a'
    if (std::tolower(seatId[0]) != 'a') {
        return -1;
    }
    
    // Extract number portion
    std::string numStr = seatId.substr(1);
    
    // Validate it's all digits
    if (!std::all_of(numStr.begin(), numStr.end(), ::isdigit)) {
        return -1;
    }
    
    // Reject leading zeros (except "0" itself, but we reject 0 anyway)
    if (numStr.length() > 1 && numStr[0] == '0') {
        return -1;
    }
    
    try {
        int num = std::stoi(numStr);
        if (num >= 1 && num <= static_cast<int>(MAX_SEATS)) {
            return seatNumberToBit(num);
        }
    } catch (const std::invalid_argument&) {
        return -1;
    } catch (const std::out_of_range&) {
        return -1;
    }
    
    return -1;
}

std::string SeatBitmask::bitToSeatId(uint32_t bit) {
    if (bit >= MAX_SEATS) {
        return "";
    }
    return "a" + std::to_string(bitToSeatNumber(bit));
}

uint32_t SeatBitmask::createMask(const std::vector<std::string>& seatIds) {
    uint32_t mask = 0;
    
    for (const auto& seatId : seatIds) {
        int bit = seatIdToBit(seatId);
        if (bit >= 0 && bit < 32) {
            mask |= (1u << bit);
        }
    }
    
    return mask;
}

// Attempt to book atomically using Compare-And-Swap (CAS)
bool SeatBitmask::tryBook(uint32_t seatMask) {
    uint32_t expected = 0;
    uint32_t desired = 0;

    constexpr uint32_t MAX_RETRIES = 100;

    // CAS failed → backoff
    for (uint32_t retries = 0; retries < MAX_RETRIES; ++retries) {

        // Explicitly reload current state
        expected = occupied_.load(std::memory_order_acquire);

        // Check if any desired seat is already occupied
        if ((expected & seatMask) != 0) {
            // At least one seat is already occupied
            return false;
        }

        desired = expected | seatMask;

        // Attempt CAS
        if (occupied_.compare_exchange_weak(
                expected,
                desired,
                std::memory_order_release,
                std::memory_order_acquire)) {
            return true;
        }

         // Fairness / backoff
        std::this_thread::yield();
        std::this_thread::sleep_for(
            std::chrono::nanoseconds(50 * (retries + 1))
        );
    }

    return false;
}


bool SeatBitmask::areAvailable(uint32_t seatMask) const {
    uint32_t current = occupied_.load(std::memory_order_acquire);
    // Seats are available if none of their bits are set
    return (current & seatMask) == 0;
}

std::vector<std::string> SeatBitmask::getAvailableSeats() const {
    std::vector<std::string> available;
    uint32_t current = occupied_.load(std::memory_order_acquire);
    
    for (uint32_t bit = 0; bit < MAX_SEATS; ++bit) {
        if ((current & (1u << bit)) == 0) {
            available.push_back(bitToSeatId(bit));
        }
    }
    
    return available;
}

uint32_t SeatBitmask::getAvailableCount() const {
    uint32_t current = occupied_.load(std::memory_order_acquire);
    // Count set bits and subtract from total
    uint32_t occupiedCount = __builtin_popcount(current & ALL_SEATS_MASK);
    return MAX_SEATS - occupiedCount;
}

bool SeatBitmask::isValidSeatId(const std::string& seatId) {
    return seatIdToBit(seatId) >= 0;
}
