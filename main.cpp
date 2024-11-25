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


// Function prototypes
void initializePlayer();
void displayMenu();
void addSong(vector<Song>& playlist);
void displayPlaylist(const vector<Song>& playlist, int currentSong = -1);
void removeSong(vector<Song>& playlist);
void playSong(const Song& song, bool& shouldExit);
void displayProgress(sf::Music& music, const Song& song, bool isPaused, float& volume);
void clearScreen() {
    system("CLS");
}
string getProgressBar(float percentage, bool isPaused);
void savePlaylist(const vector<Song>& playlist);
void loadPlaylist(vector<Song>& playlist);
bool validateAudioFile(const string& filepath);
void editSong(vector<Song>& playlist);
void searchSongs(const vector<Song>& playlist);
void displayNowPlaying(const Song& song, bool isPaused, ostream& out);
void handleResize(int sig);
pair<int, int> getTerminalSize();
void displayError(const string& message);
void displaySuccess(const string& message);
void displayInfo(const string& message);
string formatDuration(float seconds);
void sortPlaylist(vector<Song>& playlist);
string centerText(const string& text, int width);
void displayHelp();
void displayLogo();

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

int main()
{
    initializePlayer();
    SetConsoleOutputCP(CP_UTF8);
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
                // removeSong(playlist);
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
                        playSong(playlist[index - 1], shouldExit);
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



void addSong(vector<Song>& playlist)
{
    system("CLS");
    cout << MAGENTA << BOLD << "╔════════════════════════════════════╗\n";
    cout << "║          🎵 Add New Song 🎵          ║\n";
    cout << "╚════════════════════════════════════╝" << RESET << "\n\n";

    Song song;

    // Enter song title
    cout << CYAN << "🔹 Enter song title: " << RESET;
    getline(cin, song.title);

    // Default artist (can be adjusted if user input is needed)
    song.artist = "BTS";

    // Enter album name
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

    // Enter audio filepath
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


void playSong(const Song& song, bool& shouldExit)
{
    sf::Music music;
    if (!music.openFromFile(song.filepath))
    {
        displayError("Error loading music file!");
        return;
    }

    music.play();
    bool isPaused = false;
    float currentVolume = 100.0f; // Changed to double to match displayProgress
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

void displayProgress(sf::Music& music, const Song& song, bool isPaused, float& volume)
{
    static string lastDisplay = "";
    auto [width, height] = getTerminalSize();

    float duration = music.getDuration().asSeconds();
    float currentTime = music.getPlayingOffset().asSeconds();
    float percentage = (currentTime / duration) * 100;
    int volumeInt = static_cast<int>(volume);

    stringstream ss;

    // Now Playing
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
        "⏯ Space: Play/Pause",
        "⏹ Q: Stop",
        "🔁 R: Restart",
        "⏪⏩ <,>: Seek",
        "🔈🔊 -,+: Volume",
        "❌ ESC: Exit"
    };

    for (const auto& control : controls)
    {
        ss << CYAN << "- " << control << RESET << "\n";
    }

    // Update the terminal
    string currentDisplay = ss.str();
    if (currentDisplay != lastDisplay)
    {
        cout << "\033[3;1H"; // Move cursor to line 3
        cout << "\033[J";    // Clear from cursor to end of screen
        cout << currentDisplay << flush;
        lastDisplay = currentDisplay;
    }
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

// ... [Remaining function implementations] ...

void savePlaylist(const vector<Song>& playlist)
{
    ofstream file("playlist_data/playlist.dat", ios::binary);
    size_t size = playlist.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& song : playlist)
    {
        size_t titleLen = song.title.length();
        size_t artistLen = song.artist.length();
        size_t filepathLen = song.filepath.length();
        size_t albumLen = song.album.length();

        file.write(reinterpret_cast<const char*>(&titleLen), sizeof(titleLen));
        file.write(song.title.c_str(), titleLen);
        file.write(reinterpret_cast<const char*>(&artistLen), sizeof(artistLen));
        file.write(song.artist.c_str(), artistLen);
        file.write(reinterpret_cast<const char*>(&filepathLen), sizeof(filepathLen));
        file.write(song.filepath.c_str(), filepathLen);
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

        file.read(reinterpret_cast<char*>(&albumLen), sizeof(albumLen));
        song.album.resize(albumLen);
        file.read(&song.album[0], albumLen);

        file.read(reinterpret_cast<char*>(&song.year), sizeof(song.year));
        file.read(reinterpret_cast<char*>(&song.duration), sizeof(song.duration));

        playlist.push_back(song);
    }
}

bool validateAudioFile(const string& filepath)
{
    if (!fs::exists(filepath)) return false;

    string extension = fs::path(filepath).extension().string();
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    vector<string> validExtensions = {".wav", ".ogg", ".flac", ".mp3"};
    return find(validExtensions.begin(), validExtensions.end(), extension) != validExtensions.end();
}

void editSong(vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("Playlist is empty!");
        return;
    }

    displayPlaylist(playlist);
    cout << "\n" << CYAN << "Enter song number to edit (0 to cancel): " << RESET;
    int index;
    cin >> index;
    cin.ignore();

    if (index <= 0 || index > playlist.size())
    {
        return;
    }

    Song& song = playlist[index - 1];
    clearScreen();
    cout << MAGENTA << BOLD << "=== Edit Song ===" << RESET << "\n\n";
    cout << "Current details:\n";
    cout << CYAN << "1. Title: " << WHITE << song.title << "\n";
    cout << CYAN << "2. Album: " << WHITE << song.album << "\n";
    cout << CYAN << "3. Year: " << WHITE << song.year << "\n";
    cout << CYAN << "4. Filepath: " << WHITE << song.filepath << "\n" << RESET;

    cout << "\n" << CYAN << "Enter field number to edit (0 to finish): " << RESET;
    int field;
    cin >> field;
    cin.ignore();

    switch (field)
    {
        case 1:
            cout << CYAN << "Enter new title: " << RESET;
            getline(cin, song.title);
            break;
        case 2:
            cout << CYAN << "Enter new album: " << RESET;
            getline(cin, song.album);
            break;
        case 3:
            cout << CYAN << "Enter new year: " << RESET;
            cin >> song.year;
            break;
        case 4:
            while (true) {
                cout << CYAN << "Enter new filepath: " << RESET;
                getline(cin, song.filepath);
                if (validateAudioFile(song.filepath))
                {
                    // Update duration
                    sf::Music music;
                    if (music.openFromFile(song.filepath))
                    {
                        song.duration = music.getDuration().asSeconds();
                    }
                    break;
                }
                displayError("Invalid audio file! Please check the path and try again.");
            }
            break;
    }
    displaySuccess("Song updated successfully!");
}

void searchSongs(const vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("Playlist is empty!");
        return;
    }

    clearScreen();
    cout << MAGENTA << BOLD << "=== Search Songs ===" << RESET << "\n\n";
    cout << CYAN << "Enter search term: " << RESET;
    string searchTerm;
    getline(cin, searchTerm);
    transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);

    vector<pair<size_t, Song>> matches;
    for (size_t i = 0; i < playlist.size(); ++i)
    {
        string title = playlist[i].title;
        string album = playlist[i].album;
        transform(title.begin(), title.end(), title.begin(), ::tolower);
        transform(album.begin(), album.end(), album.begin(), ::tolower);

        if (title.find(searchTerm) != string::npos ||
            album.find(searchTerm) != string::npos)
        {
            matches.push_back({i, playlist[i]});
        }
    }

    if (matches.empty())
    {
        displayError("No matches found!");
        return;
    }

    cout << "\nSearch results:\n";
    for (const auto& [index, song] : matches)
    {
        cout << CYAN << index + 1 << ". " << WHITE << song.title
             << BLUE << " (" << song.album << ")" << RESET << "\n";
    }
    cout << "\nPress Enter to continue...";
    cin.get();
}

