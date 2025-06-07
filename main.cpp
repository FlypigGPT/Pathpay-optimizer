#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <queue>
#include <limits>
#include <json/json.h>
#include <curl/curl.h>
#include <sstream>

// User management
class UserManager {
private:
    std::string filename = "users.txt";

public:
    // Register a new user
    void registerUser(const std::string& username, const std::string& password, const std::string& role) {
        std::ofstream file(filename, std::ios::app);
        if (!file) {
            std::cerr << "Unable to open user file" << std::endl;
            return;
        }
        file << username << " " << password << " " << role << std::endl;
        std::cout << "Registration successful!" << std::endl;
    }

    // Attempt to log in a user
    std::string loginUser(const std::string& username, const std::string& password) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Unable to open user file" << std::endl;
            return "";
        }

        std::string storedUser, storedPass, role;
        while (file >> storedUser >> storedPass >> role) {
            if (storedUser == username) {
                if (storedPass == password) {
                    std::cout << "Login successful!" << std::endl;
                    return role;
                }
                else {
                    std::cout << "Incorrect password!" << std::endl;
                    return "";
                }
            }
        }
        std::cout << "User does not exist!" << std::endl;
        return "";
    }
};

// Currency exchange rate management
class RateManager {
private:
    std::string apiKey = "RGBaQ6phT9oTNYbWnfsWacN28mahcOJj";

public:
    // Callback function for CURL to handle response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t totalSize = size * nmemb;
        output->append((char*)contents, totalSize);
        return totalSize;
    }

    // Fetch the exchange rate between two currencies from an online API
    double getRateOnline(const std::string& from, const std::string& to) {
        std::string url = "https://api.apilayer.com/currency_data/convert?from=" + from + "&to=" + to + "&amount=1";
        CURL* curl = curl_easy_init();
        std::string readBuffer;

        if (curl) {
            struct curl_slist* headers = NULL;
            std::string authHeader = "apikey: " + apiKey;
            headers = curl_slist_append(headers, authHeader.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
                return -1.0;
            }

            Json::Value jsonData;
            Json::CharReaderBuilder builder;
            std::string errs;
            std::istringstream s(readBuffer);
            if (Json::parseFromStream(builder, s, &jsonData, &errs)) {
                if (jsonData["success"].asBool()) {
                    double rate = jsonData["result"].asDouble();
                    std::cout << "Exchange rate retrieved successfully: " << from << " -> " << to << " = " << rate << std::endl;
                    return rate;
                }
                else {
                    std::cerr << "API returned failure." << std::endl;
                    return -1.0;
                }
            }
            else {
                std::cerr << "Failed to parse JSON: " << errs << std::endl;
                return -1.0;
            }
        }

        return -1.0;
    }
};

// Graph structure and path search
struct Edge {
    std::string to;
    double feeRate;
    std::string fromCurrency;
    std::string toCurrency;
};

class GraphManager {
private:
    std::string edgesFile = "edges.txt";
    std::string institutionsFile = "nodes.txt";
    std::unordered_map<std::string, std::vector<Edge>> graph;

public:
    // Add a new financial institution
    void addInstitution(const std::string& name, const std::string& country) {
        std::ofstream outFile(institutionsFile, std::ios::app);
        if (!outFile) {
            std::cerr << "Unable to open institution file." << std::endl;
            return;
        }
        outFile << name << " " << country << std::endl;
        std::cout << "Institution added successfully." << std::endl;
    }

    // View all registered institutions
    void viewInstitutions() {
        std::ifstream inFile(institutionsFile);
        if (!inFile) {
            std::cerr << "Unable to read institution file." << std::endl;
            return;
        }
        std::string name, country;
        std::cout << "Institution list:" << std::endl;
        while (inFile >> name >> country) {
            std::cout << "Institution: " << name << "Country: " << country << std::endl;
        }
    }

