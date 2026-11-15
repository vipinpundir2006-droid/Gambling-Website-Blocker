#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <cctype>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

// ==========================================
// 1. EVIDENCE STRUCTURE
// ==========================================

struct Evidence {
    int oddsPatterns = 0;
    int betActions = 0;
    int stakeInputs = 0;
    int marketGroups = 0;
    int depositActions = 0;
    int casinoEmbeds = 0;
    int paymentForms = 0;
    int gamblingTerms = 0;
    int bettingControls = 0;
    int clickableOdds = 0;
    int walletSignals = 0;
    int casinoTerms = 0;

    // Game-specific signals
    int aviatorSignals = 0;
    int vortexSignals = 0;
    int minesSignals = 0;
    int chickenRoadSignals = 0;

    // Game mechanics
    int multiplierSignals = 0;
    int cashoutSignals = 0;
    int mineGridSignals = 0;
    int safeTileSignals = 0;
    int roundSignals = 0;
    int payoutSignals = 0;
    int gameControls = 0;

    string url;
};

struct AnalysisResult {
    int score = 0;
    string status;
    vector<string> reasons;
};

struct ScanRecord {
    string url;
    int score;
    string status;
};

// ==========================================
// 2. URL DECODING
// ==========================================

string urlDecode(const string &s) {
    string out;

    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') {
            out += ' ';
        }
        else if (s[i] == '%' && i + 2 < s.size()) {
            string hex = s.substr(i + 1, 2);
            char ch = (char)strtol(hex.c_str(), NULL, 16);
            out += ch;
            i += 2;
        }
        else {
            out += s[i];
        }
    }

    return out;
}

map<string, string> parseForm(const string &body) {
    map<string, string> values;
    stringstream ss(body);
    string part;

    while (getline(ss, part, '&')) {
        size_t eq = part.find('=');

        if (eq == string::npos) continue;

        values[urlDecode(part.substr(0, eq))] =
            urlDecode(part.substr(eq + 1));
    }

    return values;
}

int getNumber(const map<string, string> &v,
              const string &key) {

    map<string, string>::const_iterator it = v.find(key);

    if (it == v.end()) return 0;

    char *end = NULL;
    long n = strtol(it->second.c_str(), &end, 10);

    if (end == it->second.c_str()) return 0;
    if (n < 0) return 0;
    if (n > 1000) return 1000;

    return (int)n;
}

// ==========================================
// 3. DOMAIN EXTRACTION
// ==========================================

string getDomain(string url) {

    transform(url.begin(), url.end(), url.begin(),
        [](unsigned char c) {
            return (char)tolower(c);
        });

    size_t scheme = url.find("://");

    if (scheme != string::npos) {
        url = url.substr(scheme + 3);
    }

    size_t end = url.find_first_of("/?#");

    if (end != string::npos) {
        url = url.substr(0, end);
    }

    size_t at = url.rfind('@');

    if (at != string::npos) {
        url = url.substr(at + 1);
    }

    size_t port = url.find(':');

    if (port != string::npos) {
        url = url.substr(0, port);
    }

    if (url.size() > 4 && url.substr(0, 4) == "www.") {
        url = url.substr(4);
    }

    while (!url.empty() && url.back() == '.') {
        url.pop_back();
    }

    return url;
}

// ==========================================
// 4. TRIE DATA STRUCTURE
// ==========================================

class Trie {

private:

    struct Node {
        Node* child[128];
        bool isEnd;

        Node() {
            isEnd = false;

            for (int i = 0; i < 128; i++) {
                child[i] = NULL;
            }
        }
    };

    Node* root;

    void deleteTree(Node* node) {
        if (node == NULL) return;

        for (int i = 0; i < 128; i++) {
            deleteTree(node->child[i]);
        }

        delete node;
    }

public:

    Trie() {
        root = new Node();
    }

    ~Trie() {
        deleteTree(root);
    }

    void insert(const string &word) {
        Node* current = root;

        for (char c : word) {
            unsigned char index = (unsigned char)c;

            if (index >= 128) return;

            if (current->child[index] == NULL) {
                current->child[index] = new Node();
            }

            current = current->child[index];
        }

        current->isEnd = true;
    }

    bool search(const string &word) const {
        Node* current = root;

        for (char c : word) {
            unsigned char index = (unsigned char)c;

            if (index >= 128) return false;

            if (current->child[index] == NULL) {
                return false;
            }

            current = current->child[index];
        }

        return current->isEnd;
    }
};

