#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <conio.h>
#include <windows.h>
#include <regex>
#include <sstream>


using namespace std;
namespace fs = std::filesystem;

// ANSI Color Codes
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string BLACK = "\033[30m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE = "\033[34m";
const string MAGENTA = "\033[35m";
const string CYAN = "\033[36m";
const string WHITE = "\033[37m";
const string BG_BLACK = "\033[40m";
const string BG_RED = "\033[41m";
const string BG_GREEN = "\033[42m";
const string BG_YELLOW = "\033[43m";
const string BG_BLUE = "\033[44m";
const string BG_MAGENTA = "\033[45m";
const string BG_CYAN = "\033[46m";
const string BG_WHITE = "\033[47m";

// Structure to store song information
struct Song
{
    string title;
    string artist;
    string filepath;
    string album;
    int year;
    float duration;
};


// Music player functions
void initializePlayer();
void playSong(const Song& song, bool& shouldExit, bool repeat);
void editSong(vector<Song>& playlist);
void searchSongs(const vector<Song>& playlist);
void sortPlaylist(vector<Song>& playlist);
void addSong(vector<Song>& playlist);
void removeSong(vector<Song>& playlist);

// Helper functions
string getProgressBar(float percentage, bool isPaused);
bool validateAudioFile(string& filepath);
string formatDuration(float seconds);
string toLower(const string& str);
void handleResize(int sig);

// Utility code
void clearScreen();
void hideCursor();
void showCursor();
int get_int(string prompt);

// UI functions
void displayMenu();
void displayPlaylist(const vector<Song>& playlist, int currentSong = -1);
void displayProgress(sf::Music& music, const Song& song, bool isPaused, float& volume);
void displayError(const string& message);
void displaySuccess(const string& message);
void displayInfo(const string& message);
void displayHelp();
void displayLogo();
void displayNowPlaying(const Song& song, bool isPaused, ostream& out);


// File I/O
void savePlaylist(const vector<Song>& playlist);
void loadPlaylist(vector<Song>& playlist);


int main()
{
    initializePlayer();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitle("BTS Music Player");
    vector<Song> playlist;
    char choice;
    bool isPlaying = false;
    bool shouldExit = false;

    // Load saved playlist
    loadPlaylist(playlist);

    while (!shouldExit)
    {
        system("CLS");
        displayLogo();
        displayMenu();
        int choice = get_int("Choice: ");

        switch (choice)
        {
            case 1: // Add Song
                addSong(playlist);
                savePlaylist(playlist);
                break;
            case 2: // View Playlist
                displayPlaylist(playlist);
                cout << "\n" << CYAN << "Press Enter to continue..." << RESET;
                cin.get();
                break;
            case 3: // Remove Song
                removeSong(playlist);
                savePlaylist(playlist);
                break;
            case 4: // Play Song
                if (!playlist.empty())
                {
                    displayPlaylist(playlist);
                    int index = get_int("Enter song number to play (0 to cancel): ");
                    if (index > 0 && index <= playlist.size())
                    {
                        hideCursor();
                        playSong(playlist[index - 1], shouldExit, false);
                    }
                }
                else
                {
                    displayError("Playlist is empty!");
                }
                showCursor();
                break;
            case 5: // Edit Song
                editSong(playlist);
                savePlaylist(playlist);
                break;
            case 6: // Search Songs
                searchSongs(playlist);
                break;
            case 7: // Sort Playlist
                sortPlaylist(playlist);
                savePlaylist(playlist);
                displaySuccess("Playlist sorted successfully!");
                break;
            case 8: // Help
                displayHelp();
                break;
            case 9: // Exit
                shouldExit = true;
                break;
            default:
                displayError("Invalid choice!");
        }
    }

    cout << MAGENTA << "\nThank you for using BTS Music Player! 안녕히 가세요!\n" << RESET;
    return 0;
}


// Player functions
void initializePlayer()
{
    // Set up terminal
    #ifdef _WIN32
        system("color");
    #else
        // Handle terminal resize for Unix-like systems
        signal(SIGWINCH, handleResize);
    #endif

    // Create save directory if it doesn't exist
    fs::create_directories("playlist_data");
}

void playSong(const Song& song, bool& shouldExit, bool repeat)
{
    sf::Music music;
    if (!music.openFromFile(song.filepath))
    {
        displayError("Error loading music file!");
        return;
    }

    music.play();
    bool isPaused = false;
    float currentVolume = 100.0f;
    music.setVolume(static_cast<float>(currentVolume));

    clearScreen();

    auto lastUpdateTime = chrono::steady_clock::now();
    while (music.getStatus() != sf::Music::Stopped && !shouldExit)
    {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - lastUpdateTime).count() >= 100)
        {
            displayProgress(music, song, isPaused, currentVolume);
            lastUpdateTime = now;
        }

        if (music.getStatus() == sf::Music::Stopped)
        {
            if (repeat)
            {
                music.setPlayingOffset(sf::Time::Zero);
                music.play();
            }
            else
            {
                // If not repeating, just exit the function to move to next song
                return;
            }
        }

        if (_kbhit())
        {
            char input = _getch();
            bool needsRedraw = false;
            switch (input)
            {
                case ' ': // Pause/Resume
                    if (isPaused)
                        music.play();
                    else
                        music.pause();
                    isPaused = !isPaused;
                    needsRedraw = true;
                    break;
                case 'q': // Stop
                    music.stop();
                    return;
                case 27: // ESC - Exit program
                    shouldExit = true;
                    music.stop();
                    return;
                case 'r': // Restart
                    music.setPlayingOffset(sf::Time::Zero);
                    needsRedraw = true;
                    break;
                case '>': // Forward 5 seconds
                    music.setPlayingOffset(music.getPlayingOffset() + sf::seconds(5.f));
                    needsRedraw = true;
                    break;
                case '<': // Backward 5 seconds
                    {
                        sf::Time newTime = music.getPlayingOffset() - sf::seconds(5.f);
                        music.setPlayingOffset(newTime < sf::Time::Zero ? sf::Time::Zero : newTime);
                        needsRedraw = true;
                    }
                    break;
                case '+': // Volume up
                case '=': // Alternative volume up
                    currentVolume = min(currentVolume + 5.0, 100.0);
                    music.setVolume(static_cast<float>(currentVolume));
                    needsRedraw = true;
                    break;
                case '-': // Volume down
                    currentVolume = max(currentVolume - 5.0, 0.0);
                    music.setVolume(static_cast<float>(currentVolume));
                    needsRedraw = true;
                    break;
                case 'l': // Toggle repeat
                    repeat = !repeat;
                    needsRedraw = true;
                    break;
            }
            if (needsRedraw)
            {
                displayProgress(music, song, isPaused, currentVolume);
                lastUpdateTime = now;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void editSong(vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("🎵 The playlist is empty! Add songs to edit.");
        return;
    }

    displayPlaylist(playlist);
    int index = get_int("🎶 Enter song number to edit (0 to cancel): ");

    if (index == 0)
    {
        displayInfo("❌ Edit canceled.");
        return;
    }
    if (index < 1 || index > static_cast<int>(playlist.size()))
    {
        displayError("🚫 Invalid song number!");
        return;
    }

    Song& song = playlist[index - 1];

    clearScreen();
    cout << MAGENTA << "    ╔═══════════════════════════════════════════════╗\n";
    cout << "    ║           🎶 Edit Song 🎶                     ║\n";
    cout << "    ╚═══════════════════════════════════════════════╝" << RESET << "\n\n";

    // Display current song details
    cout << "🎶 Current details:\n";
    cout << CYAN << "1. Title:    " << WHITE << song.title << "\n";
    cout << CYAN << "2. Album:    " << WHITE << song.album << "\n";
    cout << CYAN << "3. Year:     " << WHITE << song.year << "\n";
    cout << CYAN << "4. Filepath: " << WHITE << song.filepath << RESET << "\n\n";

    while (true)
    {
        int field = get_int("🎶 Enter field number to edit (0 to finish): ");

        // Handle finishing edits
        if (field == 0)
        {
            displaySuccess("✔️ All changes saved!");
            break;
        }

        switch (field)
        {
            case 1: // Edit title
                cout << CYAN << "🎶 Enter new title: " << RESET;
                getline(cin, song.title);
                displaySuccess("✔️ Title updated successfully!");
                break;

            case 2: // Edit album
                cout << CYAN << "🎶 Enter new album: " << RESET;
                getline(cin, song.album);
                displaySuccess("✔️ Album updated successfully!");
                break;

            case 3: // Edit year
                song.year = get_int("🎶 Enter new year: ");
                if (song.year < 1800 || song.year > 2100)
                {
                    displayError("🚫 Invalid year! Please enter a valid year.");
                    continue;
                }
                displaySuccess("✔️ Year updated successfully!");
                break;

            case 4:
                while (true)
                {
                    cout << CYAN << "🎶 Enter new filepath: " << RESET;
                    getline(cin, song.filepath);
                    if (validateAudioFile(song.filepath))
                    {
                        // Update duration
                        sf::Music music;
                        if (music.openFromFile(song.filepath))
                        {
                            song.duration = music.getDuration().asSeconds();
                        }
                        displaySuccess("✔️ Filepath and duration updated successfully!");
                        break;
                    }
                    displayError("🚫 Invalid audio file! Please check the path and try again.");
                }
                break;

            default:
                displayError("🚫 Invalid option! Please enter a number between 1 and 4.");
        }
    }
}

void searchSongs(const vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("🎵 Playlist is empty!");
        return;
    }

    string searchTerm;
    vector<Song> filteredSongs;

    while (true)
    {
        clearScreen();
        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║           🎶 Search Songs 🎶             ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n' << '\n';

        // Get the search term from the user
        cout << YELLOW << "Enter search term (press Esc to exit): " << RESET;
        cout << searchTerm;

        // Perform the search if the term is not empty
        if (!searchTerm.empty())
        {
            filteredSongs.clear();
            string lowerSearchTerm = toLower(searchTerm);
            for (const auto& song : playlist)
            {
                string title = toLower(song.title);
                string album = toLower(song.album);

                if (title.find(lowerSearchTerm) != string::npos || album.find(lowerSearchTerm) != string::npos)
                {
                    filteredSongs.push_back(song);
                }
            }

            // Display results
            if (!filteredSongs.empty())
            {
                cout << "\n\n";
                cout << CYAN << "┌────────────────────┬────────────────────┐" << RESET << endl;
                cout << CYAN << "│ " << WHITE << setw(18) << left << "Title"
                     << CYAN << " │ " << WHITE << setw(18) << left << "Album" << BLUE << " │" << RESET << endl;
                cout << CYAN << "├────────────────────┼────────────────────┤" << RESET << endl;

                for (const auto& song : filteredSongs)
                {
                    cout << CYAN << "│ " << WHITE << setw(18) << left << song.title
                         << CYAN << " │ " << WHITE << setw(18) << left << song.album << BLUE << " │" << RESET << endl;
                }

                cout << CYAN << "└────────────────────┴────────────────────┘" << RESET << endl;
            }
            else
            {
                cout << "\n\n" << YELLOW << "No matching songs found." << RESET << endl;
            }
        }

        // Handle user input for search term
        char ch = _getch();

        if (ch == 27) // Escape key to exit
        {
            break;
        }
        else if (ch == 8) // Backspace to remove last character
        {
            if (!searchTerm.empty())
            {
                searchTerm.pop_back();
            }
        }
        else if (isprint(ch)) // Handle printable characters
        {
            searchTerm += ch;
        }
    }
}

void sortPlaylist(vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("🎵 Playlist is empty!");
        return;
    }

    while (true)
    {
        clearScreen();
        cout << BOLD << CYAN << "╔══════════════════════════════════════════╗" << RESET << '\n';
        cout << BOLD << CYAN << "║          🎶 Sort Playlist 🎶             ║" << RESET << '\n';
        cout << BOLD << CYAN << "╚══════════════════════════════════════════╝" << RESET << '\n' << '\n';

        cout << "Sort by:\n";
        cout << CYAN << "1. Title\n";
        cout << "2. Album\n";
        cout << "3. Year\n";
        cout << "4. Duration\n" << RESET;

        int choice = get_int("Enter choice: ");

        switch (choice)
        {
            case 1:
                sort(playlist.begin(), playlist.end(),
                    [](const Song& a, const Song& b) { return a.title < b.title; });
                displaySuccess("Playlist sorted by Title!");
                return;
            case 2:
                sort(playlist.begin(), playlist.end(),
                    [](const Song& a, const Song& b) { return a.album < b.album; });
                displaySuccess("Playlist sorted by Album!");
                return;
            case 3:
                sort(playlist.begin(), playlist.end(),
                    [](const Song& a, const Song& b) { return a.year < b.year; });
                displaySuccess("Playlist sorted by Year!");
                return;
            case 4:
                sort(playlist.begin(), playlist.end(),
                    [](const Song& a, const Song& b) { return a.duration < b.duration; });
                displaySuccess("Playlist sorted by Duration!");
                return;
            default:
                displayError("Invalid choice! Please try again.");
                break;
        }
    }
}

