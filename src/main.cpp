#include "BookingService.h"
#include <iostream>
#include <string>
#include <sstream>
#include <limits>

/**
 * @brief CLI for booking service
 */
class BookingCLI {
public:
    BookingCLI() : service_(), running_(true) {
        initializeSampleData();
    }
    
    void run() {
        std::cout << "=================================\n";
        std::cout << "Movie Booking System              \n";
        std::cout << "=================================\n\n";
        
        while (running_) {
            printMenu();
            handleUserInput();
        }
    }

private:
    BookingService service_;
    bool running_;
    
    void initializeSampleData() {
          // Add theaters
        service_.addTheater(std::make_shared<Theater>(1, "VOX Cinemas - Mall of the Emirates (Dubai)"));
        service_.addTheater(std::make_shared<Theater>(2, "Reel Cinemas - Dubai Mall"));
        service_.addTheater(std::make_shared<Theater>(3, "Novo Cinemas - IMG Worlds of Adventure"));
        // service_.addTheater(std::make_shared<Theater>(4, "Cinema City AFI Cotroceni"));
        // service_.addTheater(std::make_shared<Theater>(5, "Cinema City Mega Mall"));
        // service_.addTheater(std::make_shared<Theater>(6, "Happy Cinema Colosseum"));

        // Add movies
        service_.addMovie(std::make_shared<Movie>(1, "Mission: Impossible – Dead Reckoning"));
        service_.addMovie(std::make_shared<Movie>(2, "Dune: Part Two"));
        service_.addMovie(std::make_shared<Movie>(3, "Oppenheimer"));
        service_.addMovie(std::make_shared<Movie>(4, "Avatar: The Way of Water"));
        //service_.addMovie(std::make_shared<Movie>(1, "The Matrix Resurrections"));
        
        // Link movies to theaters
        service_.linkMovieToTheater(1, 1);
        service_.linkMovieToTheater(1, 2);
        service_.linkMovieToTheater(2, 1);
        service_.linkMovieToTheater(2, 3);
        service_.linkMovieToTheater(3, 2);
        service_.linkMovieToTheater(3, 3);
        service_.linkMovieToTheater(4, 1);
        service_.linkMovieToTheater(4, 2);
        service_.linkMovieToTheater(4, 3);
    }
    
    void printMenu() {
        std::cout << "\n--- Main Menu ---\n";
        std::cout << "1. View all movies\n";
        std::cout << "2. Select movie and view theaters\n";
        std::cout << "3. View available seats\n";
        std::cout << "4. Book seats \n";
        std::cout << "5. View booking details\n";
        std::cout << "6. View occupancy statistics\n";
        std::cout << "7. Exit\n";
        std::cout << "\nEnter choice: ";
    }
    