void sortPlaylist(vector<Song>& playlist)
{
    if (playlist.empty())
    {
        displayError("Playlist is empty!");
        return;
    }

    clearScreen();
    cout << MAGENTA << BOLD << "=== Sort Playlist ===" << RESET << "\n\n";
    cout << "Sort by:\n";
    cout << CYAN << "1. Title\n";
    cout << "2. Album\n";
    cout << "3. Year\n";
    cout << "4. Duration\n" << RESET;

    cout << "\n" << CYAN << "Enter choice: " << RESET;
    int choice;
    cin >> choice;

    switch (choice)
    {
        case 1:
            sort(playlist.begin(), playlist.end(),
                [](const Song& a, const Song& b) { return a.title < b.title; });
            break;
        case 2:
            sort(playlist.begin(), playlist.end(),
                [](const Song& a, const Song& b) { return a.album < b.album; });
            break;
        case 3:
            sort(playlist.begin(), playlist.end(),
                [](const Song& a, const Song& b) { return a.year < b.year; });
            break;
        case 4:
            sort(playlist.begin(), playlist.end(),
                [](const Song& a, const Song& b) { return a.duration < b.duration; });
            break;
        default:
            displayError("Invalid choice!");
            return;
    }
}

pair<int, int> getTerminalSize()
{
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return {
            csbi.srWindow.Right - csbi.srWindow.Left + 1,
            csbi.srWindow.Bottom - csbi.srWindow.Top + 1
        };
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return {w.ws_col, w.ws_row};
    #endif
}

