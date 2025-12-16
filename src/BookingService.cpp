#include "BookingService.h"
#include <algorithm>
#include <mutex>

BookingService::BookingService() : nextBookingId_(1) {
}

// ===== Movie Operations =====

void BookingService::addMovie(std::shared_ptr<Movie> movie) {
    std::unique_lock<std::shared_mutex> lock(metadataMutex_);
    movies_[movie->id] = movie;
}

std::vector<std::shared_ptr<Movie>> BookingService::getAllMovies() const {
    std::shared_lock<std::shared_mutex> lock(metadataMutex_);
    std::vector<std::shared_ptr<Movie>> result;
    result.reserve(movies_.size());
    
    for (const auto& pair : movies_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::shared_ptr<Movie> BookingService::getMovie(uint32_t movieId) const {
    std::shared_lock<std::shared_mutex> lock(metadataMutex_);
    auto it = movies_.find(movieId);
    return (it != movies_.end()) ? it->second : nullptr;
}

// ===== Theater Operations =====

void BookingService::addTheater(std::shared_ptr<Theater> theater) {
    std::unique_lock<std::shared_mutex> lock(metadataMutex_);
    theaters_[theater->id] = theater;
}

bool BookingService::linkMovieToTheater(uint32_t movieId, uint32_t theaterId) {
    std::unique_lock<std::shared_mutex> lock(metadataMutex_);
    
    // Check that movie and theater exist
    if (movies_.find(movieId) == movies_.end() || 
        theaters_.find(theaterId) == theaters_.end()) {
        return false;
    }
    
    // Add link
    movieToTheaters_[movieId].push_back(theaterId);
    
    return true;
}

std::vector<std::shared_ptr<Theater>> BookingService::getTheatersForMovie(uint32_t movieId) const {
    std::shared_lock<std::shared_mutex> lock(metadataMutex_);
    std::vector<std::shared_ptr<Theater>> result;
    
    auto it = movieToTheaters_.find(movieId);
    if (it != movieToTheaters_.end()) {
        for (uint32_t theaterId : it->second) {
            auto theaterIt = theaters_.find(theaterId);
            if (theaterIt != theaters_.end()) {
                result.push_back(theaterIt->second);
            }
        }
    }
    
    return result;
}

// ===== Seat Operations (LOCK-FREE!) =====

std::shared_ptr<SeatBitmask> BookingService::getSeatMask(
    uint32_t movieId, uint32_t theaterId) const {
    
    std::shared_lock<std::shared_mutex> lock(seatsMutex_);
    auto key = std::make_pair(movieId, theaterId);
    auto it = seatMasks_.find(key);
    return (it != seatMasks_.end()) ? it->second : nullptr;
}

std::shared_ptr<SeatBitmask> BookingService::getOrCreateSeatMask(
    uint32_t movieId, uint32_t theaterId) {
    
    auto key = std::make_pair(movieId, theaterId);
    
    // Try read first (shared lock)
    {
        std::shared_lock<std::shared_mutex> lock(seatsMutex_);
        auto it = seatMasks_.find(key);
        if (it != seatMasks_.end()) {
            return it->second;
        }
    }
    
    // Need to create - upgrade to unique lock
    std::unique_lock<std::shared_mutex> lock(seatsMutex_);
    
    // Double-check (might have been created between locks)
    auto it = seatMasks_.find(key);
    if (it != seatMasks_.end()) {
        return it->second;
    }
    
    // Create new
    auto mask = std::make_shared<SeatBitmask>();
    seatMasks_[key] = mask;
    return mask;
}

std::vector<std::string> BookingService::getAvailableSeats(
    uint32_t movieId, uint32_t theaterId) const {
    
    auto mask = getSeatMask(movieId, theaterId);
    if (!mask) {
        // No bookings yet - all seats available
        std::vector<std::string> all;
        for (uint32_t i = 1; i <= SeatBitmask::MAX_SEATS; ++i) {
            all.push_back("a" + std::to_string(i));
        }
        return all;
    }
    
    // LOCK-FREE READ!
    return mask->getAvailableSeats();
}

uint32_t BookingService::getAvailableCount(
    uint32_t movieId, uint32_t theaterId) const {
    
    auto mask = getSeatMask(movieId, theaterId);
    if (!mask) {
        return SeatBitmask::MAX_SEATS;
    }
    
    // LOCK-FREE READ!
    return mask->getAvailableCount();
}

std::shared_ptr<Booking> BookingService::bookSeats(
    uint32_t movieId, uint32_t theaterId,
    const std::vector<std::string>& seatIds) {
    
    // Validate seat IDs
    if (seatIds.empty()) {
        return nullptr;
    }
    
    for (const auto& seatId : seatIds) {
        if (!SeatBitmask::isValidSeatId(seatId)) {
            return nullptr;
        }
    }
    
    // Check that movie and theater exist and are linked
    {
        std::shared_lock<std::shared_mutex> lock(metadataMutex_);
        
        if (movies_.find(movieId) == movies_.end() || 
            theaters_.find(theaterId) == theaters_.end()) {
            return nullptr;
        }
        
        auto it = movieToTheaters_.find(movieId);
        if (it == movieToTheaters_.end()) {
            return nullptr;
        }
        
        bool found = false;
        for (uint32_t tid : it->second) {
            if (tid == theaterId) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return nullptr;
        }
    }
    
    // Create bitmask for requested seats
    uint32_t seatMask = SeatBitmask::createMask(seatIds);
    if (seatMask == 0) {
        return nullptr;  // Invalid seat IDs
    }
    
    // Get or create seat mask for this combination
    auto currentSeatBitmask = getOrCreateSeatMask(movieId, theaterId);
    
    // LOCK-FREE BOOKING! Uses atomic CAS
    if (!currentSeatBitmask->tryBook(seatMask)) {
        return nullptr;  // At least one seat was already occupied
    }
    
    // Booking succeeded! Create the record
    uint32_t bookingId = nextBookingId_.fetch_add(1, std::memory_order_relaxed);
    auto booking = std::make_shared<Booking>(bookingId, movieId, theaterId, seatIds);
    
    // Save booking
    {
        std::unique_lock<std::shared_mutex> lock(bookingsMutex_);
        bookings_[bookingId] = booking;
    }
    
    return booking;
}

std::shared_ptr<Booking> BookingService::getBooking(uint64_t bookingId) const {
    std::shared_lock<std::shared_mutex> lock(bookingsMutex_);
    auto it = bookings_.find(bookingId);
    return (it != bookings_.end()) ? it->second : nullptr;
}

double BookingService::getOccupancyPercentage(
    uint32_t movieId, uint32_t theaterId) const {
    
    uint32_t available = getAvailableCount(movieId, theaterId);
    uint32_t occupied = SeatBitmask::MAX_SEATS - available;
    
    return (static_cast<double>(occupied) / SeatBitmask::MAX_SEATS) * 100.0;
}