// ==========================================
// 5. HASH TABLE
// ==========================================

class DomainHashTable {

private:

    struct HashNode {
        string domain;
        HashNode* next;

        HashNode(string d) {
            domain = d;
            next = NULL;
        }
    };

    static const int SIZE = 101;

    HashNode* table[SIZE];

    int hashFunction(const string &domain) const {
        unsigned long hash = 0;

        for (char c : domain) {
            hash = hash * 31 + (unsigned char)c;
        }

        return (int)(hash % SIZE);
    }

public:

    DomainHashTable() {
        for (int i = 0; i < SIZE; i++) {
            table[i] = NULL;
        }
    }

    ~DomainHashTable() {
        for (int i = 0; i < SIZE; i++) {
            HashNode* current = table[i];

            while (current != NULL) {
                HashNode* temp = current;
                current = current->next;
                delete temp;
            }
        }
    }

    void insert(const string &domain) {
        int index = hashFunction(domain);

        HashNode* current = table[index];

        while (current != NULL) {
            if (current->domain == domain) return;
            current = current->next;
        }

        HashNode* newNode = new HashNode(domain);
        newNode->next = table[index];
        table[index] = newNode;
    }

    bool search(const string &domain) const {
        int index = hashFunction(domain);

        HashNode* current = table[index];

        while (current != NULL) {
            if (current->domain == domain) return true;
            current = current->next;
        }

        return false;
    }

    bool containsDomainOrParent(string domain) const {

        while (!domain.empty()) {

            if (search(domain)) return true;

            size_t dot = domain.find('.');

            if (dot == string::npos) break;

            domain = domain.substr(dot + 1);
        }

        return false;
    }
};

// ==========================================
// 6. STACK
// ==========================================

template <typename T>
class Stack {

private:

    vector<T> items;
    int capacity;

public:

    Stack(int maxSize = 100) {
        capacity = maxSize;
    }

    void push(const T &value) {

        if ((int)items.size() == capacity) {
            items.erase(items.begin());
        }

        items.push_back(value);
    }

    void pop() {
        if (!items.empty()) items.pop_back();
    }

    T top() const {
        if (!items.empty()) return items.back();
        return T();
    }

    bool isEmpty() const {
        return items.empty();
    }

    int size() const {
        return (int)items.size();
    }
};

// ==========================================
// 7. QUEUE
// ==========================================

template <typename T>
class Queue {

private:

    struct Node {
        T data;
        Node* next;

        Node(const T &value) {
            data = value;
            next = NULL;
        }
    };

    Node* frontNode;
    Node* rearNode;
    int count;

public:

    Queue() {
        frontNode = NULL;
        rearNode = NULL;
        count = 0;
    }

    ~Queue() {
        while (!isEmpty()) {
            T temp;
            dequeue(temp);
        }
    }

    void enqueue(const T &value) {

        Node* newNode = new Node(value);

        if (rearNode == NULL) {
            frontNode = rearNode = newNode;
        }
        else {
            rearNode->next = newNode;
            rearNode = newNode;
        }

        count++;
    }

    bool dequeue(T &value) {

        if (frontNode == NULL) return false;

        Node* temp = frontNode;

        value = temp->data;

        frontNode = frontNode->next;

        if (frontNode == NULL) {
            rearNode = NULL;
        }

        delete temp;
        count--;

        return true;
    }

    bool isEmpty() const {
        return frontNode == NULL;
    }

    int size() const {
        return count;
    }
};

// ==========================================
// 8. CONTENT ANALYSIS
// ==========================================