    // Add a new edge (financial flow path)
    void addEdge(const std::string& from, const std::string& to, double feeRate, const std::string& fromCur, const std::string& toCur) {
        std::ofstream outFile(edgesFile, std::ios::app);
        if (!outFile) {
            std::cerr << "Unable to open edge file." << std::endl;
            return;
        }
        outFile << from << " " << to << " " << feeRate << " " << fromCur << " " << toCur << std::endl;
        std::cout << "Financial flow path added successfully." << std::endl;
    }

    // View all edges (financial flow paths)
    void viewEdges() {
        std::ifstream inFile(edgesFile);
        if (!inFile) {
            std::cerr << "Unable to read edge file." << std::endl;
            return;
        }
        std::string from, to, fromCur, toCur;
        double fee;
        std::cout << "Financial flow path list:" << std::endl;
        while (inFile >> from >> to >> fee >> fromCur >> toCur) {
            std::cout << "From " << from << " to " << to << "Fee rate: " << fee << "Currency: " << fromCur << " -> " << toCur << std::endl;
        }
    }

    // Load the graph from file
    void loadGraph() {
        graph.clear();
        std::ifstream inFile(edgesFile);
        std::string from, to, fromCur, toCur;
        double fee;
        while (inFile >> from >> to >> fee >> fromCur >> toCur) {
            graph[from].push_back({ to, fee, fromCur, toCur });
        }
    }

	// Find the best path between two institutions by Dijkstra's algorithm
    void findBestPath(const std::string& start, const std::string& end, double amount, RateManager& rateManager) {
        loadGraph();
        std::unordered_map<std::string, double> cost;
        std::unordered_map<std::string, std::string> prev;

        auto cmp = [&](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
            return a.second < b.second;
            };
        std::priority_queue<std::pair<std::string, double>, std::vector<std::pair<std::string, double>>, decltype(cmp)> pq(cmp);

        for (const auto& pair : graph) {
            cost[pair.first] = 0.0;
        }

        cost[start] = amount;
        pq.push({ start, amount });

        while (!pq.empty()) {
            auto [current, currentAmount] = pq.top(); pq.pop();
            if (current == end) break;

            for (const auto& edge : graph[current]) {
                double rate = rateManager.getRateOnline(edge.fromCurrency, edge.toCurrency);
                if (rate < 0) continue;
                double newAmount = currentAmount * rate * (1 - edge.feeRate);
                if (newAmount > cost[edge.to]) {
                    cost[edge.to] = newAmount;
                    prev[edge.to] = current;
                    pq.push({ edge.to, newAmount });
                }
            }
        }

        if (cost[end] == 0.0) {
            std::cout << "Unable to reach the target institution." << std::endl;
            return;
        }

        std::vector<std::string> path;
        for (std::string at = end; !at.empty(); at = prev[at]) {
            path.insert(path.begin(), at);
        }

        std::cout << "Optimal path: ";
        for (const auto& node : path) std::cout << node << " ";
        std::cout << "\nFinal amount: " << cost[end] << std::endl;
    }
};

// Helper function to get currency from edges file
std::string getCurrencyFromEdges(const std::string& institution, bool isSource) {
    std::ifstream inFile("edges.txt");
    std::string from, to, fromCur, toCur;
    double fee;

    while (inFile >> from >> to >> fee >> fromCur >> toCur) {
        if (isSource && from == institution) {
            return fromCur;
        }
        else if (!isSource && to == institution) {
            return toCur;
        }
    }
    return "Unknown";
}