    void handleUserInput() {
        int choice;
        std::cin >> choice;
        
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nInvalid option!\n";
            return;
        }
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        switch (choice) {
            case 1:
                viewAllMovies();
                break;
            case 2:
                selectMovieAndViewTheaters();
                break;
            case 3:
                viewAvailableSeats();
                break;
            case 4:
                bookSeats();
                break;
            case 5:
                viewBookingDetails();
                break;
            case 6:
                viewOccupancyStats();
                break;
            case 7:
                running_ = false;
                std::cout << "\nThank you for using the booking system!\n";
                break;
            default:
                std::cout << "\nInvalid choice. Please try again.\n";
        }
    }
    
    void viewAllMovies() {
        auto movies = service_.getAllMovies();
        
        std::cout << "\n--- All Movies ---\n";
        for (const auto& movie : movies) {
            std::cout << "ID: " << movie->id 
                      << " | Title: " << movie->title << " \n";
        }
    }
    
    void selectMovieAndViewTheaters() {
        std::cout << "\nEnter Movie ID: ";
        uint32_t movieId;
        std::cin >> movieId;
        
        auto movie = service_.getMovie(movieId);
        if (!movie) {
            std::cout << "Movie not found!\n";
            return;
        }
        
        std::cout << "\nMovie: " << movie->title << "\n";
        
        auto theaters = service_.getTheatersForMovie(movieId);
        
        if (theaters.empty()) {
            std::cout << "No theaters showing this movie.\n";
            return;
        }
        
        std::cout << "\n--- Theaters ---\n";
        for (const auto& theater : theaters) {
            std::cout << "ID: " << theater->id 
                      << " | Name: " << theater->name << "\n";
        }
    }
    
    void viewAvailableSeats() {
        std::cout << "\nEnter Movie ID: ";
        uint32_t movieId;
        std::cin >> movieId;
        
        std::cout << "Enter Theater ID: ";
        uint32_t theaterId;
        std::cin >> theaterId;
        
        auto seats = service_.getAvailableSeats(movieId, theaterId);
        uint32_t count = service_.getAvailableCount(movieId, theaterId);
        
        std::cout << "\n--- Available Seats ---\n";
        std::cout << "Total available: " << count << " seats\n";
        
        if (seats.empty()) {
            std::cout << "No seats available!\n";
            return;
        }
        
        std::cout << "Seats: ";
        for (size_t i = 0; i < seats.size(); ++i) {
            std::cout << seats[i];
            if (i < seats.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    
    void bookSeats() {
        std::cout << "\nEnter Movie ID: ";
        uint32_t movieId;
        std::cin >> movieId;
        
        std::cout << "Enter Theater ID: ";
        uint32_t theaterId;
        std::cin >> theaterId;
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        std::cout << "Enter seat IDs (comma-separated, e.g., a1,a2,a3): ";
        std::string seatsInput;
        std::getline(std::cin, seatsInput);
        
        // Parse seats
        std::vector<std::string> seats;
        std::stringstream ss(seatsInput);
        std::string seatId;
        
        while (std::getline(ss, seatId, ',')) {
            // Trim whitespace
            seatId.erase(0, seatId.find_first_not_of(" \t"));
            seatId.erase(seatId.find_last_not_of(" \t") + 1);
            
            if (!seatId.empty()) {
                seats.push_back(seatId);
            }
        }
        
        if (seats.empty()) {
            std::cout << "No seats specified!\n";
            return;
        }
        
        // LOCK-FREE BOOKING!
        auto booking = service_.bookSeats(movieId, theaterId, seats);
        
        if (booking) {
            std::cout << "\n✓ Booking successful! (Lock-Free)\n";
            std::cout << "Booking ID: " << booking->bookingId << "\n";
            std::cout << "Seats booked: ";
            for (size_t i = 0; i < booking->seats.size(); ++i) {
                std::cout << booking->seats[i];
                if (i < booking->seats.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "\n";
        } else {
            std::cout << "\n✗ Booking failed! One or more seats already booked.\n";
        }
    }
    
    void viewBookingDetails() {
        std::cout << "\nEnter Booking ID: ";
        uint32_t bookingId;
        std::cin >> bookingId;
        
        auto booking = service_.getBooking(bookingId);
        
        if (!booking) {
            std::cout << "Booking not found!\n";
            return;
        }
        
        std::cout << "\n--- Booking Details ---\n";
        std::cout << "Booking ID: " << booking->bookingId << "\n";
        std::cout << "Movie ID: " << booking->movieId << "\n";
        std::cout << "Theater ID: " << booking->theaterId << "\n";
        std::cout << "Seats: ";
        for (size_t i = 0; i < booking->seats.size(); ++i) {
            std::cout << booking->seats[i];
            if (i < booking->seats.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    
    void viewOccupancyStats() {
        std::cout << "\nEnter Movie ID: ";
        uint32_t movieId;
        std::cin >> movieId;
        
        std::cout << "Enter Theater ID: ";
        uint32_t theaterId;
        std::cin >> theaterId;
        
        uint32_t available = service_.getAvailableCount(movieId, theaterId);
        double occupancy = service_.getOccupancyPercentage(movieId, theaterId);
        
        std::cout << "\n--- Statistics ---\n";
        std::cout << "Available seats: " << available << " / 20\n";
        std::cout << "Occupied seats: " << (20 - available) << " / 20\n";
        std::cout << "Occupancy: " << occupancy << "%\n";
    }
};

int main() {
    BookingCLI cli;
    cli.run();
    return 0;
}