AnalysisResult evaluateContent(const Evidence &e) {

    AnalysisResult r;

    // Decimal odds
    if (e.oddsPatterns >= 3 &&
        (e.betActions > 0 || e.bettingControls > 0)) {

        r.score += 20;
        r.reasons.push_back(
            "Multiple decimal odds with betting context found");
    }
    else if (e.oddsPatterns > 0 &&
             (e.betActions > 0 || e.bettingControls > 0)) {

        r.score += 8;
        r.reasons.push_back(
            "Decimal odds with betting context found");
    }

    // Betting actions
    if (e.betActions > 0) {
        r.score += 20;
        r.reasons.push_back("Betting actions found");
    }

    // Stake inputs
    if (e.stakeInputs > 0) {
        r.score += 15;
        r.reasons.push_back("Stake input found");
    }

    // Market groups
    if (e.marketGroups >= 3 &&
        (e.betActions > 0 || e.bettingControls > 0)) {

        r.score += 15;
        r.reasons.push_back(
            "Multiple betting market groups found");
    }

    // Deposit and withdrawal
    if (e.depositActions > 0) {
        r.score += 8;
        r.reasons.push_back(
            "Deposit or withdrawal actions found");
    }

    // Casino embeds
    if (e.casinoEmbeds > 0) {
        r.score += 20;
        r.reasons.push_back("Casino game embed found");
    }

    // Payment forms
    if (e.paymentForms > 0 &&
        (e.depositActions > 0 || e.casinoTerms >= 3)) {

        r.score += 5;
        r.reasons.push_back(
            "Payment form with gambling indicators found");
    }

    // Gambling terminology
    if (e.gamblingTerms >= 10) {
        r.score += 15;
        r.reasons.push_back(
            "High concentration of gambling terms");
    }
    else if (e.gamblingTerms >= 5) {
        r.score += 10;
        r.reasons.push_back("Multiple gambling terms found");
    }
    else if (e.gamblingTerms > 0) {
        r.score += 4;
        r.reasons.push_back("Some gambling terms found");
    }

    // Betting controls
    if (e.bettingControls > 0) {
        r.score += 20;
        r.reasons.push_back(
            "Interactive betting controls found");
    }

    // Clickable odds
    if (e.clickableOdds >= 2 &&
        (e.betActions > 0 || e.bettingControls > 0)) {

        r.score += 15;
        r.reasons.push_back(
            "Multiple clickable odds with betting context found");
    }

    // Wallet signals
    if (e.walletSignals >= 3 && e.depositActions > 0) {
        r.score += 10;
        r.reasons.push_back(
            "Wallet signals combined with deposit activity");
    }

    // Casino terminology
    if (e.casinoTerms >= 8) {
        r.score += 20;
        r.reasons.push_back(
            "Strong casino and lottery indicators found");
    }
    else if (e.casinoTerms >= 3) {
        r.score += 12;
        r.reasons.push_back(
            "Multiple casino-related terms found");
    }
    else if (e.casinoTerms > 0) {
        r.score += 5;
        r.reasons.push_back("Casino-related terms found");
    }

    // Strong combinations
    if (e.oddsPatterns > 0 && e.betActions > 0) {
        r.score += 10;
        r.reasons.push_back(
            "Odds and betting actions occur together");
    }

    if (e.stakeInputs > 0 && e.betActions > 0) {
        r.score += 10;
        r.reasons.push_back(
            "Stake and betting actions occur together");
    }

    if (e.clickableOdds > 0 && e.bettingControls > 0) {
        r.score += 10;
        r.reasons.push_back(
            "Clickable odds and betting controls occur together");
    }

    if (e.gamblingTerms >= 10 && e.casinoTerms >= 8) {
        r.score += 15;
        r.reasons.push_back(
            "Strong combination of gambling and casino indicators");
    }

    if (e.casinoTerms >= 8 && e.depositActions > 0) {
        r.score += 10;
        r.reasons.push_back(
            "Casino indicators combined with deposit activity");
    }

    // Game-specific detection
    int gameCount = 0;

    if (e.aviatorSignals > 0) gameCount++;
    if (e.vortexSignals > 0) gameCount++;
    if (e.minesSignals > 0) gameCount++;
    if (e.chickenRoadSignals > 0) gameCount++;

    if (gameCount >= 1) {
        r.score += 15;
        r.reasons.push_back(
            "Recognized gambling-style game signals found");
    }

    if (gameCount >= 2) {
        r.score += 15;
        r.reasons.push_back(
            "Multiple gambling game patterns detected");
    }

    // Multiplier and cashout
    if (e.multiplierSignals > 0 &&
        e.cashoutSignals > 0) {

        r.score += 20;
        r.reasons.push_back(
            "Multiplier and cashout mechanics detected");
    }

    // Mines grid and safe tiles
    if (e.mineGridSignals > 0 &&
        e.safeTileSignals > 0) {

        r.score += 20;
        r.reasons.push_back(
            "Mine grid and safe tile mechanics detected");
    }

    // Game controls with monetary activity
    if (e.gameControls > 0 &&
        (e.payoutSignals > 0 || e.stakeInputs > 0)) {

        r.score += 15;
        r.reasons.push_back(
            "Game controls with monetary activity detected");
    }

    // Multiple game mechanics
    int mechanics = 0;

    if (e.multiplierSignals > 0) mechanics++;
    if (e.cashoutSignals > 0) mechanics++;
    if (e.mineGridSignals > 0) mechanics++;
    if (e.safeTileSignals > 0) mechanics++;
    if (e.roundSignals > 0) mechanics++;
    if (e.payoutSignals > 0) mechanics++;

    if (mechanics >= 3) {
        r.score += 15;
        r.reasons.push_back(
            "Multiple gambling game mechanics detected");
    }

    return r;
}

