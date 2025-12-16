#ifndef LOCK_FREE_BOOKING_SERVICE_H
#define LOCK_FREE_BOOKING_SERVICE_H

#include "SeatBitmask.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <shared_mutex>

/**
 * @brief Movie entity - simple, without thread-safety 
 */
struct Movie {
    uint32_t id;
    std::string title;
   
    Movie(uint32_t id_, const std::string& title_)
        : id(id_), title(title_) {}
};

/**
 * @brief Theater entity - simple, without thread-safety 
 */
struct Theater {
    uint32_t id;
    std::string name;
    
    Theater(uint32_t id_, const std::string& name_)
        : id(id_), name(name_) {}
};

/**
 * @brief Booking record - result of a booking
 */
struct Booking {
    uint64_t bookingId;
    uint32_t movieId;
    uint32_t theaterId;
    std::vector<std::string> seats;
    
    Booking(uint32_t bid, uint32_t mid, uint32_t tid, const std::vector<std::string>& s)
        : bookingId(bid), movieId(mid), theaterId(tid), seats(s) {}
};

/**
 * @brief Lock-free booking service using bitmasks
 * 
 * Features:
 * - Seat booking is LOCK-FREE (uses atomic CAS)
 * - Metadata (movies, theaters) uses shared_mutex for read-heavy workloads
 * - Each (movie, theater) combination has its own atomic SeatBitmask
 * - Superior performance for concurrent operations
 */
class BookingService {
public:
    BookingService();
    
    // ===== Movie Operations =====
    
    /**
     * @brief Adds a movie (thread-safe)
     */
    void addMovie(std::shared_ptr<Movie> movie);
    
    /**
     * @brief Gets all movies (thread-safe)
     */
    std::vector<std::shared_ptr<Movie>> getAllMovies() const;
    
    /**
     * @brief Gets a movie by ID (thread-safe)
     */
    std::shared_ptr<Movie> getMovie(uint32_t movieId) const;
    
    // ===== Theater Operations =====
    
    /**
     * @brief Adds a theater (thread-safe)
     */
    void addTheater(std::shared_ptr<Theater> theater);
    
    /**
     * @brief Links a movie to a theater (thread-safe)
     */
    bool linkMovieToTheater(uint32_t movieId, uint32_t theaterId);
    
    /**
     * @brief Gets theaters for a movie (thread-safe)
     */
    std::vector<std::shared_ptr<Theater>> getTheatersForMovie(uint32_t movieId) const;
    
    // ===== Seat Operations (LOCK-FREE!) =====
    
    /**
     * @brief Gets available seats (lock-free read)
     * 
     * This operation is lock-free and extremely fast.
     */
    std::vector<std::string> getAvailableSeats(uint32_t movieId, uint32_t theaterId) const;
    
    /**
     * @brief Gets number of available seats (lock-free)
     */
    uint32_t getAvailableCount(uint32_t movieId, uint32_t theaterId) const;
    
    /**
     * @brief Books seats - LOCK-FREE OPERATION!
     * 
     * Uses Compare-And-Swap (CAS) for atomic booking.
     * Multiple threads can book simultaneously without blocking.
     * 
     * @param movieId Movie ID
     * @param theaterId Theater ID
     * @param seatIds Vector of seat IDs (e.g., ["a1", "a5"])
     * @return Booking if successful, nullptr if failed
     */
    std::shared_ptr<Booking> bookSeats(uint32_t movieId, uint32_t theaterId,
                                       const std::vector<std::string>& seatIds);
    
    /**
     * @brief Gets a booking by ID (thread-safe)
     */
    std::shared_ptr<Booking> getBooking(uint64_t bookingId) const;
    
    // ===== Statistics =====
    
    /**
     * @brief Gets occupancy percentage (lock-free)
     */
    double getOccupancyPercentage(uint32_t movieId, uint32_t theaterId) const;

private:
    // Metadata (uses shared_mutex for read-heavy access)
    mutable std::shared_mutex metadataMutex_;
    std::map<uint32_t, std::shared_ptr<Movie>> movies_;
    std::map<uint32_t, std::shared_ptr<Theater>> theaters_;
    std::map<uint32_t, std::vector<uint32_t>> movieToTheaters_;
    
    // Seat bitmasks - LOCK-FREE!
    // Key: pair<movieId, theaterId>
    // Value: atomic SeatBitmask
    mutable std::shared_mutex seatsMutex_;  // Only for map access, not for booking!
    std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<SeatBitmask>> seatMasks_;
    
    // Bookings storage (lock for map access, but booking itself is lock-free)
    mutable std::shared_mutex bookingsMutex_;
    std::map<uint32_t, std::shared_ptr<Booking>> bookings_;
    std::atomic<uint64_t> nextBookingId_;
    
    // Helper methods
    std::shared_ptr<SeatBitmask> getSeatMask(uint32_t movieId, uint32_t theaterId) const;
    std::shared_ptr<SeatBitmask> getOrCreateSeatMask(uint32_t movieId, uint32_t theaterId);
};

#endif // LOCK_FREE_BOOKING_SERVICE_H