void addSong(vector<Song>& playlist)
{
    system("CLS");
    cout << MAGENTA << BOLD << "╔════════════════════════════════════╗\n";
    cout << "║          🎵 Add New Song 🎵        ║\n";
    cout << "╚════════════════════════════════════╝" << RESET << "\n\n";

    Song song;

    cout << CYAN << "🔹 Enter song title: " << RESET;
    getline(cin, song.title);

    song.artist = "BTS";

    cout << CYAN << "🔹 Enter album name: " << RESET;
    getline(cin, song.album);

    // Enter release year
    while (true)
    {
        cout << CYAN << "🔹 Enter release year: " << RESET;
        cin >> song.year;

        if (cin.fail() || song.year < 1900 || song.year > 2100)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            displayError("Invalid year! Please enter a valid year (1900-2100).");
        }
        else
        {
            cin.ignore();
            break;
        }
    }

    while (true)
    {
        cout << CYAN << "🔹 Enter audio filepath: " << RESET;
        getline(cin, song.filepath);

        if (validateAudioFile(song.filepath))
        {
            break;
        }
        displayError("❌ Invalid audio file! Please check the path and try again.");
    }

    // Get duration using SFML
    sf::Music music;
    if (music.openFromFile(song.filepath))
    {
        song.duration = music.getDuration().asSeconds();
    }
    else
    {
        song.duration = 0.0f;
    }

    playlist.push_back(song);

    displaySuccess("✅ Song added successfully!");
}

