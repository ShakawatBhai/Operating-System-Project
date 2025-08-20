#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <list>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <map>
#include <cmath>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace chrono;

void clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printFrames(const string& algo, int page, const vector<int>& frames, bool hit) {
    cout << "\n[" << algo << "] Page Reference: " << page << "\n";
    cout << (hit ? "  Page HIT " : "  Page FAULT ") << "\n";
    cout << "Current Frames: ";
    for (int f : frames) cout << (f == -1 ? '-' : f) << " ";
    cout << "\n" << string(40, '-') << "\n";
}

void printSummary(const string& name, int hits, int faults, int total, double timeMs, ostream& out, ofstream& csv) {
    double hitRate = (double)hits / total * 100.0;
    double missRate = (double)faults / total * 100.0;

    out << "\n========== " << name << " Summary ==========\n";
    out << "Total References : " << total << "\n";
    out << "Page Hits       : " << hits << "\n";
    out << "Page Faults     : " << faults << "\n";
    out << fixed << setprecision(2);
    out << "Hit Rate        : " << hitRate << "%\n";
    out << "Fault Rate      : " << missRate << "%\n";
    out << "Execution Time  : " << timeMs << " ms\n";
    out << string(40, '=') << "\n\n";

    csv << name << "," << hits << "," << faults << "," << fixed << setprecision(2) << hitRate << "," << missRate << "," << timeMs << "\n";
}