// Main program entry point
int main() {
    UserManager userManager;

    GraphManager graphManager;
    RateManager rateManager;
    char choice;
    std::string username, password, role;

    while (true) {
        std::cout << "A. Register\nB. Login\nChoose: ";
        std::cin >> choice;
        choice = std::toupper(choice);

        if (choice == 'A') {
            std::cout << "Enter username: ";
            std::cin >> username;
            std::cout << "Enter password: ";
            std::cin >> password;
            std::cout << "Choose role (1. User  2. Admin): ";
            int roleChoice;
            std::cin >> roleChoice;
            role = (roleChoice == 2) ? "admin" : "user";
            userManager.registerUser(username, password, role);
        }
        else if (choice == 'B') {
            std::cout << "Enter username: ";
            std::cin >> username;
            std::cout << "Enter password: ";
            std::cin >> password;
            std::string userRole = userManager.loginUser(username, password);

            if (!userRole.empty()) {
                if (userRole == "admin") {
                    std::cout << "Welcome! Admins can access additional management functions.\n";
                    while (true) {
                        std::cout << "1. Add institution\n2. Add financial flow path\n3. Query optimal path\n4. View institutions\n5. View financial flow paths\n6. Exit admin functions\nEnter choice: ";
                        int adminChoice;
                        std::cin >> adminChoice;

                        if (adminChoice == 1) {
                            std::string instName, country;
                            std::cout << "Enter institution name: ";
                            std::cin >> instName;
                            std::cout << "Enter country: ";
                            std::cin >> country;
                            graphManager.addInstitution(instName, country);
                        }
                        else if (adminChoice == 2) {
                            std::string from, to, fromCurrency, toCurrency;
                            double fee;
                            std::cout << "Enter source institution: ";
                            std::cin >> from;
                            std::cout << "Enter destination institution: ";
                            std::cin >> to;
                            std::cout << "Enter fee rate (e.g., 0.001 for 0.1%): ";
                            std::cin >> fee;
                            std::cout << "Enter source currency: ";
                            std::cin >> fromCurrency;
                            std::cout << "Enter destination currency: ";
                            std::cin >> toCurrency;
                            graphManager.addEdge(from, to, fee, fromCurrency, toCurrency);
                        }
                        else if (adminChoice == 3) {
                            std::string start, end;
                            double amount;
                            std::cout << "Enter source institution: ";
                            std::cin >> start;
                            std::string startCurrency = getCurrencyFromEdges(start, true);
                            std::cout << "Source currency: " << startCurrency << std::endl;
                            std::cout << "Enter destination institution: ";
                            std::cin >> end;
                            std::string endCurrency = getCurrencyFromEdges(end, false);
                            std::cout << "Destination currency: " << endCurrency << std::endl;
                            std::cout << "Enter initial amount (in " << startCurrency << "):" << std::endl;
                            std::cin >> amount;
                            graphManager.findBestPath(start, end, amount, rateManager);
                        }
                        else if (adminChoice == 4) {
                            graphManager.viewInstitutions();
                        }
                        else if (adminChoice == 5) {
                            graphManager.viewEdges();
                        }
                        else if (adminChoice == 6) {
                            break;
                        }
                    }
                }
                else {
                    while (true) {
                        std::cout << "1. Query optimal path\n2. Exit\nEnter choice: ";
                        int userChoice;
                        std::cin >> userChoice;
                        if (userChoice == 1) {
                            std::string start, end;
                            double amount;
                            std::cout << "Enter source institution: ";
                            std::cin >> start;
                            std::string startCurrency = getCurrencyFromEdges(start, true);
                            std::cout << "Source currency: " << startCurrency << std::endl;
                            std::cout << "Enter destination institution: ";
                            std::cin >> end;
                            std::string endCurrency = getCurrencyFromEdges(end, false);
                            std::cout << "Destination currency: " << endCurrency << std::endl;
                            std::cout << "Enter initial amount (in " << startCurrency << "):" << std::endl;
                            std::cin >> amount;
                            graphManager.findBestPath(start, end, amount, rateManager);
                        }
                        else if (userChoice == 2) {
                            break;
                        }
                    }
                }
                break;
            }
        }
        else {
            std::cout << "Invalid option. Please enter A or B." << std::endl;
        }
    }

    return 0;
}