void removeSong(vector<Song>& playlist)
{
    clearScreen();

    // Display the current playlist header
    cout << MAGENTA << BOLD << "╔════════════════════════════════════════╗\n";
    cout << "║          🎵 Remove a Song 🎵           ║\n";
    cout << "╚════════════════════════════════════════╝" << RESET << "\n\n";

    if (playlist.empty())
    {
        displayInfo("Playlist is empty! There are no songs to remove.");
        return;
    }

    displayPlaylist(playlist);

    int songNumber;

    while (true)
    {
        songNumber = get_int("\nEnter the song number to remove (0 to cancel): ");

        if (songNumber == 0)
        {
            displayInfo("Song removal cancelled.");
            return;
        }
        else if (songNumber > 0 && songNumber <= static_cast<int>(playlist.size()))
        {
            break;
        }
        else
        {
            displayError("Invalid input! Please enter a valid song number.");
        }
    }

    size_t indexToRemove = static_cast<size_t>(songNumber - 1);
    string removedSongTitle = playlist[indexToRemove].title;

    cout << RED << "\nAre you sure you want to remove \"" << removedSongTitle << "\"? (y/n): " << RESET;
    string confirmation;
    getline(cin, confirmation);

    if (toLower(confirmation) != "y" && toLower(confirmation) != "yes")
    {
        displayInfo("Song removal cancelled.");
        Sleep(2000);
        return;
    }

    playlist.erase(playlist.begin() + indexToRemove);

    displaySuccess("✅ Successfully removed \"" + removedSongTitle + "\" from the playlist.");
    Sleep(2000);

    // Save the updated playlist
    savePlaylist(playlist);
}