// ==========================================
// 9. OOPS RISK RULE
// ==========================================

class RiskRule {

public:

    virtual void apply(
        const Evidence &e,
        AnalysisResult &result,
        const Trie &trie,
        const DomainHashTable &domains
    ) = 0;

    virtual ~RiskRule() {}
};

// ==========================================
// 10. CONTENT RISK RULE
// ==========================================

class ContentRiskRule : public RiskRule {

public:

    void apply(
        const Evidence &e,
        AnalysisResult &result,
        const Trie &trie,
        const DomainHashTable &domains
    ) override {

        AnalysisResult content = evaluateContent(e);

        result.score += content.score;

        for (const string &reason : content.reasons) {
            result.reasons.push_back(reason);
        }
    }
};

// ==========================================
// 11. DOMAIN RISK RULE
// ==========================================

class DomainRiskRule : public RiskRule {

public:

    void apply(
        const Evidence &e,
        AnalysisResult &result,
        const Trie &trie,
        const DomainHashTable &domains
    ) override {

        string domain = getDomain(e.url);

        if (domain.empty()) return;

        if (domains.containsDomainOrParent(domain)) {

            result.score += 65;

            result.reasons.push_back(
                "Domain found in known domain database");
        }

        stringstream ss(domain);
        string label;

        while (getline(ss, label, '.')) {

            if (trie.search(label)) {

                result.score += 15;

                result.reasons.push_back(
                    "Gambling-related domain keyword found: " + label);
            }
        }
    }
};

// ==========================================
// 12. RISK ENGINE
// ==========================================

class RiskEngine {

private:

    Trie keywordTrie;
    DomainHashTable blockedDomains;

    vector<unique_ptr<RiskRule>> rules;

    // Check exact domain and its subdomains
    bool isKnownGamblingDomain(string url) const {

        string domain = getDomain(url);

        const string knownDomains[] = {
            "jalwaclub3.com",
            "tgdream.pro",
            "2okwin6.com",
            "ok-win.ai",
            "tiranagam.ai",
            "jalwav.com"
        };

        for (int i = 0; i < 6; i++) {

            string known = knownDomains[i];

            // Exact match
            if (domain == known) {
                return true;
            }

            // Subdomain match
            if (domain.size() > known.size() &&
                domain.compare(
                    domain.size() - known.size(),
                    known.size(),
                    known
                ) == 0 &&
                domain[domain.size() - known.size() - 1] == '.') {

                return true;
            }
        }

        return false;
    }

public:

    RiskEngine() {

        // Trie keywords
        keywordTrie.insert("casino");
        keywordTrie.insert("bet");
        keywordTrie.insert("betting");
        keywordTrie.insert("poker");
        keywordTrie.insert("gambling");
        keywordTrie.insert("sportsbook");
        keywordTrie.insert("roulette");
        keywordTrie.insert("slots");
        keywordTrie.insert("aviator");
        keywordTrie.insert("mines");
        keywordTrie.insert("vortex");
        keywordTrie.insert("chickenroad");
        keywordTrie.insert("multiplier");
        keywordTrie.insert("cashout");
        keywordTrie.insert("payout");
        keywordTrie.insert("jackpot");
        keywordTrie.insert("wager");
        keywordTrie.insert("stake");
        keywordTrie.insert("crash");

        // Six known domains
        blockedDomains.insert("jalwaclub3.com");
        blockedDomains.insert("tgdream.pro");
        blockedDomains.insert("2okwin6.com");
        blockedDomains.insert("ok-win.ai");
        blockedDomains.insert("tiranagam.ai");
        blockedDomains.insert("jalwav.com");

        // OOPS polymorphism
        rules.push_back(
            unique_ptr<RiskRule>(new ContentRiskRule()));

        rules.push_back(
            unique_ptr<RiskRule>(new DomainRiskRule()));
    }

