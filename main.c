
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define ROWS 5
#define COLS 8
#define SHOW_COUNT 3
#define DATA_FILE "movie_bookings.dat"

typedef struct {
    char movie_name[60];
    char time[20];
    float price;
    int seat_matrix[ROWS][COLS]; /* 0 = free, otherwise booking ID */
} Show;

typedef struct Booking {
    int id;
    int show_index;
    int row;
    int col;
    float price;
    int active;
    struct Booking *next;
} Booking;

Show *shows = NULL;
Booking *bookings = NULL;
int nextBookingId = 1001;

/* Read an integer safely from the keyboard. */
int readInt(const char *prompt) {
    char line[100], *end;
    long value;

    while (1) {
        printf("%s", prompt);

        if (fgets(line, sizeof(line), stdin) == NULL)
            exit(0);

        errno = 0;
        value = strtol(line, &end, 10);

        while (*end == ' ' || *end == '\t' ||
               *end == '\n' || *end == '\r')
            end++;

        if (end != line && *end == '\0' &&
            errno == 0 && value >= INT_MIN &&
            value <= INT_MAX)
            return (int)value;

        printf("Invalid input. Please enter an integer.\n");
    }
}

/* Allocate and initialize the movie show list. */
void initializeShows(void) {
    shows = malloc(SHOW_COUNT * sizeof(Show));

    if (shows == NULL) {
        printf("Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(shows, 0, SHOW_COUNT * sizeof(Show));

    strcpy(shows[0].movie_name, "Avengers: Endgame");
    strcpy(shows[0].time, "10:00 AM");
    shows[0].price = 150.0f;

    strcpy(shows[1].movie_name, "Interstellar");
    strcpy(shows[1].time, "02:00 PM");
    shows[1].price = 180.0f;

    strcpy(shows[2].movie_name, "Inception");
    strcpy(shows[2].time, "06:00 PM");
    shows[2].price = 220.0f;
}

/* Find a booking by its unique ID. */
Booking *findBooking(int id) {
    Booking *current = bookings;

    while (current != NULL) {
        if (current->id == id)
            return current;

        current = current->next;
    }

    return NULL;
}

/* Save all booking records, including cancellations. */
void saveData(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    Booking *current = bookings;

    if (fp == NULL) {
        printf("Warning: Could not save booking data.\n");
        return;
    }

    while (current != NULL) {
        if (fwrite(current, sizeof(Booking), 1, fp) != 1) {
            printf("Warning: Error writing booking data.\n");
            break;
        }

        current = current->next;
    }

    fclose(fp);
}

/* Restore saved bookings and their seat occupancy. */
void loadData(void) {
    FILE *fp = fopen(DATA_FILE, "rb");

    if (fp == NULL)
        return; /* First run: no saved data yet. */

    while (1) {
        Booking record;

        if (fread(&record, sizeof(Booking), 1, fp) != 1)
            break;

        if (record.show_index < 0 ||
            record.show_index >= SHOW_COUNT ||
            record.row < 0 || record.row >= ROWS ||
            record.col < 0 || record.col >= COLS ||
            record.id <= 0)
            continue;

        Booking *b = malloc(sizeof(Booking));

        if (b == NULL) {
            printf("Not enough memory to load bookings.\n");
            break;
        }

        b->id = record.id;
        b->show_index = record.show_index;
        b->row = record.row;
        b->col = record.col;
        b->price = shows[b->show_index].price;
        b->active = record.active == 1;
        b->next = bookings;
        bookings = b;

        if (b->id >= nextBookingId)
            nextBookingId = b->id + 1;

        if (b->active) {
            /* Keep the first valid active booking for a seat. */
            if (shows[b->show_index]
                    .seat_matrix[b->row][b->col] == 0) {
                shows[b->show_index]
                    .seat_matrix[b->row][b->col] = b->id;
            } else {
                b->active = 0;
            }
        }
    }

    fclose(fp);
}

/* Display all available movie shows. */
void displayShows(void) {
    int i;

    printf("\n========== AVAILABLE MOVIES ==========\n");

    for (i = 0; i < SHOW_COUNT; i++) {
        printf("%d. %-22s | %s | Rs. %.2f\n",
               i + 1, shows[i].movie_name,
               shows[i].time, shows[i].price);
    }
}

/* Display seats: O = available, X = booked. */
void displaySeats(int showIndex) {
    int r, c;

    printf("\nMovie: %s (%s)\n",
           shows[showIndex].movie_name,
           shows[showIndex].time);

    printf("O = Available, X = Booked\n\n");
    printf("       ");

    for (c = 0; c < COLS; c++)
        printf("%2d ", c + 1);

    printf("\n");

    for (r = 0; r < ROWS; r++) {
        printf("Row %d: ", r + 1);

        for (c = 0; c < COLS; c++) {
            if (shows[showIndex].seat_matrix[r][c] == 0)
                printf(" O ");
            else
                printf(" X ");
        }

        printf("\n");
    }
}

/* Book one seat in the selected show. */
void bookTicket(void) {
    int choice, row, col;
    Booking *b;

    displayShows();

    choice = readInt("Select movie number: ");

    if (choice < 1 || choice > SHOW_COUNT) {
        printf("Invalid movie selection.\n");
        return;
    }

    int index = choice - 1;
    displaySeats(index);

    row = readInt("Enter row number (1-5): ");
    col = readInt("Enter seat number (1-8): ");

    if (row < 1 || row > ROWS ||
        col < 1 || col > COLS) {
        printf("Invalid seat number.\n");
        return;
    }

    row--;
    col--;

    if (shows[index].seat_matrix[row][col] != 0) {
        printf("Sorry, this seat is already booked.\n");
        return;
    }

    printf("Movie: %s\n", shows[index].movie_name);
    printf("Showtime: %s\n", shows[index].time);
    printf("Ticket price: Rs. %.2f\n", shows[index].price);

    int confirm = readInt("Confirm booking? (1 = Yes, 0 = No): ");

    if (confirm != 1) {
        printf("Booking cancelled before confirmation.\n");
        return;
    }

    b = malloc(sizeof(Booking));

    if (b == NULL) {
        printf("Memory allocation failed. Booking not completed.\n");
        return;
    }

    b->id = nextBookingId++;
    b->show_index = index;
    b->row = row;
    b->col = col;
    b->price = shows[index].price;
    b->active = 1;

    b->next = bookings;
    bookings = b;

    shows[index].seat_matrix[row][col] = b->id;

    saveData();

    printf("\nBooking successful!\n");
    printf("Booking ID : %d\n", b->id);
    printf("Movie      : %s\n", shows[index].movie_name);
    printf("Showtime   : %s\n", shows[index].time);
    printf("Seat       : Row %d, Seat %d\n", row + 1, col + 1);
    printf("Amount     : Rs. %.2f\n", b->price);
}

/* Cancel a ticket using its booking ID. */
void cancelTicket(void) {
    int id;
    Booking *b;

    id = readInt("Enter booking ID to cancel: ");
    b = findBooking(id);

    if (b == NULL || b->active == 0) {
        printf("Booking not found or already cancelled.\n");
        return;
    }

    int confirm = readInt("Confirm cancellation? (1 = Yes, 0 = No): ");

    if (confirm != 1) {
        printf("Cancellation aborted.\n");
        return;
    }

    shows[b->show_index]
        .seat_matrix[b->row][b->col] = 0;

    b->active = 0;

    saveData();

    printf("Ticket cancelled successfully.\n");
    printf("Booking ID: %d\n", b->id);
    printf("Refund amount: Rs. %.2f\n", b->price);
}

/* Show active bookings, revenue, and daily summary. */
void generateReport(void) {
    int i;
    int totalTickets = 0;
    float totalRevenue = 0.0f;

    printf("\n========== MOVIE REVENUE REPORT ==========\n");

    for (i = 0; i < SHOW_COUNT; i++) {
        int count = 0;
        float revenue = 0.0f;
        Booking *b = bookings;

        while (b != NULL) {
            if (b->active && b->show_index == i) {
                count++;
                revenue += b->price;
            }

            b = b->next;
        }

        printf("\nMovie: %s\n", shows[i].movie_name);
        printf("Showtime: %s\n", shows[i].time);
        printf("Tickets sold: %d\n", count);
        printf("Revenue: Rs. %.2f\n", revenue);

        totalTickets += count;
        totalRevenue += revenue;
    }

    printf("\n========== DAILY SUMMARY ==========\n");
    printf("Total tickets sold: %d\n", totalTickets);
    printf("Total revenue: Rs. %.2f\n", totalRevenue);
}

/* Release all dynamically allocated memory. */
void cleanup(void) {
    Booking *current = bookings;

    while (current != NULL) {
        Booking *temp = current;
        current = current->next;
        free(temp);
    }

    free(shows);
}

int main(void) {
    int choice;

    initializeShows();
    loadData();

    printf("========================================\n");
    printf("   ONLINE MOVIE TICKET RESERVATION\n");
    printf("========================================\n");

    do {
        printf("\n--------------- MAIN MENU ---------------\n");
        printf("1. View Movies and Showtimes\n");
        printf("2. View Available Seats\n");
        printf("3. Book Ticket\n");
        printf("4. Cancel Ticket\n");
        printf("5. Generate Revenue Report\n");
        printf("6. Exit\n");

        choice = readInt("Enter your choice: ");

        switch (choice) {
            case 1:
                displayShows();
                break;

            case 2: {
                displayShows();
                int n = readInt("Select movie number: ");

                if (n >= 1 && n <= SHOW_COUNT)
                    displaySeats(n - 1);
                else
                    printf("Invalid movie selection.\n");

                break;
            }

            case 3:
                bookTicket();
                break;

            case 4:
                cancelTicket();
                break;

            case 5:
                generateReport();
                break;

            case 6:
                saveData();
                printf("Thank you for using the system!\n");
                break;

            default:
                printf("Invalid choice. Please try again.\n");
        }

    } while (choice != 6);

    cleanup();
    return 0;
}