// Helper and utility functions

string getProgressBar(float percentage, bool isPaused)
{
    const int barWidth = 50;
    int pos = barWidth * percentage / 100.0f;

    string bar = CYAN + "[" + RESET;

    for (int i = 0; i < barWidth; ++i)
    {
        if (i < pos)
        {
            bar += MAGENTA + "━" + RESET;
        }
        else if (i == pos)
        {
            bar += isPaused ? YELLOW + "◆" + RESET : GREEN + "⬤" + RESET;
        }
        else
        {
            bar += BLUE + "━" + RESET;
        }
    }

    bar += CYAN + "]" + RESET;

    // Use stringstream for formatting percentage
    ostringstream oss;
    oss << YELLOW << fixed << setprecision(1) << percentage << "%" << RESET;
    return bar + " " + oss.str();
}

bool validateAudioFile(string& filepath)
{
    fs::path inputPath(filepath);
    fs::path fullPath;

    // If path is already absolute, use it directly
    if (inputPath.is_absolute())
    {
        fullPath = inputPath;
    }
    else
    {
        // Try multiple potential base paths
        vector<fs::path> searchPaths =
        {
            fs::current_path(),
            fs::current_path() / "playlist_data"
        };

        bool found = false;
        for (const auto& basePath : searchPaths)
        {
            fullPath = basePath / inputPath;
            if (fs::exists(fullPath))
            {
                found = true;
                break;
            }
        }

        // If file not found in any of the search paths, return false
        if (!found)
        {
            return false;
        }
    }

    // Check if file exists and is a regular file
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath))
    {
        return false;
    }

    // Get extension
    string extension = fullPath.extension().string();

    // Validate extension (case-insensitive)
    vector<string> validExtensions = {".wav", ".ogg", ".flac", ".mp3"};
    bool isValid = any_of(validExtensions.begin(), validExtensions.end(),
        [&extension](const string& validExt)
        {
            return equal(validExt.begin(), validExt.end(),
                         extension.begin(), extension.end(),
                         [](char a, char b)
                         {
                             return tolower(a) == tolower(b);
                         });
        });

    if (isValid)
    {
        filepath = fullPath.lexically_normal().string();
    }

    return isValid;
}