    AnalysisResult analyze(const Evidence &e) {

        AnalysisResult result;

        // PRIORITY 1: Known domain detection
        if (isKnownGamblingDomain(e.url)) {

            result.score = 100;

            result.status = "HIGH RISK - LIKELY GAMBLING";

            result.reasons.push_back(
                "Known gambling domain matched");

            result.reasons.push_back(
                "Exact domain or subdomain blacklist detection");

            return result;
        }

        // PRIORITY 2: Content and domain analysis
        for (size_t i = 0; i < rules.size(); i++) {

            rules[i]->apply(
                e, result, keywordTrie, blockedDomains);
        }

        if (result.score > 100) {
            result.score = 100;
        }

        // Classification
        if (result.score >= 60) {
            result.status = "HIGH RISK - LIKELY GAMBLING";
        }
        else if (result.score >= 30) {
            result.status = "SUSPICIOUS - NEEDS REVIEW";
        }
        else {
            result.status = "LOW EVIDENCE - NOT CONFIRMED";
        }

        return result;
    }
};

// ==========================================
// 13. JSON RESPONSE
// ==========================================

string jsonEscape(const string &s) {

    string out;

    for (size_t i = 0; i < s.size(); i++) {

        if (s[i] == '"' || s[i] == '\\') {
            out += '\\';
        }

        if (s[i] == '\n') {
            out += "\\n";
            continue;
        }

        if (s[i] == '\r') continue;

        out += s[i];
    }

    return out;
}

string makeJson(const AnalysisResult &r) {

    ostringstream out;

    out << "{\"score\":" << r.score
        << ",\"status\":\"" << jsonEscape(r.status)
        << "\",\"reasons\":[";

    for (size_t i = 0; i < r.reasons.size(); i++) {

        if (i) out << ",";

        out << "\"" << jsonEscape(r.reasons[i]) << "\"";
    }

    out << "],\"note\":\"Heuristic result; not a guarantee of safety.\"}";

    return out.str();
}

// ==========================================
// 14. HTTP RESPONSE
// ==========================================

void sendResponse(
    SOCKET client,
    int code,
    const string &status,
    const string &body
) {

    ostringstream response;

    response << "HTTP/1.1 " << code << " " << status << "\r\n";
    response << "Content-Type: application/json; charset=utf-8\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "Content-Length: " << body.size() << "\r\n\r\n";

    response << body;

    string data = response.str();

    size_t sent = 0;

    while (sent < data.size()) {

        int result = send(
            client,
            data.c_str() + sent,
            (int)(data.size() - sent),
            0
        );

        if (result <= 0) break;

        sent += result;
    }
}

// ==========================================
// 15. MAIN SERVER
// ==========================================