string centerText(const string& text, int width)
{
    // Strip ANSI codes for length calculation
    string stripped = text;
    size_t pos = 0;
    while ((pos = stripped.find("\033[", pos)) != string::npos)
    {
        size_t end = stripped.find("m", pos);
        if (end != string::npos)
        {
            stripped.erase(pos, end - pos + 1);
        }
    }

    int padding = (width - stripped.length()) / 2;
    return string(max(0, padding), ' ') + text;
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

void displayHelp()
{
    clearScreen();
    cout << MAGENTA << BOLD << "=== BTS Music Player Help ===" << RESET << "\n\n";

    cout << CYAN << "Navigation Controls:" << RESET << "\n";
    cout << "• Use number keys to select menu options\n";
    cout << "• Press Enter to confirm selections\n\n";

    cout << CYAN << "Playback Controls:" << RESET << "\n";
    cout << "• Space: Play/Pause\n";
    cout << "• Q: Stop current song\n";
    cout << "• R: Restart song\n";
    cout << "• <: Rewind 5 seconds\n";
    cout << "• >: Forward 5 seconds\n";
    cout << "• ESC: Exit to main menu\n\n";

    cout << CYAN << "File Management:" << RESET << "\n";
    cout << "• Supported formats: .wav, .ogg, .flac, .mp3\n";
    cout << "• Playlist is automatically saved\n";
    cout << "• Use absolute paths or relative paths from program directory\n\n";

    cout << "\nPress Enter to return to menu..." << RESET;
    cin.get();
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

void handleResize(int sig)
{
    // Clear screen and redraw interface
    clearScreen();
    displayLogo();
    displayMenu();
}


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

void displayLogo()
{
    cout << MAGENTA << BOLD;
    cout << R"(
    ╔═══════════════════════════════════════════════╗
    ║     ♪♫•*¨*•.¸¸♫♪ BTS PLAYER ♪♫•*¨*•.¸¸♫♪      ║
    ╚═══════════════════════════════════════════════╝
)" << RESET << endl;
}

void displayMenu()
{
    string menu[] = {
        CYAN + "1. " + GREEN + "Add Song" + RESET,
        CYAN + "2. " + GREEN + "View Playlist" + RESET,
        CYAN + "3. " + GREEN + "Remove Song" + RESET,
        CYAN + "4. " + GREEN + "Play Song" + RESET,
        CYAN + "5. " + GREEN + "Edit Song" + RESET,
        CYAN + "6. " + GREEN + "Search Songs" + RESET,
        CYAN + "7. " + GREEN + "Sort Playlist" + RESET,
        CYAN + "8. " + GREEN + "Help" + RESET,
        CYAN + "9. " + RED + "Exit" + RESET
    };

    for (const auto& item : menu)
    {
        cout << item << "\n";
    }
    cout << "\n";
}


void removeSong(vector<Song>& playlist)
{
    clearScreen();

    // Display the current playlist
    cout << MAGENTA << BOLD << "╔════════════════════════════════════════╗\n";
    cout << "║          🎵 Remove a Song 🎵           ║\n";
    cout << "╚════════════════════════════════════════╝" << RESET << "\n\n";

    if (playlist.empty())
    {
        displayInfo("Playlist is empty! There are no songs to remove.");
        return;
    }

    // displayPlaylist()

    // Prompt the user to select a song to remove
    int songNumber;
    while (true)
    {
        cout << CYAN << "\nEnter the song number to remove (0 to cancel): " << RESET;
        cin >> songNumber;

        if (cin.fail() || songNumber < 0 || songNumber > static_cast<int>(playlist.size()))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            displayError("Invalid input! Please enter a valid song number.");
        }
        else if (songNumber == 0)
        {
            displayInfo("Song removal cancelled.");
            return;
        }
        else
        {
            break;
        }
    }

    size_t indexToRemove = static_cast<size_t>(songNumber - 1);
    string removedSongTitle = playlist[indexToRemove].title; // For feedback
    playlist.erase(playlist.begin() + indexToRemove);

    displaySuccess("✅ Successfully removed \"" + removedSongTitle + "\" from the playlist.");

    // Save the updated playlist
    savePlaylist(playlist);
}