string formatDuration(float seconds)
{
    int mins = seconds / 60;
    int secs = (int)seconds % 60;
    stringstream ss;
    ss << setfill('0') << setw(2) << mins << ":"
       << setfill('0') << setw(2) << secs;
    return ss.str();
}

void handleResize(int sig)
{
    // Clear screen and redraw interface
    clearScreen();
    displayLogo();
    displayMenu();
}

string toLower(const string& str)
{
    string lower = str;
    transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return tolower(c); });
    return lower;
}

void clearScreen()
{
    system("CLS");
}

void hideCursor()
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void showCursor()
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

int get_int(string prompt)
{
    regex integer_regex("^-?[0-9]+$");
    string input;

    while (true)
    {
        cout << CYAN << prompt << RESET;
        getline(cin, input);

        if (regex_match(input, integer_regex))
        {
            try
            {
                return stoi(input);
            }
            catch (out_of_range&)
            {
                cout << "Error: Number out of range. Please try again.\n";
            }
        }
        else
        {
            cout << RED << "Invalid input. Please enter a valid integer.\n" << RESET;
        }
    }
}


// UI Functions
void displayMenu()
{
    cout << BLUE << BOLD << "╔══════════════════════════════════════════════════════╗" << RESET << endl;
    cout << BLUE << "║                                                      ║" << RESET << endl;

    string menu[] = {
        CYAN + "1." + RESET + "  " + YELLOW + "Add Song" + RESET,
        CYAN + "2." + RESET + "  " + YELLOW + "View Playlist" + RESET,
        CYAN + "3." + RESET + "  " + YELLOW + "Remove Song" + RESET,
        CYAN + "4." + RESET + "  " + YELLOW + "Play Song" + RESET,
        CYAN + "5." + RESET + "  " + YELLOW + "Edit Song" + RESET,
        CYAN + "6." + RESET + "  " + YELLOW + "Search Songs" + RESET,
        CYAN + "7." + RESET + "  " + YELLOW + "Sort Playlist" + RESET,
        CYAN + "8." + RESET + "  " + YELLOW + "Help" + RESET,
        CYAN + "9." + RESET + "  " + RED + "Exit" + RESET
    };

    for (const auto& item : menu)
    {
        cout << "║ " << item << string(70 - item.length(), ' ') << " ║" << endl;
    }

    cout << BLUE << "║                                                      ║" << RESET << endl;
    cout << BLUE << "╚══════════════════════════════════════════════════════╝" << RESET << endl;
    cout << '\n';
}