// FIFO
pair<int, int> fifo(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    unordered_set<int> s;
    queue<int> q;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        bool hit = s.find(page) != s.end();
        if (hit) hits++;
        else {
            faults++;
            if (s.size() == capacity) {
                int rem = q.front(); q.pop();
                s.erase(rem);
                auto it = find(frames.begin(), frames.end(), rem);
                if (it != frames.end()) *it = page;
            } else {
                auto it = find(frames.begin(), frames.end(), -1);
                if (it != frames.end()) *it = page;
            }
            s.insert(page);
            q.push(page);
        }
        if (trace) printFrames("FIFO", page, frames, hit);
    }
    printSummary("FIFO", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// LRU
pair<int, int> lru(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    list<int> lruList;
    unordered_map<int, list<int>::iterator> pageMap;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        bool hit = pageMap.find(page) != pageMap.end();
        if (hit) {
            hits++;
            lruList.erase(pageMap[page]);
        } else {
            faults++;
            if ((int)lruList.size() == capacity) {
                int lruPage = lruList.back();
                lruList.pop_back();
                pageMap.erase(lruPage);
                auto it = find(frames.begin(), frames.end(), lruPage);
                if (it != frames.end()) *it = page;
            } else {
                auto it = find(frames.begin(), frames.end(), -1);
                if (it != frames.end()) *it = page;
            }
        }
        lruList.push_front(page);
        pageMap[page] = lruList.begin();

        int i = 0;
        for (int f : lruList) {
            if (i < capacity) frames[i++] = f;
        }
        while (i < capacity) frames[i++] = -1;

        if (trace) printFrames("LRU", page, frames, hit);
    }
    printSummary("LRU", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// Optimal
pair<int, int> optimal(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    vector<int> frames;
    vector<int> display(capacity, -1);
    int hits = 0, faults = 0;

    for (int i = 0; i < pages.size(); ++i) {
        int page = pages[i];
        auto it = find(frames.begin(), frames.end(), page);
        bool hit = (it != frames.end());
        if (hit) {
            hits++;
        } else {
            faults++;
            if (frames.size() < capacity) {
                frames.push_back(page);
                auto pit = find(display.begin(), display.end(), -1);
                if (pit != display.end()) *pit = page;
            } else {
                int farthest = i + 1, index = -1;
                for (int j = 0; j < frames.size(); j++) {
                    int k;
                    for (k = i + 1; k < pages.size(); k++) {
                        if (pages[k] == frames[j]) break;
                    }
                    if (k > farthest) {
                        farthest = k;
                        index = j;
                    }
                }
                if (index == -1) index = 0;
                int victim = frames[index];
                frames[index] = page;
                auto it2 = find(display.begin(), display.end(), victim);
                if (it2 != display.end()) *it2 = page;
            }
        }
        if (trace) printFrames("Optimal", page, display, hit);
    }
    printSummary("Optimal", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// Second Chance
pair<int, int> secondChance(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    queue<pair<int, bool>> mem;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        bool found = false;
        queue<pair<int, bool>> temp;

        while (!mem.empty()) {
            pair<int, bool> front = mem.front(); mem.pop();
            int val = front.first;
            bool ref = front.second;

            if (val == page) {
                found = true;
                temp.push(make_pair(val, true));
            } else {
                temp.push(make_pair(val, ref));
            }
        }
        mem = temp;

        if (found) hits++;
        else {
            faults++;
            if (mem.size() == capacity) {
                while (true) {
                    pair<int, bool> front = mem.front(); mem.pop();
                    int val = front.first;
                    bool ref = front.second;
                    if (!ref) break;
                    mem.push(make_pair(val, false));
                }
            }
            mem.push(make_pair(page, true));
        }

        vector<int> current;
        queue<pair<int, bool>> copy = mem;
        while (!copy.empty()) {
            current.push_back(copy.front().first);
            copy.pop();
        }
        while (current.size() < capacity) current.push_back(-1);

        if (trace) printFrames("Second Chance", page, current, found);
    }

    printSummary("Second Chance", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// LFU
pair<int, int> lfu(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    unordered_map<int, int> freq;
    list<int> mem;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        bool found = find(mem.begin(), mem.end(), page) != mem.end();
        if (found) {
            hits++; freq[page]++;
        } else {
            faults++;
            if (mem.size() == capacity) {
                int minFreq = INT_MAX, victim = -1;
                for (int p : mem) {
                    if (freq[p] < minFreq) {
                        minFreq = freq[p];
                        victim = p;
                    }
                }
                mem.remove(victim);
                freq.erase(victim);
                auto it = find(frames.begin(), frames.end(), victim);
                if (it != frames.end()) *it = page;
            } else {
                auto it = find(frames.begin(), frames.end(), -1);
                if (it != frames.end()) *it = page;
            }
            mem.push_back(page);
            freq[page] = 1;
        }

        int i = 0;
        for (int p : mem) {
            if (i < capacity) frames[i++] = p;
        }
        while (i < capacity) frames[i++] = -1;

        if (trace) printFrames("LFU", page, frames, found);
    }
    printSummary("LFU", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// MFU
pair<int, int> mfu(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    unordered_map<int, int> freq;
    list<int> mem;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        bool found = find(mem.begin(), mem.end(), page) != mem.end();
        if (found) {
            hits++; freq[page]++;
        } else {
            faults++;
            if (mem.size() == capacity) {
                int maxFreq = -1, victim = -1;
                for (int p : mem) {
                    if (freq[p] > maxFreq) {
                        maxFreq = freq[p];
                        victim = p;
                    }
                }
                mem.remove(victim);
                freq.erase(victim);
                auto it = find(frames.begin(), frames.end(), victim);
                if (it != frames.end()) *it = page;
            } else {
                auto it = find(frames.begin(), frames.end(), -1);
                if (it != frames.end()) *it = page;
            }
            mem.push_back(page);
            freq[page] = 1;
        }

        int i = 0;
        for (int p : mem) {
            if (i < capacity) frames[i++] = p;
        }
        while (i < capacity) frames[i++] = -1;

        if (trace) printFrames("MFU", page, frames, found);
    }
    printSummary("MFU", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// Aging
pair<int, int> aging(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace) {
    unordered_map<int, unsigned char> counters;
    vector<int> mem;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;

    for (int page : pages) {
        for (auto& p : mem) counters[p] >>= 1;

        bool found = find(mem.begin(), mem.end(), page) != mem.end();
        if (found) {
            hits++; counters[page] |= 0x80;
        } else {
            faults++;
            if ((int)mem.size() == capacity) {
                int victim = mem[0];
                for (int p : mem) {
                    if (counters[p] < counters[victim]) victim = p;
                }
                mem.erase(find(mem.begin(), mem.end(), victim));
                counters.erase(victim);
            }
            mem.push_back(page);
            counters[page] = 0x80;
        }

        int i = 0;
        for (int p : mem) {
            if (i < capacity) frames[i++] = p;
        }
        while (i < capacity) frames[i++] = -1;

        if (trace) printFrames("Aging", page, frames, found);
    }

    printSummary("Aging", hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// Custom Algorithm
pair<int, int> custom(const vector<int>& pages, int capacity, ostream& out, ofstream& csv, bool trace, int ruleChoice, int windowSize) {
    unordered_map<int, int> freq;
    vector<int> mem;
    vector<int> frames(capacity, -1);
    int hits = 0, faults = 0;
    vector<int> recentPages;

    for (int i = 0; i < pages.size(); ++i) {
        int page = pages[i];
        bool found = find(mem.begin(), mem.end(), page) != mem.end();
        recentPages.push_back(page);
        if (recentPages.size() > windowSize) recentPages.erase(recentPages.begin());

        if (found) {
            hits++; 
            freq[page]++;
        } else {
            faults++;
            if (mem.size() == capacity) {
                int victim = -1;
                if (ruleChoice == 1) { // Evict page with fewest accesses in last windowSize steps
                    unordered_map<int, int> windowFreq;
                    for (int p : recentPages) windowFreq[p]++;
                    int minFreq = INT_MAX;
                    for (int p : mem) {
                        int count = windowFreq.count(p) ? windowFreq[p] : 0;
                        if (count < minFreq) {
                            minFreq = count;
                            victim = p;
                        }
                    }
                } else if (ruleChoice == 2) { // Prioritize even-numbered pages
                    bool hasOdd = false;
                    for (int p : mem) {
                        if (p % 2 != 0) {
                            victim = p;
                            hasOdd = true;
                            break;
                        }
                    }
                    if (!hasOdd) victim = mem[0]; // Fallback to first page if all are even
                }
                mem.erase(find(mem.begin(), mem.end(), victim));
                freq.erase(victim);
                auto it = find(frames.begin(), frames.end(), victim);
                if (it != frames.end()) *it = page;
            } else {
                auto it = find(frames.begin(), frames.end(), -1);
                if (it != frames.end()) *it = page;
            }
            mem.push_back(page);
            freq[page]++;
        }

        int j = 0;
        for (int p : mem) {
            if (j < capacity) frames[j++] = p;
        }
        while (j < capacity) frames[j++] = -1;

        if (trace) printFrames("Custom", page, frames, found);
    }

    string ruleName = (ruleChoice == 1) ? "Custom (Fewest in Window)" : "Custom (Even Priority)";
    printSummary(ruleName, hits, faults, pages.size(), 0, out, csv);
    return {hits, faults};
}

// Memory Access Patterns Detector
string detectPattern(const vector<int>& pages) {
    unordered_map<int, int> freq;
    int sequentialCount = 0;
    int uniquePages = 0;
    double avgGap = 0.0;
    int gaps = 0;

    // Frequency and sequential analysis
    for (int i = 0; i < pages.size(); ++i) {
        freq[pages[i]]++;
        if (i > 0 && (pages[i] == pages[i-1] + 1 || pages[i] == pages[i-1] - 1)) {
            sequentialCount++;
        }
    }
    uniquePages = freq.size();

    // Calculate average gap between re-references
    unordered_map<int, int> lastSeen;
    for (int i = 0; i < pages.size(); ++i) {
        if (lastSeen.count(pages[i])) {
            avgGap += (i - lastSeen[pages[i]]);
            gaps++;
        }
        lastSeen[pages[i]] = i;
    }
    avgGap = gaps > 0 ? avgGap / gaps : pages.size();

    // Pattern detection logic
    double uniqueRatio = (double)uniquePages / pages.size();
    double seqRatio = (double)sequentialCount / (pages.size() - 1);

    if (avgGap < pages.size() / 4.0 && uniqueRatio < 0.5) {
        return "Locality-based (Optimal recommended)";
    } else if (seqRatio > 0.7) {
        return "Sequential (FIFO or LRU recommended)";
    } else {
        return "Random (Aging or Second Chance recommended)";
    }
}

// Main
int main() {
    int n, capacity, choice;
    vector<int> pages;

    clearConsole();
    cout << "===== Smart Memory Management and Algorithm Customization Simulator =====\n";
    cout<<"Please choose between 1 and 2"<<endl;
    cout << "1. Manual Input\n2. Auto Random Generation\nChoose Input Method: ";
    int inputChoice; cin >> inputChoice;

    if (inputChoice == 2) {
        cout << "Number of page references: ";
        cin >> n;
        pages.resize(n);
        srand(time(0));
        for (int i = 0; i < n; ++i) pages[i] = rand() % 10;
        cout << "Generated: ";
        for (int x : pages) cout << x << " ";
        cout << "\n";
    } else {
        cout << "Enter number of references: ";
        cin >> n;
        pages.resize(n);
        cout << "Enter page references: ";
        for (int i = 0; i < n; ++i) cin >> pages[i];
    }

    cout << "Enter number of frames: ";
    cin >> capacity;

    // Pattern Detection
    cout << "\n=== Memory Access Pattern Analysis ===\n";
    cout << "Detected Pattern: " << detectPattern(pages) << "\n";
    cout << string(40, '=') << "\n";

    do {
        cout << "\n--- Menu ---\n";
        cout << "1. FIFO\n2. LRU\n3. Optimal\n4. Second Chance\n5. LFU\n6. MFU\n7. Aging\n8. Custom Algorithm\n9. Run All + Report\n10. Clear Console\n0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        ofstream report("report.txt"), csv("report.csv");
        csv << "Algorithm,Hits,Faults,HitRate,MissRate,ExecutionTime(ms)\n";

        map<string, double> hitRates;
        int customRule = 0, windowSize = 4;

        if (choice == 8) {
            cout << "\n=== Custom Algorithm Designer ===\n";
            cout << "Choose eviction rule:\n";
            cout << "1. Evict page with fewest accesses in last 4 steps\n";
            cout << "2. Give priority to even-numbered pages\n";
            cout << "Enter rule choice: ";
            cin >> customRule;
            if (customRule == 1) {
                cout << "Enter window size for access counting (e.g., 4): ";
                cin >> windowSize;
            }
        }

        switch (choice) {
            case 1: fifo(pages, capacity, cout, csv, true); break;
            case 2: lru(pages, capacity, cout, csv, true); break;
            case 3: optimal(pages, capacity, cout, csv, true); break;
            case 4: secondChance(pages, capacity, cout, csv, true); break;
            case 5: lfu(pages, capacity, cout, csv, true); break;
            case 6: mfu(pages, capacity, cout, csv, true); break;
            case 7: aging(pages, capacity, cout, csv, true); break;
            case 8: custom(pages, capacity, cout, csv, true, customRule, windowSize); break;
            case 9: {
                auto f =fifo(pages, capacity, report, csv, false); hitRates["FIFO"] = 100.0 * f.first / pages.size();
                auto l = lru(pages, capacity, report, csv, false); hitRates["LRU"] = 100.0 * l.first / pages.size();
                auto o = optimal(pages, capacity, report, csv, false); hitRates["Optimal"] = 100.0 * o.first / pages.size();
                auto s = secondChance(pages, capacity, report, csv, false); hitRates["SecondChance"] = 100.0 * s.first / pages.size();
                auto lf = lfu(pages, capacity, report, csv, false); hitRates["LFU"] = 100.0 * lf.first / pages.size();
                auto mf = mfu(pages, capacity, report, csv, false); hitRates["MFU"] = 100.0 * mf.first / pages.size();
                auto ag = aging(pages, capacity, report, csv, false); hitRates["Aging"] = 100.0 * ag.first / pages.size();
                auto cu = custom(pages, capacity, report, csv, false, 1, 4); hitRates["Custom (Fewest in Window)"] = 100.0 * cu.first / pages.size();
                auto cu2 = custom(pages, capacity, report, csv, false, 2, 4); hitRates["Custom (Even Priority)"] = 100.0 * cu2.first / pages.size();

                cout << "\n\n=== HIT RATIO GRAPH (TEXT) ===\n";
                for (map<string, double>::iterator it = hitRates.begin(); it != hitRates.end(); ++it) {
                    cout << setw(20) << left << it->first << " | ";
                    int bars = (int)(it->second / 2);
                    for (int i = 0; i < bars; ++i) cout << "#";
                    cout << " (" << fixed << setprecision(2) << it->second << "%)\n";
                }

                double optimalRate = hitRates["Optimal"];
                string closestAlgo;
                double minDiff = 100.0;

                for (map<string, double>::iterator it = hitRates.begin(); it != hitRates.end(); ++it) {
                    string algo = it->first;
                    double rate = it->second;
                    if (algo == "Optimal") continue;
                    double diff = abs(rate - optimalRate);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestAlgo = algo;
                    }
                }

                cout << "\n Compare with Optimal:\n";
                cout << "Optimal Hit Rate   : " << fixed << setprecision(2) << optimalRate << "%\n";
                cout << "Closest Algorithm  : " << closestAlgo << " (" << hitRates[closestAlgo] << "%)\n";

                cout << "\n Report generated: report.txt and report.csv\n";
                break;
            }
            case 10: clearConsole(); break;
            case 0: cout << "Thanks for using the simulator!\n"; break;
            default: cout << "Invalid choice!\n"; break;
        }

        report.close();
        csv.close();
    } while (choice != 0);

    return 0;
}