int main() {

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {

        cout << "Winsock startup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (serverSocket == INVALID_SOCKET) {

        cout << "Could not create server socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress = {};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8765);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(
        serverSocket,
        (sockaddr*)&serverAddress,
        sizeof(serverAddress)
    ) == SOCKET_ERROR) {

        cout << "Port 8765 is busy or could not be opened.\n";

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {

        cout << "Server listen failed.\n";

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    // Risk engine
    RiskEngine engine;

    // DSA: Stack
    Stack<ScanRecord> scanHistory(100);

    // DSA: Queue
    Queue<Evidence> scanQueue;

    cout << "\n====================================\n";
    cout << " GAMBLING WEBSITE BLOCKER BACKEND\n";
    cout << "====================================\n";

    cout << "Listening: 127.0.0.1:8765\n";
    cout << "Trie: Ready\n";
    cout << "Hash Table: Ready\n";
    cout << "Stack: Ready\n";
    cout << "Queue: Ready\n";
    cout << "OOPS Risk Engine: Ready\n";

    cout << "Known domains: 6\n";
    cout << "Game detection: Ready\n";
    cout << "Keep this window open.\n\n";

    while (true) {

        SOCKET client = accept(serverSocket, NULL, NULL);

        if (client == INVALID_SOCKET) {
            continue;
        }

        string request;
        char buffer[4096];
        int received;

        // Read headers
        do {

            received = recv(
                client,
                buffer,
                sizeof(buffer),
                0
            );

            if (received > 0) {
                request.append(buffer, received);
            }

        } while (
            received > 0 &&
            request.find("\r\n\r\n") == string::npos &&
            request.size() < 65536
        );

        size_t headerEnd = request.find("\r\n\r\n");

        if (headerEnd == string::npos) {

            sendResponse(
                client,
                400,
                "Bad Request",
                "{\"error\":\"Bad request\"}"
            );

            closesocket(client);
            continue;
        }

        string headers = request.substr(0, headerEnd);
        string body = request.substr(headerEnd + 4);

        // OPTIONS request
        if (headers.find("OPTIONS ") == 0) {

            sendResponse(
                client,
                204,
                "No Content",
                ""
            );

            closesocket(client);
            continue;
        }

        // Only POST /analyze
        if (headers.find("POST /analyze ") != 0) {

            sendResponse(
                client,
                404,
                "Not Found",
                "{\"error\":\"Use POST /analyze\"}"
            );

            closesocket(client);
            continue;
        }

        // Content-Length
        size_t contentPos = headers.find("Content-Length:");

        int contentLength = 0;

        if (contentPos != string::npos) {

            contentLength = atoi(
                headers.c_str() + contentPos + 15
            );
        }

        if (contentLength < 0 || contentLength > 32768) {

            sendResponse(
                client,
                413,
                "Payload Too Large",
                "{\"error\":\"Invalid or oversized body\"}"
            );

            closesocket(client);
            continue;
        }

        // Read remaining body
        while ((int)body.size() < contentLength) {

            received = recv(
                client,
                buffer,
                sizeof(buffer),
                0
            );

            if (received <= 0) break;

            body.append(buffer, received);
        }

        if ((int)body.size() < contentLength) {

            sendResponse(
                client,
                400,
                "Bad Request",
                "{\"error\":\"Incomplete request body\"}"
            );

            closesocket(client);
            continue;
        }

        if ((int)body.size() > 32768) {

            sendResponse(
                client,
                413,
                "Payload Too Large",
                "{\"error\":\"Payload too large\"}"
            );

            closesocket(client);
            continue;
        }

        // Parse form data
        map<string, string> fields = parseForm(body);

        Evidence e;

        e.oddsPatterns = getNumber(fields, "odds");
        e.betActions = getNumber(fields, "betActions");
        e.stakeInputs = getNumber(fields, "stakeInputs");
        e.marketGroups = getNumber(fields, "marketGroups");
        e.depositActions = getNumber(fields, "depositActions");
        e.casinoEmbeds = getNumber(fields, "casinoEmbeds");
        e.paymentForms = getNumber(fields, "paymentForms");

        e.gamblingTerms = getNumber(fields, "gamblingTerms");
        e.bettingControls = getNumber(fields, "bettingControls");
        e.clickableOdds = getNumber(fields, "clickableOdds");
        e.walletSignals = getNumber(fields, "walletSignals");
        e.casinoTerms = getNumber(fields, "casinoTerms");

        // Game-specific fields
        e.aviatorSignals = getNumber(fields, "aviatorSignals");
        e.vortexSignals = getNumber(fields, "vortexSignals");
        e.minesSignals = getNumber(fields, "minesSignals");
        e.chickenRoadSignals = getNumber(fields, "chickenRoadSignals");

        // Game mechanics
        e.multiplierSignals = getNumber(fields, "multiplierSignals");
        e.cashoutSignals = getNumber(fields, "cashoutSignals");
        e.mineGridSignals = getNumber(fields, "mineGridSignals");
        e.safeTileSignals = getNumber(fields, "safeTileSignals");
        e.roundSignals = getNumber(fields, "roundSignals");
        e.payoutSignals = getNumber(fields, "payoutSignals");
        e.gameControls = getNumber(fields, "gameControls");

        // Receive URL
        if (fields.find("url") != fields.end()) {
            e.url = fields["url"];
        }
        else if (fields.find("pageUrl") != fields.end()) {
            e.url = fields["pageUrl"];
        }
        else if (fields.find("page_url") != fields.end()) {
            e.url = fields["page_url"];
        }

        // Queue processing
        scanQueue.enqueue(e);

        Evidence currentEvidence;

        if (!scanQueue.dequeue(currentEvidence)) {
            closesocket(client);
            continue;
        }

        // Analyze
        AnalysisResult result = engine.analyze(currentEvidence);

        // Stack history
        ScanRecord record;

        record.url = currentEvidence.url;
        record.score = result.score;
        record.status = result.status;

        scanHistory.push(record);

        // Console output
        cout << "\nScanned page: "
             << currentEvidence.url << "\n";

        cout << "Domain: "
             << getDomain(currentEvidence.url) << "\n";

        cout << "Score: "
             << result.score << "\n";

        cout << "Status: "
             << result.status << "\n";

        cout << "History: "
             << scanHistory.size() << "\n";

        cout << "-----------------------------\n";

        // Send response
        sendResponse(
            client,
            200,
            "OK",
            makeJson(result)
        );

        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}