void displayPlaylist(const vector<Song>& playlist, int currentSong)
{
    clearScreen();
    cout << MAGENTA << BOLD << "╔══════════════════════════════════════════════════════════════════════╗\n";
    cout << "║                            🎵 Current Playlist 🎵                    ║\n";
    cout << "╚══════════════════════════════════════════════════════════════════════╝" << RESET << "\n\n";

    if (playlist.empty())
    {
        displayInfo("Playlist is empty!");
        return;
    }

    // Table header
    cout << CYAN << "╔════╤──────────────────────────────────────┬──────────────────────┬──────┬──────────╗\n";
    cout << "║ No │ Title                                │ Album                │ Year │ Duration ║\n";
    cout << "╟────┼──────────────────────────────────────┼──────────────────────┼──────┼──────────╢\n";

    // Table content
    for (size_t i = 0; i < playlist.size(); i++)
    {
        string rowColor = (i == currentSong) ? GREEN : WHITE;

        // Truncate text if too long
        string title = playlist[i].title.length() > 30 ? playlist[i].title.substr(0, 27) + "..." : playlist[i].title;
        string album = playlist[i].album.length() > 20 ? playlist[i].album.substr(0, 17) + "..." : playlist[i].album;

        cout << rowColor
             << "║ " << setw(2) << i + 1 << " │ "
             << setw(36) << left << title << " │ "
             << setw(20) << left << album << " │ "
             << setw(4) << right << playlist[i].year << " │ "
             << setw(8) << right << formatDuration(playlist[i].duration) << " ║\n";

        if (i < playlist.size() - 1)
        {
            cout << CYAN << "╟────┼──────────────────────────────────────┼──────────────────────┼──────┼──────────╢\n";
        }
    }

    // Table footer
    cout << CYAN << "╚════╧──────────────────────────────────────┴──────────────────────┴──────┴──────────╝" << RESET << "\n";
}

void displayProgress(sf::Music& music, const Song& song, bool isPaused, float& volume)
{
    static string lastDisplay = "";

    float duration = music.getDuration().asSeconds();
    float currentTime = music.getPlayingOffset().asSeconds();
    float percentage = (currentTime / duration) * 100;
    int volumeInt = static_cast<int>(volume);

    stringstream ss;

    displayNowPlaying(song, isPaused, ss);
    ss << "\n";
    // Progress bar
    string progressBar = getProgressBar(percentage, isPaused);
    ss << YELLOW << progressBar << RESET << "\n";

    // Time and Volume display
    string timeDisplay = formatDuration(currentTime) + " / " + formatDuration(duration);
    string volumeDisplay = "Volume: " + to_string(volumeInt) + "%";
    ss << YELLOW << timeDisplay << RESET << "   " << GREEN << volumeDisplay << RESET << "\n\n";

    // Controls
    ss << CYAN << "Controls:" << RESET << "\n";
    vector<string> controls =
    {
        "⏯  Space: Play/Pause",
        "⏹  Q: Stop",
        "🔁  R: Restart",
        "⏪⏩ <,>: Seek",
        "🔈🔊 -,+: Volume",
        "❌ ESC: Exit Program"
    };

    for (const auto& control : controls)
    {
        ss << CYAN << "- " << control << RESET << "\n";
    }

    string currentDisplay = ss.str();
    if (currentDisplay != lastDisplay)
    {
        cout << "\033[3;1H";
        cout << "\033[J";
        cout << currentDisplay << flush;
        lastDisplay = currentDisplay;
    }
}


void displayError(const string& message)
{
    cout << RED << BOLD << "Error: " << message << RESET << "\n";
    this_thread::sleep_for(chrono::seconds(2));
}

void displaySuccess(const string& message)
{
    cout << GREEN << BOLD << "Success: " << message << RESET << "\n";
    this_thread::sleep_for(chrono::seconds(2));
}

void displayInfo(const string& message)
{
    cout << BLUE << BOLD << "Info: " << message << RESET << "\n";
}

void displayHelp()
{
    clearScreen();

    cout << MAGENTA << BOLD << "╔══════════════════════════════════════════════════════╗" << '\n';
    cout << "║                🎵 BTS Music Player Help 🎵           ║" << '\n';
    cout << "╠══════════════════════════════════════════════════════╣" << RESET << '\n';

    cout << '\n';
    // Navigation Controls Section
    cout << CYAN << BOLD << "Navigation Controls:" << RESET << "\n";
    cout << GREEN << "• " << RESET << "Use number keys to select menu options\n";
    cout << GREEN << "• " << RESET << "Press Enter to confirm selections\n\n";

    // Playback Controls Section
    cout << CYAN << BOLD << "Playback Controls:" << RESET << "\n";
    cout << YELLOW << "• " << RESET << "Space: Play/Pause\n";
    cout << YELLOW << "• " << RESET << "Q: Stop current song\n";
    cout << YELLOW << "• " << RESET << "R: Restart song\n";
    cout << YELLOW << "• " << RESET << "<: Rewind 5 seconds\n";
    cout << YELLOW << "• " << RESET << ">: Forward 5 seconds\n";
    cout << YELLOW << "• " << RESET << "ESC: Exit to main menu\n\n";

    cout << CYAN << BOLD << "Volume Controls:" << RESET << "\n";
    cout << BLUE << "• " << RESET << "-: Decrease volume\n";
    cout << BLUE << "• " << RESET << "+ or /: Increase volume\n\n";

    cout << CYAN << BOLD << "File Management:" << RESET << "\n";
    cout << BLUE << "• " << RESET << "Supported formats: .wav, .ogg, .flac, .mp3\n";
    cout << BLUE << "• " << RESET << "Playlist is automatically saved\n";
    cout << BLUE << "• " << RESET << "Use absolute paths or relative paths from program directory\n\n";

    cout << MAGENTA << BOLD << "╚══════════════════════════════════════════════════════╝" << '\n';
    cout << "\n" << CYAN << "Press Enter to return to menu..." << RESET;
    cin.get();
}

void displayLogo()
{
    cout << MAGENTA << BOLD;
    cout << R"(
    ╔═══════════════════════════════════════════════╗
    ║     ♪♫•*¨*•.¸¸♫♪ BTS PLAYER ♪♫•*¨*•.¸¸♫♪      ║
    ╚═══════════════════════════════════════════════╝
)" << RESET << '\n';
}

void displayNowPlaying(const Song& song, bool isPaused, ostream& out)
{
    // Playing status
    string status = isPaused ? "⏸️ PAUSED" : "▶️ NOW PLAYING";

    // Song details
    string title = song.title + " - " + song.artist;
    string album = "Album: " + song.album + " (" + to_string(song.year) + ")";

    // Display the now-playing information
    out << MAGENTA << BOLD << status << RESET << "\n";
    out << GREEN << BOLD << title << RESET << "\n";
    out << BLUE << album << RESET << "\n";
}

// File I/O
void savePlaylist(const vector<Song>& playlist)
{
    ofstream file("playlist_data/playlist.dat", ios::binary);
    size_t size = playlist.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& song : playlist)
    {
        // Convert absolute path to relative path for storage
        fs::path fullPath(song.filepath);
        fs::path relativePath = fs::proximate(fullPath, fs::current_path());
        string relativePathStr = relativePath.string();

        size_t titleLen = song.title.length();
        size_t artistLen = song.artist.length();
        size_t filepathLen = relativePathStr.length();
        size_t albumLen = song.album.length();

        // Write data using relative path
        file.write(reinterpret_cast<const char*>(&titleLen), sizeof(titleLen));
        file.write(song.title.c_str(), titleLen);
        file.write(reinterpret_cast<const char*>(&artistLen), sizeof(artistLen));
        file.write(song.artist.c_str(), artistLen);
        file.write(reinterpret_cast<const char*>(&filepathLen), sizeof(filepathLen));
        file.write(relativePathStr.c_str(), filepathLen);
        file.write(reinterpret_cast<const char*>(&albumLen), sizeof(albumLen));
        file.write(song.album.c_str(), albumLen);
        file.write(reinterpret_cast<const char*>(&song.year), sizeof(song.year));
        file.write(reinterpret_cast<const char*>(&song.duration), sizeof(song.duration));
    }
}

void loadPlaylist(vector<Song>& playlist)
{
    ifstream file("playlist_data/playlist.dat", ios::binary);
    if (!file) return;

    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));

    for (size_t i = 0; i < size; ++i)
    {
        Song song;
        size_t titleLen, artistLen, filepathLen, albumLen;

        file.read(reinterpret_cast<char*>(&titleLen), sizeof(titleLen));
        song.title.resize(titleLen);
        file.read(&song.title[0], titleLen);

        file.read(reinterpret_cast<char*>(&artistLen), sizeof(artistLen));
        song.artist.resize(artistLen);
        file.read(&song.artist[0], artistLen);

        file.read(reinterpret_cast<char*>(&filepathLen), sizeof(filepathLen));
        song.filepath.resize(filepathLen);
        file.read(&song.filepath[0], filepathLen);

        // Convert stored relative path to absolute path
        fs::path relativePath(song.filepath);
        fs::path fullPath = fs::current_path() / relativePath;
        song.filepath = fullPath.lexically_normal().string();

        file.read(reinterpret_cast<char*>(&albumLen), sizeof(albumLen));
        song.album.resize(albumLen);
        file.read(&song.album[0], albumLen);

        file.read(reinterpret_cast<char*>(&song.year), sizeof(song.year));
        file.read(reinterpret_cast<char*>(&song.duration), sizeof(song.duration));

        playlist.push_back(song);
    }
}
